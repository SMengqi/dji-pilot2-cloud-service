#define THIS_MODULE MODULE_LCF

#include "pl_utils.h"

#include <random>
#include <sstream>
#include <chrono>

#include "dji_cloud_api.pb.h"
#include "bxt_mqtt_config.pb.h"
#include "bxt_cloud_common.pb.h"
#include "jsonUtil.h"
#include "json2pb.h"

extern bxt_mqtt::mqtt_config_data s_pbMqttCfg;
extern bxt_cloud_common::common_message s_pbCommonCfg;

// 多线程安全的传输ID
std::string s_transId;
std::mutex s_transIdMutex;

// 飞行器控制模式
E_BxtUavControlMode s_uavControlMode = IDLE;
std::mutex s_uavControlModeMutex;

// 心跳req 计数
std::atomic<uint32_t> s_drcHeartbeatCount{0};
std::mutex s_heartbeatMutex;
std::condition_variable s_heartbeatCv;
std::atomic<bool> s_heartbeatAckReceived(false);
std::atomic<uint32_t> s_waitingHeartbeatSeq(0);

/*timer index*/
U32 ulHeartBeatTimerId = 0;

// 最后一次收到drc模式的控制时间 - 用于判断是否超时
U64 s_lastDrcControlTime = 0;


void updateTransId(const std::string& transId)
{
    std::lock_guard<std::mutex> lock(s_transIdMutex);
    s_transId = transId;
}

std::string generate_uuid(void)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    std::uniform_int_distribution<> dis2(8, 11);

    std::stringstream ss;
    int i;
    ss << std::hex;
    for (i = 0; i < 8; i++) {
        ss << dis(gen);
    }
    ss << "-";
    for (i = 0; i < 4; i++) {
        ss << dis(gen);
    }
    ss << "-4";
    for (i = 0; i < 3; i++) {
        ss << dis(gen);
    }
    ss << "-";
    ss << dis2(gen);
    for (i = 0; i < 3; i++) {
        ss << dis(gen);
    }
    ss << "-";
    for (i = 0; i < 12; i++) {
        ss << dis(gen);
    }
    return ss.str();
}

uint64_t get_milliseconds(void)
{
	 // 获取当前时间点
    auto now = std::chrono::high_resolution_clock::now();

    // 将当前时间点转换为时间戳（从1970年1月1日午夜开始的毫秒数）
    auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now).time_since_epoch().count();
	
	return now_ms;
}

// 辅助函数，获取消息ID对应的描述文本（提升日志可读性） 
const char* getMsgIdDesc(uint32_t ulMsgId)
{ 
    switch (ulMsgId) {
        case FLYTO_RESULT_DATA_IND:       return "指点飞行响应";
        case PAYLOAD_RESULT_DATA_IND:     return "负载控制响应"; 
        case WAYPOINT_RESULT_DATA_IND:    return "航点控制响应";
        case PAYLOAD_GIMBAL_DATA_IND:     return "负载-云台控制";
        case PAYLOAD_CAMERA_DATA_IND:     return "负载-相机控制";

        case COMMON_REG_IND:              return "通用-控制指令"; 

        default:                          return "未知消息"; 
        
    } 
} 

/************** 控制响应 **************/
/**
 * @brief 处理飞行器状态上报消息
 * @param  code 状态码
 * @param  msg 消息内容
 * @param  isAck 是否为响应消息
 * @return true 处理成功 false 处理失败
 */
bool handleFlightStatus(uint16_t code, const std::string& msg, bool isAck)
{
    dji_cloud::code_message message;

    message.set_code(code);
    message.set_msg(msg);
    if (isAck) {
        message.set_transid(s_transId);
    }

    std::string jsonStr;
    if (!proto_to_json(message, jsonStr))
    {
        pl_log(ERR, "Protobuf to JSON 失败");
        return false;
    }
    pl_log(INF, "状态 | code: %d, message: %s", code, message.msg().c_str());
    pf_copy_msg(THIS_MODULE, BXT_UAV_STATUS_PUBLISH_DATA_IND, MODULE_MQTTPUB, (void*)jsonStr.c_str(), jsonStr.size());
    return true;
}

bool handleFlightResult(const std::string& result)
{
    dji_cloud::result_message message;

    message.set_result(result);
    message.set_transid(s_transId);
    std::string jsonStr;
    if (!proto_to_json(message, jsonStr))
    {
        pl_log(ERR, "Protobuf to JSON 失败");
        return false;
    }
    pl_log(INF, "控制结果 | result: %s", result.c_str());
    pf_copy_msg(THIS_MODULE, BXT_UAV_RESULT_PUBLISH_DATA_IND, MODULE_MQTTPUB, (void*)jsonStr.c_str(), jsonStr.size());
    return true;
}

bool handleUavResult(uint16_t code, const std::string& msg, int result)
{
    std::string ul_msg = std::move(msg);
    handleFlightStatus(code, ul_msg);
    handleFlightResult(result == 0? "0" : "1");
    return true;
}
/************** 控制响应 **************/

/******* UAV CONTROL STATE MACHINE *******/
E_BxtUavControlMode getUavControlMode(void)
{
    std::lock_guard<std::mutex> lock(s_uavControlModeMutex);
    return s_uavControlMode;
}

bool setUavControlMode(E_BxtUavControlMode mode)
{
    std::lock_guard<std::mutex> lock(s_uavControlModeMutex);
    s_uavControlMode = mode;
    return true;
}

const char* getControlModeDesc()
{
    E_BxtUavControlMode mode = getUavControlMode();
    switch(mode) {
        case IDLE:           return "空闲";
        case WAYPOINT_MODE:  return "航点控制";
        case FLYTO_MODE:     return "飞行控制";
        default:             return "未知控制模式";
    }
}

/******** UAV CONTROL STATE MACHINE ********/

/************** 指令飞行模式 进入、退出 **************/
void drcModeEnter(dji_cloud::services_down& message)
{
    dji_cloud::request_data* p_data = message.mutable_data();
    dji_cloud::data_brokers* p_brokers = p_data->mutable_mqtt_broker();

    message.set_tid(generate_uuid());
    message.set_bid(generate_uuid());
    U64 timestamp = get_milliseconds();
    message.set_timestamp(timestamp);
    message.set_method("drc_mode_enter");

    p_data->set_hsi_frequency(1);
    p_data->set_osd_frequency(10);

    std::string address = s_pbMqttCfg.server_address();
    address = address.substr(6);
    std::string clientId = "drcEnter-" + s_pbMqttCfg.pub_client_id();
    
    p_brokers->set_address(address);
    p_brokers->set_username(s_pbMqttCfg.user_name());
    p_brokers->set_password(s_pbMqttCfg.password());
    p_brokers->set_client_id(clientId);
    p_brokers->set_enable_tls(false);
    double expire_time = timestamp * 0.001 + 3600;
    p_brokers->set_expire_time(expire_time);
}

bool drcModeExit(void)
{
    dji_cloud::services_down message;
    message.set_tid(generate_uuid());
    message.set_bid(generate_uuid());
    message.set_timestamp(get_milliseconds());
    message.set_method("drc_mode_exit");
    message.set_gateway(s_pbCommonCfg.dock_sn());
    dji_cloud::request_data* p_data = message.mutable_data();

    std::string jsonStr;
    jsonStr = pb2json(message);
    if (jsonStr.empty()) {
        pl_log(ERR, "Protobuf to JSON 失败 | method=%s", message.method().c_str());
        return false;
    }
    pl_log(INF, "发送请求 | method=%s | data=%.*s", message.method().c_str(), static_cast<int>(jsonStr.size()), jsonStr.data());

    pf_copy_msg(THIS_MODULE, DJI_SERVICES_PUBLISH_DATA_IND, MODULE_MQTTPUB, (void*)jsonStr.c_str(), jsonStr.size());
    
    s_drcHeartbeatCount = 0;
    s_lastDrcControlTime = 0;

    return true;
}

void drcUpHeartbeat(const std::string& msg)
{
    dji_cloud::drc_up_down message;
    if (!json_to_proto(msg, message))
    {
        pl_log(ERR, "JSON to proto 转换失败 | 原始数据: %.*s",
               static_cast<int>(msg.size()), msg.data());
        return;
    }

    {
        std::lock_guard<std::mutex> lock(s_heartbeatMutex);
        // 检查是否是等待中的心跳
        if (message.seq() == s_waitingHeartbeatSeq.load()) {
            pl_log(INF, "收到有效心跳响应 | seq=%u", message.seq());
            s_heartbeatAckReceived = true;
            s_heartbeatCv.notify_one();
            return;
        }
    }
    
    // seq=0可能是服务器端发送的心跳请求，不是错误
    if (message.seq() == 0) {
        pl_log(INF, "收到服务器心跳请求 | seq=0");
        return;
    }
}

bool drcDownHeartbeat(void)
{
    dji_cloud::services_down message;
    dji_cloud::request_data* p_data = message.mutable_data();
    
    uint32_t currentSeq  = s_drcHeartbeatCount.fetch_add(1);
    message.set_seq(currentSeq);
    message.set_method("heart_beat");
    p_data->set_timestamp(get_milliseconds());

    // 记录等待的seq
    {
        std::lock_guard<std::mutex> lock(s_heartbeatMutex);
        s_waitingHeartbeatSeq = currentSeq;
        s_heartbeatAckReceived = false;
    }

    std::string jsonStr = pb2json(message);
    if (jsonStr.empty()) {
        pl_log(ERR, "Protobuf to JSON 失败 | method=%s", message.method().c_str());
        return false;
    }
    pl_log(INF, "发送心跳包 | data=%.*s", static_cast<int>(jsonStr.size()), jsonStr.data());
    
    pf_copy_msg(THIS_MODULE, DJI_DRC_DOWN_PUBLISH_DATA_IND, MODULE_MQTTPUB, (void*)jsonStr.c_str(), jsonStr.size());
    
     // 等待心跳确认（带超时）
    {
        std::unique_lock<std::mutex> lock(s_heartbeatMutex);
        auto timeout = std::chrono::system_clock::now() + std::chrono::seconds(3);
        if (!s_heartbeatCv.wait_until(lock, timeout, []{
            return s_heartbeatAckReceived.load();
        })) {
            pl_log(WARN, "心跳超时未收到响应 | seq=%u", currentSeq);
            return false; // 超时处理
        }
    }
    
    // pl_log(INF, "心跳响应确认成功 | seq=%u", currentSeq);

    return true;
}

bool drcDownHeartbeatWithSeq(uint32_t seq)
{
    dji_cloud::services_down message;
    dji_cloud::request_data* p_data = message.mutable_data();
    
    message.set_seq(seq);
    message.set_method("heart_beat");
    p_data->set_timestamp(get_milliseconds());

    {
        std::lock_guard<std::mutex> lock(s_heartbeatMutex);
        s_waitingHeartbeatSeq = seq;
        s_heartbeatAckReceived = false;
    }

    std::string jsonStr = pb2json(message);
    if (jsonStr.empty()) {
        pl_log(ERR, "Protobuf to JSON 失败 | method=%s", message.method().c_str());
        return false;
    }
    pl_log(INF, "发送心跳包 | seq=%u, data=%.*s", seq, static_cast<int>(jsonStr.size()), jsonStr.data());
    
    pf_copy_msg(THIS_MODULE, DJI_DRC_DOWN_PUBLISH_DATA_IND, MODULE_MQTTPUB, (void*)jsonStr.c_str(), jsonStr.size());
    
    {
        std::unique_lock<std::mutex> lock(s_heartbeatMutex);
        auto timeout = std::chrono::system_clock::now() + std::chrono::seconds(3);
        if (!s_heartbeatCv.wait_until(lock, timeout, [seq]{
            return s_heartbeatAckReceived.load() && s_waitingHeartbeatSeq.load() == seq;
        })) {
            pl_log(WARN, "心跳超时未收到响应 | seq=%u", seq);
            return false;
        }
    }

    return true;
}
