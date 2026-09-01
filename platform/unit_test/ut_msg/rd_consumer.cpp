/******************************************
ä»£ç è®¾è®¡è¯´æ˜ï¼š
æœ¬æµ‹è¯•ç”¨ä¾‹æ˜¯ç”¨äºæµ‹è¯• kafka æ¶ˆæ¯çš„å„ç§å˜åŒ–å‚æ•°æ¡ä»¶ä¸‹çš„æµ‹è¯•æ—¶é—´


******************************************/
#define THIS_MODULE PLATFORM_EX
#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <csignal>
#include <cstring>
#include <atomic>
#include <thread>
#include <sys/time.h>
/*
 * Typically include path in a real application would be
 * #include <librdkafka/rdkafkacpp.h>
 */
#include <librdkafka/rdkafkacpp.h>
#include <librdkafka/rdkafka.h>

#include "pf_rdkafka.h"
#include <algorithm> 
#include <unistd.h>
#include "pl.h"
#include "kafka_config.pb.h"

using std::cout;
using std::endl;
using std::cerr;
using std::string;

#define DEBUG_TIME 1 
#define DEFAULT_TEST_DATA_LEN 1000
#define MAX_DATA_LEN 10020000
#define TMP_SMALL_MAX 256
#define TMP_BIG_MAX 1024
#define BUF_BIG_MAX 256
#define BUF_SMALL_MAX 64
#define REC_FILE_NAME_MAX 256  
#define CONTENT_MAX 2048
#define PARA_MAX 256
#define SETTINGS_MAX 256
#define KAFKA_CONSUMER_MAX 10

int test_rounds = 100; //default 100 ´Î

int test_data_len =  DEFAULT_TEST_DATA_LEN;

volatile int test_cnt = 0;

char* recv_data;

long* rec; //¼ÇÂ¼ data trip timeµÄÊı×éÖ¸Õë
long* rec_send;
long* rec_recv;

kafka_consumer *gpstKafkaConsumer[KAFKA_CONSUMER_MAX];

using google::protobuf::Message;
extern void json2pb(Message &msg, const char *buf, size_t size);

FILE* rec_file = NULL;  // ½« rrt µÄÊ±¼ä´æÎª csv ¸ñÊ½ÎÄ¼ş£¬¿ÉÓÃÓÚ excel ±íÍ³¼Æ·ÖÎö

/**
Interface for APP users:
  User should implement a APP_EVENT_HANDLER funciton to handle the events/errors.
  * 
  function Declear:
    void (*APP_EVENT_HANDLER)(int err_code); 
  * 
  User implement this function to handle rdkafka events/errors.
*/

void app_kafka_event_recv(int err_code)
{
    cout << " Get data from kafka: err_no is " << err_code << endl;
  /**
      When the rdKafka code gets events/errors, most of them are fatal or permanent errors, rdKafka lib can NOT recovery them. 
      What user has to do: stop consumer, destory the consumer object, and renew a consumer.
  */
}

/**
Interface for APP users:
  User should implement a APP_MSG_HANDLER funciton to receive the messages.
  * 
  function Declear:
    void (*APP_MSG_HANDLER)(RdKafka::Message* message, void* opaque); 
  * 
  When the rdKafka code get the message, it will fill out the data/header(if has)/key(if has)/err_code
  NOTE: 
  If something is wrong, this function will be called back too, with an non-SUCCESS err_code 
  * 
  * NOTE: If err_code is NOT SUCCESS, data/header/key is meaningless
  * 
*/

int my_msg_consume(RdKafka::Message* message, void* opaque) 
{
    int ret = -1;

    struct timeval tv_recv;
    static struct timeval tv_recv_start;
    static struct timeval tv_recv_end;
    
    long time_recv;
    long dtt_sum = 0;
    
    static bool its_first_msg = true;
    
    char tmp[TMP_SMALL_MAX] = {0};
    
    RdKafka::Headers* headers;
    std::vector<RdKafka::Headers::Header> headers_;
    
    switch (message->err()) 
    {
        // - ERR__TIMED_OUT - timeout_ms was reached with no new messages fetched.
        case RdKafka::ERR__TIMED_OUT:
            cerr << "Consume failed: Time out" << message->errstr() << endl;
            break;

        case RdKafka::ERR_NO_ERROR:
        //---------------------------------
        #if 0   // ÕâÀï¿ÉÒÔÏÔÊ¾Ò»Ğ©message ½á¹¹µÄĞÅÏ¢ ÈçÊ±¼ä´Á £¬headers. 
            RdKafka::MessageTimestamp ts;
            ts = message->timestamp();
            if (ts.type != RdKafka::MessageTimestamp::MSG_TIMESTAMP_NOT_AVAILABLE) 
            {

                if (ts.type == RdKafka::MessageTimestamp::MSG_TIMESTAMP_CREATE_TIME)
                {
                    cout << "Timestamp: " << "create time" << " " << ts.timestamp << endl;
                }
                
                if (ts.type == RdKafka::MessageTimestamp::MSG_TIMESTAMP_LOG_APPEND_TIME)
                {
                    cout << "Timestamp: " << "log append time" << " " << ts.timestamp << endl;
                }
            }
            cout << "Partition is " << message->partition() << endl;
            cout << "Latency is " << message->latency() << endl;
            
            headers = message->headers();
            if (headers)
            {
                headers_ = headers->get_all();
                cout << "HEADERS : " << headers_[0].key() << endl;
                cout << "VALUES : "<< headers_[0].value_string() << endl;
            }

        #endif
            printf("//---------------------------------\n");
        
            gettimeofday(&tv_recv, NULL);
            if (its_first_msg) 
            {
                /* Real message */
                gettimeofday(&tv_recv_start, NULL);
                its_first_msg = false;
            }
            
            memcpy(recv_data, static_cast<const char *>(message->payload()), static_cast<int>(message->len()));
            
            time_recv = *((long*) recv_data);
            
            printf(" recv_len is %d\n", static_cast<int>(message->len()));
            
           // dtt = (tv_recv.tv_sec * 1000000 +  tv_recv.tv_usec) - time_recv;
                        
#ifdef DEBUG_TIME
            rec_recv[test_cnt] = (tv_recv.tv_sec * 1000000 +  tv_recv.tv_usec);
            rec_send[test_cnt] = time_recv;
#endif
           // rec[test_cnt] = dtt;
            
           // printf("@ %d : Data Trip Time is  %ld usecs \n",  test_cnt + 1, (tv_recv.tv_sec * 1000000 +  tv_recv.tv_usec) - time_recv );
            
            if (++test_cnt == test_rounds)           
            {
                gettimeofday(&tv_recv_end, NULL);
                
                if (tv_recv_end.tv_usec < tv_recv_start.tv_usec)
                {
                    tv_recv_end.tv_usec += 1000000;
                    tv_recv_end.tv_sec -= 1;
                }
                
                cout << "Last read msg at offset " << message->offset() << endl;
                 
                printf("Total Time is %ld(sec) %ld(usec)\n", tv_recv_end.tv_sec - tv_recv_start.tv_sec, tv_recv_end.tv_usec - tv_recv_start.tv_usec);
                        
                for (int i = 0; i < test_rounds; i++)
                {
                    // Ğ´µ½¼ÇÂ¼ÎÄ¼şÖĞ
                    memset(tmp, 0, sizeof(tmp));
                    
                    rec[i] = rec_recv[i] - rec_send[i];
                    
                    sprintf(tmp, "%d, %ld, %ld, %ld\n", i + 1, rec_send[i], rec_recv[i], rec[i]);
            
                    fwrite(tmp, strlen(tmp), 1, rec_file);
            
                    dtt_sum += rec[i];
                    
                    #ifdef DEBUG_TIME
                    printf("@ %i = %ld\n", i, rec[i]);
                    printf("send@ %ld, recv@¡¡%ld\n", rec_send[i], rec_recv[i]);
                    #endif
                }
                
                std::sort(rec,rec + test_rounds);
                
                char buf[BUF_BIG_MAX] = {0};
                
                sprintf(buf, "min dtt = %ld (usec), max dtt = %ld (usec), avg = %ld(usec) \n", rec[0], rec[test_rounds -1], dtt_sum/test_rounds);
               
                fwrite(buf, strlen(buf), 1, rec_file);
                printf("%s\n", buf);
                ret = test_rounds;
                
            }
            else
            {
                ret = 0;
            }
            break;

        //- ERR__PARTITION_EOF - End of partition reached, not an error.
        case RdKafka::ERR__PARTITION_EOF:
            /* Last message */
            cout << "============= last message =============" << endl; 
            break;

        case RdKafka::ERR__UNKNOWN_TOPIC:
        case RdKafka::ERR__UNKNOWN_PARTITION:
            std::cerr << "Consume failed: " << message->errstr() << std::endl;
            break;

        default:
             /* Errors */
            std::cerr << "Consume failed: " << message->errstr() << std::endl;
    }
    return ret;
}

extern "C" int kafka_consumer_1_recv(RdKafka::Message* message, void* opaque) 
{
    int ret = -1;

    RdKafka::Headers* headers;
    std::vector<RdKafka::Headers::Header> headers_;
    
    switch (message->err()) 
    {
        // - ERR__TIMED_OUT - timeout_ms was reached with no new messages fetched.
        case RdKafka::ERR__TIMED_OUT:
            printf("Consume failed: Time out \r\n");
            break;

        case RdKafka::ERR_NO_ERROR:
        //---------------------------------
            printf("=============Processing message ============= \r\n");
            headers = message->headers();
            if (headers)
            {
                headers_ = headers->get_all();
                cout << "HEADERS : " << headers_[0].key() << endl;
                cout << "VALUES : " << headers_[0].value_string() << endl;
            }
                        
            RdKafka::MessageTimestamp ts;
            ts = message->timestamp();
            
            if (ts.type != RdKafka::MessageTimestamp::MSG_TIMESTAMP_NOT_AVAILABLE)
            {
                if (ts.type == RdKafka::MessageTimestamp::MSG_TIMESTAMP_CREATE_TIME)
                {
                    cout << "Timestamp: " << "create time" << " " << ts.timestamp << endl;
                }
                
                if (ts.type == RdKafka::MessageTimestamp::MSG_TIMESTAMP_LOG_APPEND_TIME)
                {
                    cout << "Timestamp: " << "log append time" << " " << ts.timestamp << endl;
                }
            }
            break;

        //- ERR__PARTITION_EOF - End of partition reached, not an error.
        case RdKafka::ERR__PARTITION_EOF:
            /* Last message */
            cout << "============= last message =============" << endl;
            break;

        case RdKafka::ERR__UNKNOWN_TOPIC:
        case RdKafka::ERR__UNKNOWN_PARTITION:
            cout <<"Consume failed \r\n"<< endl;

            break;

        default:
             /* Errors */
            printf("Consume failed \r\n");
    }
    return ret;
}

extern "C" void kafka_consumer_1_event(int err_code)
{
    cout << " Get data from kafka: err_no is " << err_code << endl;
  /**
      When the rdKafka code gets events/errors, most of them are fatal or permanent errors, rdKafka lib can NOT recovery them. 
      What user has to do: stop consumer, destory the consumer object, and renew a consumer.
  */
} 

int con_group_main(void)
{
    U32 idx = 0;
    U32 idy = 0;
    U32 index = 0;
    U32 ulLen = 0;
    S8* pscData = NULL;
    
    kafka_config::kafka_user_list kafkaUser;

    APP_MSG_HANDLER msg_recv_function;  // get the message
    
    APP_EVENT_HANDLER msg_event_function; // get the error report from rdkafka if something is wrong
    
    /* Set recv function pointer for kafka consumers. 
     * You can set different function-pointer for each consumer  OR set one function for ALL consumers, it up to you*/

    msg_recv_function = kafka_consumer_1_recv;
    msg_event_function = app_kafka_event_recv;
    pf_get_file_length((S8 *)"kafka_config.txt", &ulLen);
    pscData = (S8*)pf_malloc(ulLen);
    
    if (pscData)
    {
        pf_read_flush_file((const S8 *)"kafka_config.txt", (const S8 *)pscData, ulLen);
        json2pb(kafkaUser, (const char *)pscData, ulLen);
    }
 
    printf("=== Find [ %d ] configs, file length is %d ==\n", kafkaUser.consumer_list_size(), ulLen);

    for(idx = 0; idx < kafkaUser.consumer_list_size(); idx++)
    {    
        gpstKafkaConsumer[idx] = new kafka_consumer();
        
        kafka_config::kafka_consumer_init_list* pConsumerCfg = kafkaUser.mutable_consumer_list(idx);
        printf("pConsumerCfg->brokers()%s\r\n", pConsumerCfg->brokers().c_str());
        if (gpstKafkaConsumer[idx]->configure(pConsumerCfg->brokers(), pConsumerCfg->topics(), pConsumerCfg->groupid(), msg_recv_function, msg_event_function))
        {
            printf("pConsumerCfg configure %d ERROR\r\n", idx);
            pl_log(ERR, "pConsumerCfg configure %d ERROR ", idx);
            return -1;
        }
          
        if(pConsumerCfg->has_sec_config())
        {
            kafka_config::kafka_init_security* pKafkaSec = pConsumerCfg->mutable_sec_config();
            gpstKafkaConsumer[idx]->configure_acl(pKafkaSec->security_protocol(),pKafkaSec->sasl_mechanism(),pKafkaSec->user_name(),pKafkaSec->user_passwd());
        }

        for(idy = 0; idy < pConsumerCfg->kafka_init_list_size(); idy++)
        {
            kafka_config::kafka_init_data* pInitList = pConsumerCfg->mutable_kafka_init_list(idy);
            if (gpstKafkaConsumer[idx]->set_config(pInitList->config_key(), pInitList->config_value()))    
            {         
                printf("pConsumerCfg set %d config %s to %s failed\r\n", idx, pInitList->config_key().c_str(), pInitList->config_value().c_str());   
                pl_log(ERR, "pConsumerCfg set %d config %s to %s failed", idx, pInitList->config_key().c_str(), pInitList->config_value().c_str());
            }
        }
            
        gpstKafkaConsumer[idx]->show_config();            
            
        if (gpstKafkaConsumer[idx]->create_consumer())    
        {         
            printf("pConsumerCfg create_consumer %d ERROR\r\n", idx); 
            pl_log(ERR, "pConsumerCfg create_consumer %d ERROR ", idx);
            return -1;      
        }        

        gpstKafkaConsumer[idx]->start_recving(); 
        
        printf(" consumer idx is %d \r\n", idx);
        pl_log(INF, "consumer idx is %d ", idx);
              
    }
    
    return PF_RET_SUCCESS;
}


int con_main (int argc, char* argv[]) 
{
    string brokers = "172.16.8.31:9092";
    //string topics = "test-acl-topic";
    string topics = "test_topic";
    //ACL
    string sasl_protocol = "SASL_PLAINTEXT";
    string sasl_mechanism = "SCRAM-SHA-512";
    string sasl_user = "Geely";
    string sasl_passwd = "geely123456";
    string topic;

    //cout << "rdkafka_consumer_" << SVN_VER << endl;
    /* 
    1.(record-count=300)
    2.(record-size=100000)
    3.(topic=test-producer-perf)
    4.(properties=./consumer_1.properties)
    5.brokers=172.16.8.31:9092 (¿ÉÑ¡£¬¿ÉÒÔ´ÓÃüÁîĞĞÊäÈë£¬Ò²¿ÉÒÔÔÚÅäÖÃÎÄ¼şÖĞÌîĞ´ : bootstrap.servers )
    * 
    *  ¹ØÓÚÏû·ÑÕßµÄgroup id,¿ÉÒÔÔÚÅäÖÃÎÄ¼şÖĞÌîĞ´ : group.id
    */
    char *p;
    FILE *fp = NULL;
    string file_path;
    
    for (int i = 1; i < argc; i++)
    {
        //sleep(5);
        if ((p = strstr(argv[i], "record-count")))
        {
            test_rounds = atoi(p + strlen("record-count") + 1);
            cout << "²âÊÔ" << test_rounds << "´Î" << endl;
        }
         
        if ((p = strstr(argv[i], "record-size")))
        {
            test_data_len = atoi(p + strlen("record-size") + 1);
            
            cout << "ÏûÏ¢³¤¶È" << test_data_len << endl;
            
            if ((test_data_len > 0) && (test_data_len < MAX_DATA_LEN))
            {
                recv_data = (char *)malloc(test_data_len);
            }
            
            if (NULL == recv_data)
            {
                cout << "·ÖÅä²»µ½ÄÚ´æ£¬ÍË³ö²âÊÔ" << endl;
                return PF_RET_FAILURE;
            }
        }
         
        if ((p = strstr(argv[i], "topic")))
        {
            topic = p + strlen("topic") + 1;
            cout << "topic Ãû×Ö: " << topic << endl;
        }
        
        if ((p = strstr(argv[i], "properties")))
        {
            string this_path = p + strlen("properties") + 1;
            
            cout << "ÎÄ¼şÎ»ÖÃ Ïà¶ÔÎ»ÖÃ: " << this_path << endl;
            
            char tmp[TMP_BIG_MAX] = {0};
            
            file_path = getcwd(tmp, sizeof(tmp));
            file_path += "/" + this_path;
            cout << "ÎÄ¼ş¾ø¶ÔÂ·¾¶: " << file_path << endl;
            
            fp = fopen(file_path.c_str(), "r");
            
            if (fp == NULL)
            {
                cout << " ÇëÌá¹©ÕıÈ·µÄÅäÖÃÎÄ¼şµÄÎ»ÖÃ¼°Ãû³Æ  " << endl;
            }
        }
        
        if ((p = strstr(argv[i], "brokers")))
        {
            brokers = p + strlen("brokers") + 1;
            cout << "brokers: " << brokers << endl;
        }
    }
    // ·ÖÅäÒ»¸ö¼ÇÂ¼¿Õ¼ä
    rec = (long *)malloc(test_rounds*sizeof(long));
    
    if (NULL == rec)
    {
        cout << "·ÖÅä²»µ½Í³¼ÆÓÃµÄÄÚ´æ£¬ÍË³ö²âÊÔ" << endl;
        return PF_RET_FAILURE;
    }
    
#ifdef DEBUG_TIME    
    rec_send = (long *)malloc(test_rounds*sizeof(long));
    rec_recv = (long *)malloc(test_rounds*sizeof(long));
    
    if (!rec_send || !rec_recv)
    {
        cout << "·ÖÅä²»µ½Í³¼ÆÓÃµÄÄÚ´æ-2£¬ÍË³ö²âÊÔ" << endl;
        free(recv_data);
        return PF_RET_FAILURE;      
    }
#endif
     // ÓÃµ±Ç°Ê±¼ä×÷Îª¼ÇÂ¼ÎÄ¼şµÄÃû³Æ 20210331_151212_1000000_1000B_cpp.txt
    struct timeval tv;
     
    gettimeofday(&tv, NULL);

    char rec_file_name[REC_FILE_NAME_MAX] = {0};

    char buf[BUF_SMALL_MAX] = {0};

    struct tm *tm;

    time_t clock;
   
    time(&clock);
   
    tm = localtime(&clock);
   
    strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", tm);
    
    memset(rec_file_name, 0, sizeof(rec_file_name));
    
    strcat(rec_file_name, buf);
    
    memset(buf, 0, sizeof(buf));
    
    sprintf(buf, "_%d_%dB_cpp.txt", test_rounds, test_data_len);
    
    strcat(rec_file_name, buf);
 
    rec_file = fopen(rec_file_name, "w+");
    
    if (!rec_file)
    {
        cout << "ÎÄ¼şÎÄ¼şÎŞ·¨´´½¨£¬ÍË³ö²âÊÔ" << endl;
        free(rec_send);
        free(rec_recv);
        free(recv_data);
        return PF_RET_FAILURE;
    }
    
    
    // kconsumer config : brokers : topics : group_id : app_msg_handler : app_event_handler 
    // åˆ›å»ºä¸€ä¸ª consumer å¯¹è±¡
    kafka_consumer kc;
    // é…ç½® consumer ä¿¡æ¯ï¼Œé‡‡ç”¨å¤æ‚æ§åˆ¶é…ç½®

    if (kc.configure(brokers, topic, "new_group2", my_msg_consume, app_kafka_event_recv))
    {
        free(rec_send);
        free(rec_recv);
        free(recv_data);
        return PF_RET_FAILURE;
    }
    sleep(2);
    // å¦‚æœå­˜åœ¨é…ç½®æ–‡ä»¶ï¼Œåˆ™æŒ‰é…ç½®æ–‡ä»¶çš„å†…å®¹è¿›è¡Œå‚æ•°é…ç½®
    if (fp)
    {
        
        char content[CONTENT_MAX] = {0};
        char para[PARA_MAX] = {0};
        char settings[SETTINGS_MAX] = {0};
        int index = 0;
        int j = 0;
        cout << "start to process settings " << endl;
        int file_len = fread(content, 1, sizeof(content), fp);
        
        for (int i = 0; i < file_len; i++)
        {
            if ((content[i] == '\r') || (content[i] == '\n')) //windows=\r\n linux=\n
            {
                memset(para, 0, sizeof(para));
                if (i - index < 0)
                {
                    break;
                }
                
                memcpy(para, &content[index], (i - index));
                if (content[i] == '\r')
                {
                    index = i + 2;
                    i += 1;
                }
                else
                {
                    index = i + 1;
                }
                
                memset(settings, 0, sizeof(settings));
                
                for (j = 0; j < (int)sizeof(para); j ++)
                {
                    if(isalpha(para[j]))
                    {
                        memcpy(settings, &para[j], strlen(&para[j]));
                        cout << "get settings : " << settings << endl;
                        string tag;
                        string val;
                        tag = strtok(settings, "=");
                        val = strtok(NULL, "=");
                        cout << "tag and val : " << tag << " " << val << endl;
                        //ã€€è®¾ç½®å‚æ•°
                        kc.set_config(tag, val);
                        
                        break;
                    }
                }
            }
        }   
    }
    
    kc.show_config();
    // å¯åŠ¨ consumer
    if (kc.create_consumer())
    {
        free(rec_send);
        free(rec_recv);
        free(recv_data);
        return PF_RET_FAILURE;
    }
    
#if 0
    std::vector<RdKafka::TopicPartition*> query_parts;
    
    query_parts.push_back(RdKafka::TopicPartition::create(topic, 0, 1618019554525));
    
    RdKafka::ErrorCode err = kc.m_consumer->offsetsForTimes(query_parts, 5000);
      
    cout << "err is " << RdKafka::err2str(err) << endl;
    cout << " get offset " <<  query_parts[0]->topic() << endl;
    cout << " get offset " <<  query_parts[0]->offset() << endl;
#endif
    
    // åˆ›å»ºæ¥æ”¶å¾ªç¯ï¼Œç”¨äºæ¥æ”¶æ–°äº§ç”Ÿçš„æ¶ˆæ¯
    
#if 1
    while (1)
    {
        if (kc.recv() == (test_rounds)) 
        {
            cout << "Test finished" << endl;
            break;
        }

    }
#endif   
 
    sleep(1);
    free(rec);
    free(recv_data);
#ifdef DEBUG_TIME
    free(rec_send);
    free(rec_recv);
#endif
    fclose(rec_file);
    if (fp) 
    {
        fclose(fp); 
    }
    return PF_RET_SUCCESS;
}


