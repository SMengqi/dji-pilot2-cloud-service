/*
 *  ConsumerHandlerCp.h
 *
 *  Created on: 2022-04-20
 *      Author: wangguangyu
 */
#ifndef _CONSUMER_HANDLER_H
#define _CONSUMER_HANDLER_H

#include <string>
#include <memory> 
#include <thread>
#include "pl_type.h"
#include "pf_rdkafka.h"
#include "kafka_config.pb.h"
#include "if_drc_common.h"

typedef enum
{
	CONSUMER_OK = 0,
    CONSUMER_CREATE_FAIL,
	CONSUMER_ERROR_POINTER_IS_NULL,
	CONSUMER_ERROR_OTHER
}CONSUMER_ERROR_CODE_E;


typedef enum
{
	KAFAK_CONSUMER_TOPIC_ID_DRSUAIDATA = 0,
    KAFKA_CONSUMER_TOPIC_ID_MAX         
}KAFKA_CONSUMER_TOPIC_ID_E;

typedef struct
{
    kafka_config::kafka_user_list * pstKafkaUser;
    KAFKA_DATA_TYPE dataType;
    APP_MSG_HANDLER pRecvFunc;
    APP_EVENT_HANDLER pEventFunc;    
}KAFKA_CONSUMER_PARAM;

class ConsumerHandler
{
public:
    U32 ulDataType;
    std::string strTopicName;
    kafka_consumer * pKafkaConsumer;
    kafka_config::kafka_consumer_init_list pConsumerCfg;
    APP_MSG_HANDLER pRecvFunc;
    APP_EVENT_HANDLER pEventFunc;

private:
    std::thread * pConsumerThread;

public:
    ConsumerHandler();
    ~ConsumerHandler();

public:
    U32 InitConsumer(KAFKA_CONSUMER_PARAM & ConsumerParam);
    U32 CreateConsumer();
    U32 ConsumerReadKafka();
    U32 CreateConsumerThread();
    U32 ResetConsumerHandle();
    void ShowConfig();
    
private:
    std::string GetConsumerTopicName();
};


#endif