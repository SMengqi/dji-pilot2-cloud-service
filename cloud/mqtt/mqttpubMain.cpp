#define THIS_MODULE MODULE_MQTTPUB

/* Includes ------------------------------------------------------------------*/
#include "mqttpubMain.h"

#include "bxt_mqtt_config.pb.h"

/* Private constants ---------------------------------------------------------*/

/* Exported values -----------------------------------------------------------*/

extern bxt_mqtt::mqtt_config_data s_pbMqttCfg;
/* Private values ------------------------------------------------------------*/
MqttPubMain* s_mqttPub = nullptr;
/* Private functions declaration ---------------------------------------------*/

MqttPubMain::MqttPubMain(std::string server, std::string clientId) : 
    MqttClient(server, clientId)
{}

void MqttPubMain::setTopicOsd(std::string topic)
{
    m_topicOsd = topic;
}

void MqttPubMain::setTopicDjiServices(std::string topic)
{
    m_topicDjiServices = topic;
}

void MqttPubMain::setTopicKmzResquest(std::string topic)
{
    m_topicKmzResquest = topic;
}

void MqttPubMain::setTopicDjiRequestsReply(std::string topic)
{
    m_topicDjiRequestsReply = topic;
}

void MqttPubMain::setTopicDjiDrcDown(std::string topic)
{
    m_topicDjiDrcDown = topic;
}

void MqttPubMain::setTopicBxtUavStatus(std::string topic)
{
    m_topicBxtUavStatus = topic;
}

void MqttPubMain::setTopicBxtUavResult(std::string topic)
{
    m_topicBxtUavResult = topic;
}

void MqttPubMain::setTopicBxtPayloadParam(std::string topic)
{
    m_topicBxtPayloadParam = topic;
}

std::string MqttPubMain::getTopicOsd(void)
{
    return m_topicOsd;
}

std::string MqttPubMain::getTopicDjiServices(void)
{
    return m_topicDjiServices;
}

std::string MqttPubMain::getTopicKmzResquest(void)
{
    return m_topicKmzResquest;
}

std::string MqttPubMain::getTopicDjiRequestsReply(void)
{
    return m_topicDjiRequestsReply;
}

std::string MqttPubMain::getTopicDjiDrcDown(void)
{
    return m_topicDjiDrcDown;
}

std::string MqttPubMain::getTopicBxtUavStatus(void)
{
    return m_topicBxtUavStatus;
}

std::string MqttPubMain::getTopicBxtUavResult(void)
{
    return m_topicBxtUavResult;
}

std::string MqttPubMain::getTopicBxtPayloadParam(void)
{
    return m_topicBxtPayloadParam;
}

/**********************************************************************************************
* @function   : mqttpub_init
*
* @discription: mqttpub_init
* @input      :
* @output     :
* @date       : 2025/10/22
**********************************************************************************************/
S32 mqttpub_init(U32 ulModuleId)
{
    pl_log(INF, "mqttpub init module ID is %d", ulModuleId);
    
    s_mqttPub = new MqttPubMain(s_pbMqttCfg.server_address(), s_pbMqttCfg.pub_client_id());
    s_mqttPub->setMqttOptions();
    s_mqttPub->setMqttUserAndPasswd(s_pbMqttCfg.user_name(), s_pbMqttCfg.password());
    s_mqttPub->connect();

    if(s_mqttPub->isConnected()) {
        pl_log(INF, "mqtt client connect success");
    } else {
        pl_log(ERR, "mqtt client connect failed");
    }

    bxt_mqtt::mqtt_publish* pbMqttPublish = s_pbMqttCfg.mutable_publish();
    s_mqttPub->setTopicOsd(pbMqttPublish->bxt_osd());
    s_mqttPub->setTopicDjiServices(pbMqttPublish->dji_services());
    s_mqttPub->setTopicKmzResquest(pbMqttPublish->kmz_request());
    s_mqttPub->setTopicDjiRequestsReply(pbMqttPublish->dji_requests_reply());
    s_mqttPub->setTopicDjiDrcDown(pbMqttPublish->dji_drc_down());
    s_mqttPub->setTopicBxtUavStatus(pbMqttPublish->bxt_uav_status());
    s_mqttPub->setTopicBxtUavResult(pbMqttPublish->bxt_uav_result());
    s_mqttPub->setTopicBxtPayloadParam(pbMqttPublish->bxt_payload_param());
    pl_log(INF, "publish topic is %s",pbMqttPublish->bxt_osd().c_str());

    return PF_RET_SUCCESS;
}

/**********************************************************************************************
* @function   : mqttpub_entry
*
* @discription: mqttpub_entry
* @input      :
* @output     :
* @date       : 2025/10/22
**********************************************************************************************/
void mqttpub_entry(U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, 
                               void* pcvMsg, U32 ulLength)
{
    std::string strMsg(static_cast<const char*>(pcvMsg), ulLength);
    std::string topic = "";
    switch(ulMsgId)
    {
        case TRACK_PUBLISH_DATA_IND: {
            topic = s_mqttPub->getTopicOsd();
            break;
        }
        case DJI_SERVICES_PUBLISH_DATA_IND: {
            topic = s_mqttPub->getTopicDjiServices();
            break;
        }
        case KMZ_REQUEST_PUBLISH_DATA_IND: {
            // topic = s_mqttPub->getTopicKmzResquest();
            break;
        }
        case DJI_REAUESTS_REPLY_PUBLISH_DATA_IND: {
            topic = s_mqttPub->getTopicDjiRequestsReply();
            break;
        }
        case DJI_DRC_DOWN_PUBLISH_DATA_IND: {
            topic = s_mqttPub->getTopicDjiDrcDown();
            break;
        }
        case BXT_UAV_STATUS_PUBLISH_DATA_IND: {
            topic = s_mqttPub->getTopicBxtUavStatus();
            break;
        }
        case BXT_UAV_RESULT_PUBLISH_DATA_IND: {
            topic = s_mqttPub->getTopicBxtUavResult();
            break;
        }
        case PAYLOAD_PARAM_PUBLISH_DATA_IND: {
            topic = s_mqttPub->getTopicBxtPayloadParam();
            break;
        }
        default: { 
            pl_log(ERR,"unexpected msg id %d(0x%x)", ulMsgId, ulMsgId); 
            return;
        }
    }
    if (topic.empty()) { return; }
    s_mqttPub->publish(topic, strMsg);
    pl_log(TRC, "mqttpub recv msg:%s -- %s, topic: %s", 
                pf_get_module_name(ulSrcModuleId), pf_get_event_name(ulMsgId), topic.c_str());
}
