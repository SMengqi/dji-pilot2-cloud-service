#define THIS_MODULE MODULE_DRC_COMMON
#include <algorithm>
#include "event.h"
#include "pl.h"
#include "ConsumerHandler.h"

using namespace std;

ConsumerHandler::ConsumerHandler()
{
    this->pKafkaConsumer = NULL;
}

ConsumerHandler::~ConsumerHandler()
{
    if(NULL != pKafkaConsumer)
    {
        delete pKafkaConsumer;
        pKafkaConsumer = NULL;        
    }
}


U32 ConsumerHandler::InitConsumer(KAFKA_CONSUMER_PARAM & ConsumerParam)
{
    this->ulDataType = ConsumerParam.dataType;
    this->pRecvFunc = ConsumerParam.pRecvFunc;
    this->pEventFunc = ConsumerParam.pEventFunc;
    this->pKafkaConsumer = new kafka_consumer();
    
    if(NULL==pKafkaConsumer || NULL==pRecvFunc || NULL==pEventFunc || !strTopicName.empty())
    {
        printf("new kafka_consumer error\n");
        pl_log(FATAL,"new kafka_consumer error");
        return CONSUMER_ERROR_OTHER;
    }

    //平台层用来统计用的，时间间隔设置为0，关闭统计信息
    this->pKafkaConsumer->set_statistics_inverval(0);
    printf("consumer_list_size: %d\n", ConsumerParam.pstKafkaUser->consumer_list_size());
    for(int idx = 0; idx < ConsumerParam.pstKafkaUser->consumer_list_size(); idx++)
    {  
        kafka_config::kafka_consumer_init_list* pConsumerCfg = ConsumerParam.pstKafkaUser->mutable_consumer_list(idx);
        printf("this data type: %d, consumer cfg type: %d\n", this->ulDataType, pConsumerCfg->data_type());
        if(this->ulDataType ==  pConsumerCfg->data_type())
        {
            this->strTopicName = pConsumerCfg->topics();
            this->pConsumerCfg = *pConsumerCfg;
            U32  ret = CreateConsumer();
            if(!ret)
            {
                printf("Create Consumer %s success!\n", strTopicName.c_str()); 
                pl_log(FATAL,"Create Consumer %s success!", strTopicName.c_str());
                return CONSUMER_OK;
            }
            else
            {
                printf("Create Consumer %s fail!\n", strTopicName.c_str()); 
                pl_log(FATAL,"Create Consumer %s fail!", strTopicName.c_str());
                delete pKafkaConsumer;
                this->pKafkaConsumer = NULL;
                return CONSUMER_ERROR_OTHER;
            }
        }
    }
    delete pKafkaConsumer;
    this->pKafkaConsumer = NULL;
    return CONSUMER_ERROR_OTHER;
}


U32 ConsumerHandler::CreateConsumer()
{
    if (pKafkaConsumer->configure(pConsumerCfg.brokers(), pConsumerCfg.topics(), pConsumerCfg.groupid(), pRecvFunc, pEventFunc))
    {
        printf("pConsumerCfg configure %s ERROR\n",  pConsumerCfg.topics().c_str());  
        pl_log(FATAL,"pConsumerCfg configure %s ERROR",  pConsumerCfg.topics().c_str());
        return CONSUMER_ERROR_OTHER;  
    }
    
    if(pConsumerCfg.has_sec_config())
    {
        kafka_config::kafka_init_security* pKafkaSec = pConsumerCfg.mutable_sec_config();
        pKafkaConsumer->configure_acl(pKafkaSec->security_protocol(),pKafkaSec->sasl_mechanism(),pKafkaSec->user_name(),pKafkaSec->user_passwd());
    }
    
    for(int idy = 0; idy < pConsumerCfg.kafka_init_list_size(); idy++)
    {
        kafka_config::kafka_init_data* pInitList = pConsumerCfg.mutable_kafka_init_list(idy);
        if (pKafkaConsumer->set_config(pInitList->config_key(), pInitList->config_value()))    
        {         
            printf("pConsumerCfg set %s config %s to %s failed\n", pConsumerCfg.topics().c_str(), pInitList->config_key().c_str(), pInitList->config_value().c_str());
            pl_log(FATAL,"pConsumerCfg set %s config %s to %s failed", pConsumerCfg.topics().c_str(), pInitList->config_key().c_str(), pInitList->config_value().c_str());
            return CONSUMER_ERROR_OTHER;
        }    
    }

    //pKafkaConsumer->set_consume_latest();

    if (pKafkaConsumer->create_consumer())    
    {         
        printf("pConsumerCfg create_consumer %s ERROR\n", pConsumerCfg.topics().c_str());   
        pl_log(FATAL,"pConsumerCfg create_consumer %s ERROR", pConsumerCfg.topics().c_str());  
        return CONSUMER_ERROR_OTHER;      
    } 

    if(pKafkaConsumer->set_consume_latest())
    {
        printf("pConsumerCfg set_consume_latest ERROR\n");   
        pl_log(FATAL,"pConsumerCfg set_consume_latest ERROR"); 
        return CONSUMER_ERROR_OTHER;
    }      

    return CONSUMER_OK;
}

U32 ConsumerHandler::ConsumerReadKafka()
{
    while(1)
    {
        pKafkaConsumer->recv();
    }
    return CONSUMER_OK;
}


U32 ConsumerHandler::CreateConsumerThread()
{
    if(NULL==pKafkaConsumer)
    {
        printf("pKafkaConsumer is NULL\n");
        pl_log(FATAL,"pKafkaConsumer is NULL");
        return CONSUMER_ERROR_OTHER;
    }

    pConsumerThread = new std::thread(&ConsumerHandler::ConsumerReadKafka,this);

    if(pConsumerThread)
    {
        printf("Create Consumer %s thread success!\n",strTopicName.c_str());
        pl_log(FATAL,"Create Consumer %s thread success!",strTopicName.c_str());
    }
    else
    {
        printf("Create Consumer %s thread fail!\n",strTopicName.c_str());
        pl_log(FATAL,"Create Consumer %s thread fail!",strTopicName.c_str());
        return CONSUMER_ERROR_OTHER;
    }
    
    return CONSUMER_OK;   
}

U32 ConsumerHandler::ResetConsumerHandle()
{
    delete pKafkaConsumer;

    this->pKafkaConsumer = new kafka_consumer();

    if(NULL == pKafkaConsumer)
    {
        printf("new kafka_producer error\n");
        pl_log(FATAL,"new kafka_producer error");
        return CONSUMER_ERROR_OTHER;
    }

    U32  ret = CreateConsumer();
    if(!ret)
    {
        printf("Create Consumer %s success!\n", strTopicName.c_str()); 
         pl_log(FATAL,"Create Consumer %s success!", strTopicName.c_str()); 
    }
    else
    {
        printf("Create Consumer %s fail!\n", strTopicName.c_str()); 
        pl_log(FATAL,"Create Consumer %s fail!", strTopicName.c_str()); 
        delete pKafkaConsumer;
        return CONSUMER_ERROR_OTHER;
    }

    return CONSUMER_OK;
}

std::string ConsumerHandler::GetConsumerTopicName()
{
    if(NULL == pKafkaConsumer)
    {
        return "";
    }

    if(!pConsumerCfg.has_topics())
    {
        return "";
    }

    return pConsumerCfg.topics();
}

void ConsumerHandler::ShowConfig()
{
    pKafkaConsumer->show_config();
}