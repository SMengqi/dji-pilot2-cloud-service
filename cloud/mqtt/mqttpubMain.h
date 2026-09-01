#ifndef MQTTPUBMAIN_H_
#define MQTTPUBMAIN_H_

#include "event.h"
#include "pl.h"
#include "pf_thread_mon.h"

#include "MqttClient.h"

class MqttPubMain : public MqttClient
{
public:
    MqttPubMain(std::string server, std::string clientId);
    ~MqttPubMain() = default;
    
    void setTopicOsd(std::string topic);
    void setTopicDjiServices(std::string topic);
    void setTopicKmzResquest(std::string topic);
    void setTopicDjiRequestsReply(std::string topic);
    void setTopicDjiDrcDown(std::string topic);
    void setTopicBxtUavStatus(std::string topic);
    void setTopicBxtUavResult(std::string topic);
    void setTopicBxtPayloadParam(std::string topic);

    std::string getTopicOsd(void);
    std::string getTopicDjiServices(void);
    std::string getTopicKmzResquest(void);
    std::string getTopicDjiRequestsReply(void);
    std::string getTopicDjiDrcDown(void);
    std::string getTopicBxtUavStatus(void);
    std::string getTopicBxtUavResult(void);
    std::string getTopicBxtPayloadParam(void);

private:
    std::string m_topicOsd;
    std::string m_topicDjiServices;
    std::string m_topicKmzResquest;
    std::string m_topicDjiRequestsReply;
    std::string m_topicDjiDrcDown;
    std::string m_topicBxtUavStatus;
    std::string m_topicBxtUavResult;
    std::string m_topicBxtPayloadParam;
};

#endif /*MQTTPUBMAIN_H_*/