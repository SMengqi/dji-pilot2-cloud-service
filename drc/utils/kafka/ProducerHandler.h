/*
 *  ProducerHandle.h
 *
 *  Created on: 2022-04-19
 *      Author: wangguangyu
 */
#ifndef _PRODUCER_HANDLER_H
#define _PRODUCER_HANDLER_H

#include <string>
#include <memory> 
#include "pl_type.h"
#include "pf_rdkafka.h"
#include "kafka_config.pb.h"
#include "if_drc_common.h"

#define ASYNC_DATA_NUM 50  //50条数据，调用一次flush

typedef enum
{
	PRODUCER_OK = 0,
    PRODUCER_ERROR_CONFIG,
    PRODUCER_ERROR_SET_CONFIG,
    PRODUCER_CREATE_FAIL,
	PRODUCER_ERROR_POINTER_IS_NULL,
	PRODUCER_ERROR_OTHER
}PRODUCER_ERROR_CODE_E;


typedef enum
{       
    KAFKA_PRODUCER_TOPIC_ID_DRSUDATA = 0,         
    KAFKA_PRODUCER_TOPIC_ID_DRCDATA,         
    KAFKA_PRODUCER_TOPIC_ID_EVENT,             
    KAFKA_PRODUCER_TOPIC_ID_VTDATA,         
    KAFKA_PRODUCER_TOPIC_ID_MAX,         
}KAFKA_PRODUCER_TOPIC_ID_E;


typedef struct
{
    kafka_config::kafka_user_list * pstKafkaUser;
    KAFKA_DATA_TYPE dataType;
    HANDLE_MQ_SEND_ERR errCallbackFunc;
}KAFKA_PRODUCER_PARAM;

class ProducerHandler
{
public:

private:
    U32 ulDataType;
    std::string strTopicName;
    std::string headerName;
    kafka_producer * pKafkaProducer;
    kafka_config::kafka_producer_init_list pbProducerCfg;
    HANDLE_MQ_SEND_ERR errCallBackF;
    bool bAsyncFlag; //异步写入标记
    U32  ulAsyncCount;//异步数据条数统计

public:
    ProducerHandler();
    ~ProducerHandler();

public:
    U32 InitProducer(KAFKA_PRODUCER_PARAM & ProducerParam);
    U32 CreateProducer();
    U32 ResetProducer();
    U32 SendMsg2Kafka(std::string &strMsgData ,U32 ulEventId);
    void ShowConfig();
private:
    std::string getTopic();

};

#endif