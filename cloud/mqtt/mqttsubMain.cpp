#define THIS_MODULE MODULE_MQTTSUB

/* Includes ------------------------------------------------------------------*/
#include "mqttsubMain.h"

#include "json.h"
#include "bxt_mqtt_config.pb.h"
#include "bxt_cloud_common.pb.h"

#include <fstream>

/* Private constants ---------------------------------------------------------*/

/* Exported values -----------------------------------------------------------*/
extern bxt_mqtt::mqtt_config_data s_pbMqttCfg;
extern bxt_cloud_common::common_message s_pbCommonCfg;

// 两条独立的 osd 定频遥测：遥控器(网关)属性 / 飞行器属性，对应
// 《Pilot2(RC Plus 2)官方接口清单》第九节；命名沿用机场3的 s_droneOsd/s_dock3Osd 思路，
// 但机场3走"机场OSD"，这里走"遥控器OSD"。
std::string s_aircraftOsd;
std::string s_rcOsd;
// 飞行器state属性：control_source(云端控制权判断信号)就在这个topic里，不在osd里
// （见接口迁移设计文档第6节待确认事项1，2026-09-02已用官方文档确认判断依据）
std::string s_aircraftState;

/* Private values ------------------------------------------------------------*/
std::string MqttSubMain::m_topicsFileName = "config/bootup/mqtt_sub_topics.txt";
std::vector<std::string> MqttSubMain::m_topics;

/* Private functions declaration ---------------------------------------------*/
MqttSubMain::MqttSubMain(std::string strServer, std::string cliendId) :
    MqttClient(strServer, cliendId),
    m_topicHandlers{
        // drc_mode_enter/drc_mode_exit/heart_beat/cloud_control_auth_*（DRC链路管理/权限抢占）
        // 不归本项目实现，由控制平台另一组负责，故不在此路由（见设计文档第1节范围收窄说明）。
        {"takeoff_to_point",          [this](const std::string& payload) { handleFlytoResult(payload); }},
        {"return_home",               [this](const std::string& payload) { handleFlytoResult(payload); }},
        // return_home_cancel/fly_to_point_stop（机场3没有的新方法）暂不实现，留到其它部分完成后再补充。
        {"fly_to_point",              [this](const std::string& payload) { handleFlytoResult(payload); }},
        {"fly_to_point_update",       [this](const std::string& payload) { handleFlytoResult(payload); }},
        {"fly_to_point_progress",     [this](const std::string& payload) { handleFlytoProgress(payload); }},
        // osd_info_push（drc/up 上的飞行器实时位置推送）暂不接入，flyto模块改用 TrackMain::getUavPoint()。
        // {"osd_info_push",             [this](const std::string& payload) { handleFlytoOsdInfoPush(payload); }},
        {"drc_gimbal_reset",          [this](const std::string& payload) { handlePayloadResult(payload); }},
        {"drc_camera_screen_drag",    [this](const std::string& payload) { handlePayloadResult(payload); }},
        {"drc_camera_aim",            [this](const std::string& payload) { handlePayloadResult(payload); }},
        {"drc_camera_frame_zoom",     [this](const std::string& payload) { handlePayloadResult(payload); }},
        {"drc_camera_focal_length_set",[this](const std::string& payload) { handlePayloadResult(payload); }},
    }
{}

MqttSubMain::~MqttSubMain()
{}

std::string MqttSubMain::parseMethod(const std::string& payload)
{
    Json::Value root;
    Json::CharReaderBuilder readerBuilder;
    std::unique_ptr<Json::CharReader> reader(readerBuilder.newCharReader());
    std::string errs;

    // 反序列化 JSON
    bool parsingSuccessful = reader->parse(
        payload.c_str(), payload.c_str() + payload.size(),
        &root, &errs
    );

    if (!parsingSuccessful || !errs.empty()) {
        pl_log(ERR, "JSON解析失败: %s", errs.c_str());
        return "";
    }

    // 检查并获取 method 字段
    if (!root.isMember("method") || !root["method"].isString()) {
        pl_log(ERR, "method字段缺失或类型错误");
        return "";
    }

    return root["method"].asString();
}

void MqttSubMain::onRead(std::string topic, std::string payload)
{
    // services_reply / events / requests / drc/up 这几个 topic 的消息统一按 method 名分发，
    // 与机场3完全一致（见《Pilot2(RC Plus 2)官方接口清单》一、Topic总览）。
    if ( (topic.find("services_reply") != std::string::npos) ||
         (topic.find("/events") != std::string::npos)        ||
         (topic.find("/requests") != std::string::npos)      ||
         (topic.find("/drc/up") != std::string::npos)) {
        std::string method = parseMethod(payload);

        auto handler = m_topicHandlers.find(method);
        if (handler != m_topicHandlers.end()) {
            handler->second(payload);
        } else {
            if (method.empty()) {
                pl_log(ERR, "method字段为空或解析失败");
            } else {
                pl_log(TRC, "no handler for method: %s", method.c_str());
            }
        }
        topic = "";
        payload = "";
        return;
    }

    if (topic.find(s_aircraftOsd) != std::string::npos) {
        pf_copy_msg(THIS_MODULE, COMMON_REG_IND, MODULE_TRACK, (void*)payload.c_str(), payload.size());

    } else if (topic.find(s_aircraftState) != std::string::npos) {
        pf_copy_msg(THIS_MODULE, TRACK_STATE_DATA_IND, MODULE_TRACK, (void*)payload.c_str(), payload.size());

    } else if (topic.find(s_rcOsd) != std::string::npos) {
        pf_copy_msg(THIS_MODULE, COMMON_REG_IND, MODULE_DEVICE, (void*)payload.c_str(), payload.size());

    } else if (topic.find("flight/control") != std::string::npos) {
        pf_copy_msg(THIS_MODULE, COMMON_REG_IND, MODULE_FLYTO, (void*)payload.c_str(), payload.size());

    } else if (topic.find("gimbal/control") != std::string::npos) {
        pf_copy_msg(THIS_MODULE, PAYLOAD_GIMBAL_DATA_IND, MODULE_PAYLOAD, (void*)payload.c_str(), payload.size());

    } else if (topic.find("camera/zoom") != std::string::npos) {
        pf_copy_msg(THIS_MODULE, PAYLOAD_CAMERA_DATA_IND, MODULE_PAYLOAD, (void*)payload.c_str(), payload.size());
    }
    // TODO: 机场3还有 remote/debug(舱盖/开关机)、waypoint/(航线任务)、light/control(探照灯)、
    //       kmz-file/response 几条内部平台topic，均不在本次Pilot2迁移范围内，未接入。

    topic = "";
    payload = "";
}

S32 MqttSubMain::subscribeTopics(void)
{
    if (m_topics.empty()) {
        pl_log(ERR, "no subscribe topic, m_topics is empty");
        return PF_RET_FAILURE;
    }

    for (auto topic : m_topics) {
        subscribe(topic, s_pbMqttCfg.qos());
        pl_log(INF, " %s", topic.c_str());
    }
    return PF_RET_SUCCESS;
}

/* --------- flyto module common result msg --------- */
void MqttSubMain::handleFlytoResult(const std::string& payload) {
    pf_copy_msg(THIS_MODULE, FLYTO_RESULT_DATA_IND, MODULE_FLYTO, (void*)payload.c_str(), payload.size());

    pf_copy_msg(THIS_MODULE, PAYLOAD_RESULT_DATA_IND, MODULE_PAYLOAD, (void*)payload.c_str(), payload.size());
}

void MqttSubMain::handleFlytoHeartBeat(const std::string& payload) {
    pf_copy_msg(THIS_MODULE, FLYTO_RESULT_HEARTBEAT_DATA_IND, MODULE_FLYTO, (void*)payload.c_str(), payload.size());
}

void MqttSubMain::handleFlytoOsdInfoPush(const std::string& payload) {
    pf_copy_msg(THIS_MODULE, FLYTO_DRC_OSD_DATA_IND, MODULE_FLYTO, (void*)payload.c_str(), payload.size());
}

void MqttSubMain::handleFlytoProgress(const std::string& payload) {
    pf_copy_msg(THIS_MODULE, FLYTO_PROGRESS_DATA_IND, MODULE_FLYTO, (void*)payload.c_str(), payload.size());
}

/* --------- payload module common result msg --------- */
void MqttSubMain::handlePayloadResult(const std::string& payload) {
    pf_copy_msg(THIS_MODULE, PAYLOAD_RESULT_DATA_IND, MODULE_PAYLOAD, (void*)payload.c_str(), payload.size());
}

S32 MqttSubMain::loadSubTopics(void)
{
    string file_name = m_topicsFileName;
    pl_log(INF, "load subscribe topics from file:%s", file_name.c_str());

    std::ifstream fileSource(file_name);

    if (!fileSource.is_open()) {
        pl_log(ERR, "打开文件%s失败", file_name.c_str());
        return PF_RET_FAILURE;
    }

    // TODO: bxt_cloud_common.proto 里的 common_message 目前仍是 dock_sn/aircraft_sn 两个字段
    // （机场3语义）。Pilot2 场景下 dock_sn 概念应替换为"遥控器(网关)SN"，
    // 待确定新的配置schema后，把下面的 dock_sn()/aircraft_sn() 换成对应字段。
    std::string buffer;
    std::string rcOsd;
    while (fileSource >> buffer) {
        std::size_t pos = buffer.find("#");
        if (pos != std::string::npos) {
            buffer.replace(pos, 1, s_pbCommonCfg.dock_sn());
        } else if ((pos = buffer.find("+")) != std::string::npos) {
            if (buffer.find("osd") != std::string::npos) {
                rcOsd = buffer;
                rcOsd.replace(pos, 1, s_pbCommonCfg.dock_sn());
                buffer.replace(pos, 1, s_pbCommonCfg.aircraft_sn());
            } else if (buffer.find("state") != std::string::npos) {
                // 飞行器state(control_source)是本条唯一诉求，只订阅飞行器SN，不像osd那样还需要遥控器SN
                buffer.replace(pos, 1, s_pbCommonCfg.aircraft_sn());
            } else {
                buffer.replace(pos, 1, s_pbCommonCfg.dock_sn());
            }
        }
        m_topics.push_back(buffer);
        if (!rcOsd.empty()) {
            m_topics.push_back(rcOsd);
        }
        rcOsd.clear();
    }

    fileSource.close();
    return PF_RET_SUCCESS;
}

/**********************************************************************************************
* @function   : mqttsub_init
**********************************************************************************************/
S32 mqttsub_init(U32 ulModuleId)
{
    pl_log(INF, "mqttsub init module ID is %d", ulModuleId);

    pl_log(INF, "address: %s", s_pbMqttCfg.server_address().c_str());
    MqttSubMain::loadSubTopics();

    s_aircraftOsd = s_pbCommonCfg.aircraft_sn() + "/osd";
    s_rcOsd = s_pbCommonCfg.dock_sn() + "/osd";
    s_aircraftState = s_pbCommonCfg.aircraft_sn() + "/state";

    return PF_RET_SUCCESS;
}

/**********************************************************************************************
* @function   : mqttsub_entry
**********************************************************************************************/
void mqttsub_entry(pf_addrword_t mid)
{
    MqttSubMain mqttsub(s_pbMqttCfg.server_address(), s_pbMqttCfg.sub_client_id());
    mqttsub.setOnReadCallback([&mqttsub](std::string topic, std::string payload) {
        mqttsub.onRead(topic, payload);
    });

    mqttsub.setMqttUserAndPasswd(s_pbMqttCfg.user_name(), s_pbMqttCfg.password());
    mqttsub.connect();

    if (mqttsub.isConnected()) {
        pl_log(INF, "mqtt sub client is connected");
        if (mqttsub.subscribeTopics() != PF_RET_SUCCESS) {
            pl_log(ERR, "订阅topics失败");
        }
    } else {
        pl_log(ERR, "mqtt sub client is not connected");
        s_reconnectFlag = true;
    }

    while (1) {
        if (s_reconnectFlag && mqttsub.isConnected()) {
            mqttsub.subscribeTopics();
            s_reconnectFlag = false;
        }
        pf_usleep(2000000);
    }
}
