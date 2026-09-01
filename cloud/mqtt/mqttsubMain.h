#ifndef MQTTSUBMAIN_H_
#define MQTTSUBMAIN_H_

#include "osport.h"
#include "event.h"
#include "pl.h"

#include "MqttClient.h"

#include <string>
#include <vector>
#include <unordered_map>

extern bool s_reconnectFlag;

class MqttSubMain : public MqttClient
{
public:
    MqttSubMain(std::string strServer, std::string cliendId);
    ~MqttSubMain();

    // 加载订阅的topic
    static S32 loadSubTopics(void);

    std::string parseMethod(const std::string& payload);
    void onRead(std::string topic, std::string payload);
    S32 subscribeTopics(void);

private:
    static std::string m_topicsFileName;
    // 订阅的topic
    static std::vector<std::string> m_topics;
    // method和处理函数的映射，见 mqttsubMain.cpp 构造函数注释里列出的待接入method清单
    std::unordered_map<std::string, std::function<void(const std::string&)>> m_topicHandlers;

    void handleFlytoResult(const std::string& payload);
    void handleFlytoHeartBeat(const std::string& payload);
    void handleFlytoOsdInfoPush(const std::string& payload);
    void handleFlytoProgress(const std::string& payload);

    void handlePayloadResult(const std::string& payload);
};

#endif /*MQTTSUBMAIN_H_*/
