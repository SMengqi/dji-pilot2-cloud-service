#define THIS_MODULE MODULE_TRACK

/* Includes ------------------------------------------------------------------*/
#include "trackMain.h"

#include "jsonUtil.h"
#include "pl_utils.h"
#include "json.h"
#include "bxt_if_uav.pb.h"
#include "bxt_cloud_common.pb.h"
#include "dji_cloud_api.pb.h"

/* Private constants ---------------------------------------------------------*/

/* Exported values -----------------------------------------------------------*/
extern bxt_cloud_common::common_message s_pbCommonCfg;

/* Private values ------------------------------------------------------------*/

/* Private functions declaration ---------------------------------------------*/
TrackMain::TrackMain() : m_uavId("")
{}

void TrackMain::setUavId(const std::string& uavId)
{
    m_uavId = uavId;
}

int TrackMain::getUavModeCode()
{
    return m_uavModeCode.load();
}

bool TrackMain::parseOsdMsg(const std::string& msg)
{
    m_flightInfo = FlightInfo();

    Json::Value root;
    Json::CharReaderBuilder builder;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    std::string errs;

    // 反序列化 JSON
    bool parsingSuccessful = reader->parse(
        msg.c_str(), msg.c_str() + msg.size(),
        &root, &errs
    );

    if (!parsingSuccessful || !errs.empty()) {
        pl_log(ERR, "JSON 解析失败: %s", errs.c_str());
        return false;
    }

    if (root.isMember("gateway")) {
        m_flightInfo.deviceId = root["gateway"].asString();
    } else {
        pl_log(WARN, "JSON中缺少gateway字段");
    }

    const Json::Value& data = root["data"];
    if (!data.isObject()) {
        pl_log(ERR, "data 不是一个对象");
        return false;
    }

    m_flightInfo.longitude = data.isMember("longitude")? data["longitude"].asDouble() : 0.0;
    m_flightInfo.latitude = data.isMember("latitude")? data["latitude"].asDouble() : 0.0;
    m_flightInfo.seaaltitude = data.isMember("height")? data["height"].asDouble() : 0.0;
    m_flightInfo.earthaltitude = data.isMember("elevation")? data["elevation"].asDouble() : 0.0;
    m_flightInfo.speed = data.isMember("horizontal_speed")? data["horizontal_speed"].asDouble() : 0.0;
    m_flightInfo.course = data.isMember("attitude_head")? data["attitude_head"].asDouble() : 0.0;

    // 飞机飞行模式码（用于判断外部接管：返航/降落等）。仅在变化时打日志，避免高频刷屏。
    int modeCode = data.isMember("mode_code")? data["mode_code"].asInt() : -1;
    int prevModeCode = m_uavModeCode.exchange(modeCode);
    if (modeCode != prevModeCode) {
        pl_log(INF, "飞机飞行模式变化 | mode_code: %d -> %d", prevModeCode, modeCode);
    }

    const Json::Value& battery = data["battery"];
    if (battery.isObject()) {
        if (battery.isMember("capacity_percent")) {
            m_flightInfo.battery.capacityPercent = battery["capacity_percent"].asInt();
        }
        if (battery.isMember("remain_flight_time")) {
            m_flightInfo.battery.remainFlightTime = battery["remain_flight_time"].asInt();
        }
    } else if (!battery.isNull()) {
        pl_log(WARN, "battery 不是一个对象，跳过解析");
    }

    // cameras 是字面量数组（已用真实抓包核实，见设计文档8.1节），跟机场3结构一致；
    // gimbal_pitch/gimbal_yaw 大概率在同一个数组元素里（跟机场3"按payload_index做动态key
    // 二次查找"的写法不同），但抓包时未挂载负载无法验证，挂载负载后需重新确认（设计文档第6节）。
    const Json::Value& camera = data["cameras"];
    bool cameraFound = false;
    if (camera.isArray() && camera.size() > 0) {
        for (Json::ArrayIndex i = 0; i < camera.size(); ++i) {
            const Json::Value& cam_info = camera[i];
            if (!cam_info.isObject()) continue;

            if (cam_info.isMember("payload_index")) {
                m_flightInfo.camera.payloadIndex = cam_info["payload_index"].asString();
                cameraFound = true;
            }
            if (cam_info.isMember("zoom_factor")) {
                m_flightInfo.camera.zoomFactor = static_cast<float>(cam_info["zoom_factor"].asDouble());
            }
            // 找到第一个相机元素后退出（如需处理多负载，可修改为遍历全部）
            break;
        }
    } else {
        pl_log(WARN, "cameras 数组为空，使用默认payloadIndex: %s", m_flightInfo.camera.payloadIndex.c_str());
    }

    // ★已用挂载负载的真实抓包核实（2026-09-02）：gimbal_pitch/gimbal_yaw不在cameras[]数组元素
    // 内部，而是在一个以payload_index命名的独立顶层动态key对象里，跟机场3原有的两段式查找逻辑
    // 完全一致——之前"大概率在同一个数组元素里"的推测是错的，已修正。
    if (cameraFound && data.isMember(m_flightInfo.camera.payloadIndex)) {
        const Json::Value& gimbalInfo = data[m_flightInfo.camera.payloadIndex];
        if (gimbalInfo.isObject()) {
            if (gimbalInfo.isMember("gimbal_pitch")) {
                m_flightInfo.gimbal.pitch = gimbalInfo["gimbal_pitch"].asDouble();
            }
            if (gimbalInfo.isMember("gimbal_yaw")) {
                m_flightInfo.gimbal.yaw = gimbalInfo["gimbal_yaw"].asDouble();
            }
        } else {
            pl_log(WARN, "payload index %s 对应的云台信息不是对象", m_flightInfo.camera.payloadIndex.c_str());
        }
    } else if (!cameraFound) {
        pl_log(WARN, "未找到有效的相机信息，使用默认payloadIndex: %s", m_flightInfo.camera.payloadIndex.c_str());
    }

    return true;
}

void TrackMain::packFlightInfo(void)
{
    if_uav::UAV_INFO_REPORT message;
    if_uav::UAVFlightInfo* flight_info = message.mutable_flightdata();
    if_uav::ExtraInfo* extra_info = message.mutable_extradata();

    message.set_uavid(m_uavId);
    message.set_timestamp(get_milliseconds());
    message.set_deviceid(m_flightInfo.deviceId);

    flight_info->set_longitude(m_flightInfo.longitude);
    flight_info->set_latitude(m_flightInfo.latitude);
    flight_info->set_seaaltitude(m_flightInfo.seaaltitude);
    flight_info->set_earthaltitude(m_flightInfo.earthaltitude);
    flight_info->set_speed(m_flightInfo.speed);
    flight_info->set_course(m_flightInfo.course);

    extra_info->set_battery(m_flightInfo.battery.capacityPercent);
    extra_info->set_endurance(m_flightInfo.battery.remainFlightTime);
    int pitch = static_cast<int>(m_flightInfo.gimbal.pitch * 10);
    extra_info->set_gimbalpan(pitch);
    int yaw = static_cast<int>(m_flightInfo.gimbal.yaw * 10);
    extra_info->set_gimbaltilt(yaw);
    extra_info->set_zoom(m_flightInfo.camera.zoomFactor);

    std::string json_str;
    if (proto_to_json(message, json_str)) {
        pl_log(TRC, "JSON 序列化成功: %s", json_str.c_str());
        pf_copy_msg(THIS_MODULE, TRACK_PUBLISH_DATA_IND, MODULE_MQTTPUB,
                    (void*)json_str.c_str(), static_cast<U32>(json_str.size()));
    } else {
        pl_log(ERR, "JSON 序列化失败");
    }
}

void TrackMain::packPayloadParam(void)
{
    dji_cloud::payload_param message;

    message.set_pitch(m_flightInfo.gimbal.pitch);
    message.set_yaw(m_flightInfo.gimbal.yaw);
    message.set_zoom_factor(m_flightInfo.camera.zoomFactor);

    std::string json_str;
    if (proto_to_json(message, json_str)) {
        pl_log(INF, "JSON 序列化成功: %s", json_str.c_str());
        pf_copy_msg(THIS_MODULE, PAYLOAD_PARAM_PUBLISH_DATA_IND, MODULE_MQTTPUB,
                    (void*)json_str.c_str(), static_cast<U32>(json_str.size()));
    } else {
        pl_log(ERR, "JSON 序列化失败");
    }
}

void TrackMain::handleOsdMsg(const char* msg)
{
    if (!msg) return;
    std::string json_str(msg);

    bool ret = parseOsdMsg(json_str);
    if (ret) {
        packFlightInfo();
        packPayloadParam();
    }
}

Point TrackMain::getUavPoint()
{
    return {m_flightInfo.longitude, m_flightInfo.latitude, m_flightInfo.seaaltitude, m_flightInfo.course};
}

std::string TrackMain::getPayloadIndex()
{
    return m_flightInfo.camera.payloadIndex;
}

float TrackMain::getZoomFactor()
{
    return m_flightInfo.camera.zoomFactor;
}

/**********************************************************************************************
* @function   : track_init
**********************************************************************************************/
S32 track_init(U32 ulModuleId)
{
    pl_log(INF, "track init module ID is %d", ulModuleId);
    TrackMain::getInstance().setUavId(s_pbCommonCfg.aircraft_sn());
    return PF_RET_SUCCESS;
}

/**********************************************************************************************
* @function   : track_entry
**********************************************************************************************/
void track_entry(U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId,
                               void* pcvMsg, U32 ulLength)
{
    switch (ulMsgId)
    {
        case COMMON_REG_IND: {
            TrackMain::getInstance().handleOsdMsg(static_cast<const char*>(pcvMsg));
        }
        break;

        default: { pl_log(ERR, "unexpected msg id %d(0x%x)", ulMsgId, ulMsgId); } break;
    }
}
