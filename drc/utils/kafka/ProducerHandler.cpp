#define THIS_MODULE MODULE_DRC_COMMON
#include <algorithm>
#include "event.h"
#include "pl.h"
#include "ProducerHandler.h"



ProducerHandler::ProducerHandler()
{
    this->headerName = "MsgId";
    this->pKafkaProducer = NULL;
    this->bAsyncFlag = false;
    this->ulAsyncCount = 0;
}

ProducerHandler::~ProducerHandler()
{
    if(NULL != pKafkaProducer)
    {
        delete pKafkaProducer;
        pKafkaProducer = NULL;        
    }
}


U32 ProducerHandler::InitProducer(KAFKA_PRODUCER_PARAM & ProducerParam)
{
    this->errCallBackF = ProducerParam.errCallbackFunc;
    this->ulDataType = ProducerParam.dataType;
    this->pKafkaProducer = new kafka_producer(); 

    if(NULL==pKafkaProducer || NULL==errCallBackF || NULL==ProducerParam.pstKafkaUser )
    {
        printf("new kafka_producer error\n");
        pl_log(FATAL,"new kafka_producer error");
        return PRODUCER_CREATE_FAIL;
    }

    //平台层用来统计用的，时间间隔设置为0，统计信息关闭 
    this->pKafkaProducer->set_statistics_inverval(0);

    int  listSize = ProducerParam.pstKafkaUser->producer_list_size();
    for(int idx = 0; idx < listSize; idx++)
    {      
        kafka_config::kafka_producer_init_list* pProducerCfg = ProducerParam.pstKafkaUser->mutable_producer_list(idx);

        if(this->ulDataType  == pProducerCfg->data_type())
        {
            this->pbProducerCfg = *pProducerCfg;
            this->strTopicName = pProducerCfg->topics();

            U32 ret = CreateProducer();                                  
            if(!ret)
            {
                printf("Create producer topic:%s send_api_key:%s success!\n",\
                                 pbProducerCfg.topics().c_str(), pbProducerCfg.send_api_key().c_str());  
                pl_log(FATAL,"Create producer %s success!\n",pbProducerCfg.topics().c_str());  
                return PRODUCER_OK;
            }  
            else
            {
                printf("Create producer %s fail!\n", pbProducerCfg.topics().c_str()); 
                pl_log(FATAL,"Create producer %s fail!", pbProducerCfg.topics().c_str()); 

                delete pKafkaProducer;

                pKafkaProducer = NULL;
                
                return PRODUCER_CREATE_FAIL;
            } 
        }
    } 
    
    delete pKafkaProducer;
    pKafkaProducer = NULL;    
    return PRODUCER_CREATE_FAIL;
}


U32 ProducerHandler::CreateProducer()
{   
    /*
    a)    configure
    b)    config ACL ()
    c)    set_config ()
    d)    create producer 
    */

    if(errCallBackF == NULL)
    {
        printf("pProducerCfg configure %s ERROR\n", pbProducerCfg.topics().c_str());
        pl_log(FATAL,"pProducerCfg configure %s ERROR", pbProducerCfg.topics().c_str());
        return PRODUCER_ERROR_POINTER_IS_NULL;
    }

    if (pKafkaProducer->configure(pbProducerCfg.brokers(), pbProducerCfg.topics(), errCallBackF, pbProducerCfg.idempotent()))
    {
        printf("pProducerCfg configure %s ERROR\n", pbProducerCfg.topics().c_str());  
        pl_log(FATAL,"pProducerCfg configure %s ERROR", pbProducerCfg.topics().c_str()); 
        return PRODUCER_ERROR_CONFIG;   
    }
                            
    if(pbProducerCfg.has_sec_config())
    {
        kafka_config::kafka_init_security* pKafkaSec = pbProducerCfg.mutable_sec_config();
        pKafkaProducer->configure_acl(pKafkaSec->security_protocol(),pKafkaSec->sasl_mechanism(),pKafkaSec->user_name(),pKafkaSec->user_passwd());
    }
    
    int ulListSize =  pbProducerCfg.kafka_init_list_size();
    for(int idy = 0; idy < ulListSize; idy++)
    {
        kafka_config::kafka_init_data* pInitList = pbProducerCfg.mutable_kafka_init_list(idy);
        if (pKafkaProducer->set_config(pInitList->config_key(), pInitList->config_value()))    
        {                   
            printf("pProducerCfg set %s config %s to %s failed\n", pbProducerCfg.topics().c_str(), pInitList->config_key().c_str(), pInitList->config_value().c_str());
            pl_log(FATAL,"pProducerCfg set %s config %s to %s failed", pbProducerCfg.topics().c_str(), pInitList->config_key().c_str(), pInitList->config_value().c_str());
            return PRODUCER_ERROR_SET_CONFIG;   
        }
        if(!pInitList->config_key().compare("send.mode") && !pInitList->config_value().compare("async"))
        {
            this->bAsyncFlag = true;
        }
    }     

    if(pbProducerCfg.has_send_api_key())
    {
        pKafkaProducer->set_config("custom.key", pbProducerCfg.send_api_key().c_str());
    } 

    if (pKafkaProducer->create_producer())    
    {         
        printf("pProducerCfg create_producer %s ERROR\n", pbProducerCfg.topics().c_str()); 
        pl_log(FATAL,"pProducerCfg create_producer %s ERROR", pbProducerCfg.topics().c_str()); 
        return PRODUCER_ERROR_OTHER;        
    }   

    return PRODUCER_OK ;
}



U32 ProducerHandler::SendMsg2Kafka(std::string &strMsgData ,U32 ulEventId)
{
    if(NULL == pKafkaProducer)
    {
        printf("pKafkaProducer is NULL\n");
        pl_log(FATAL,"pKafkaProducer is NULL");
        return PRODUCER_ERROR_POINTER_IS_NULL;
    }    

    std::string strEventId = std::to_string(ulEventId);

    int ret = this->pKafkaProducer->send(strMsgData,headerName,strEventId);
    if(ret)
    {
        printf("send msg to kafka fail!\n");
        pl_log(FATAL,"send msg to kafka fail!");
        return PRODUCER_ERROR_OTHER;
    }

    #if 0
    if(this->bAsyncFlag)
    {
        this->ulAsyncCount += 1;

        if(ASYNC_DATA_NUM >= ulAsyncCount)
        {
            this->pKafkaProducer->flush();
            this->ulAsyncCount = 0;
        }
    }
    #endif
  
    return PRODUCER_OK;
}

U32 ProducerHandler::ResetProducer()
{
    delete pKafkaProducer;

    this->pKafkaProducer = new kafka_producer(); 

    if(NULL == pKafkaProducer)
    {
        printf("new kafka_producer error\n");
        pl_log(FATAL,"new kafka_producer error");

        return PRODUCER_CREATE_FAIL;
    }

    U32 ret = CreateProducer();                                  

    if(!ret)
    {
        printf("Create producer %s success!\n",pbProducerCfg.topics().c_str());  
        pl_log(FATAL,"Create producer %s success!",pbProducerCfg.topics().c_str());  
    }  
    else
    {
        printf("Create producer %s fail!\n", pbProducerCfg.topics().c_str()); 
        pl_log(FATAL,"Create producer %s fail!", pbProducerCfg.topics().c_str()); 

        delete pKafkaProducer;

        pKafkaProducer = NULL;
        
        return PRODUCER_CREATE_FAIL;
    }   

    return PRODUCER_OK;
}


string ProducerHandler::getTopic()
{
    if(NULL == pKafkaProducer)
    {
        return "";
    }

    if(!pbProducerCfg.has_topics())
    {
        return "";
    }

    return pbProducerCfg.topics();
}


void ProducerHandler::ShowConfig()
{
    pKafkaProducer->show_config();
}


