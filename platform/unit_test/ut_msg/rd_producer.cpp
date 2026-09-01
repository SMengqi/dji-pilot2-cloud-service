/******************************************
ä»£ç è®¾è®¡è¯´æ˜ï¼š
æœ¬æµ‹è¯•ç”¨ä¾‹æ˜¯ç”¨äºæµ‹è¯• kafka æ¶ˆæ¯çš„å„ç§å˜åŒ–å‚æ•°æ¡ä»¶ä¸‹çš„æµ‹è¯•æ—¶é—´
ç”±äºå¯¹æ—¶é—´ç²¾åº¦çš„éœ€æ±‚æ˜¯å¾®ç§’ï¼Œæ‰€ä»¥ã€€ï½‹ï½ï½†ï½‹ï½ã€€è‡ªèº«çš„æ—¶é—´æˆ³æ— æ³•è¾¾æ ‡ï¼Œ
æ‰€ä»¥æœ¬å®éªŒä¸­ï¼Œå°†æ—¶é—´ä½œä¸ºæ¶ˆæ¯å†…å®¹ï¼Œæ”¾åœ¨æ¶ˆæ¯ä¸­
åœ¨ã€€ï¼–ï¼”ï½‚ï½‰ï½”ç³»ç»Ÿä¸‹ï¼Œé‡‡ç”¨äº†ã€€ï½Œï½ï½ï½‡ã€€ç±»å‹ï¼ˆï¼–ï¼”ï½‚ï½‰ï½”ï¼‰çš„å˜é‡å­˜æ”¾æ—¶é—´
æ¶ˆæ¯æ„é€ è¿‡ç¨‹å¦‚ä¸‹ï¼š
ã€€ï¼‘ï¼æŒ‰ç”¨æˆ·è¾“å…¥ç”³è¯·ä¸€æ®µå†…å­˜ï¼Œä½œä¸ºæ¶ˆæ¯ä¼ é€’ç”¨çš„æ•°ç»„ï¼Œä½¿ç”¨ã€€memset å°†æ•´ä¸ªæ•°ç»„ç½®ï¼
ã€€ï¼’ï¼å¤´ã€€ï¼˜ã€€å­—èŠ‚å­˜æ”¾æ—¶é—´ä¿¡æ¯ï¼ˆå¾®ç§’ï¼‰

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

#include <sstream>


using std::cout;
using std::endl;
using std::cerr;
using std::string;

//#define DEFAULT_STAT_INTVAL 10000  
#define DEFAULT_TEST_DATA_LEN 1000
#define MAX_DATA_LEN 10020000

#define KAFKA_PRODUCER_TOPIC_ID_MAX 16

/*
typedef enum
{
    KAFKA_PRODUCER_TOPIC_ID_DRSUAIDATA = 0,        
}KAFKA_PRODUCER_TOPIC_ID_E;
*/

string gstrKafkaProducerKey[KAFKA_PRODUCER_TOPIC_ID_MAX];
kafka_producer *gpstKafkaProduce[KAFKA_PRODUCER_TOPIC_ID_MAX];


using google::protobuf::Message;
extern void json2pb(Message &msg, const char *buf, size_t size);

int my_handle_mq_send_err(int err)
{
    cout << " in my_handle_mq_send_err:  " << err << endl;
    return PF_RET_SUCCESS;
}

/*
 * ä¸ºäº†æ–¹ä¾¿è‡ªåŠ¨æµ‹è¯•ï¼Œä¼šè®®ä¸Šç¡®å®šä»¥ä¸‹å‚æ•°åœ¨å‘½ä»¤è¡Œè¿›è¡Œè¾“å…¥ï¼Œç”¨ï¼‚ï¼ï¼‚ä½œä¸ºé¡¹ç›®å’Œå€¼çš„åˆ†å‰²ç¬¦
1.(record-count=300)
2.(record-size=100000)
 .
3.(mode=sync/async)
4.(topic=test-producer-perf)
5.(headers=key:value)
6.(key=stringinfo)
7.(properties=./producer_1.properties)
8. brokers=172.16.8.31:9092 (å¯é€‰ï¼Œå¯åœ¨å‘½ä»¤è¡Œä¸­è¾“å…¥ï¼Œä¹Ÿå¯ä»¥åœ¨é…ç½®æ–‡ä»¶ä¸­ï¼Œé€šè¿‡è®¾ç½®ã€€bootstrap.servers æ¥å®ç°ï¼‰

 */

int pro_group_main (int argc, char* argv[])
{
    U32 ulLen = 0;
    S8* pscData = NULL;
    U32 idx = 0;
    U32 idy = 0;
    U32 index = 0;
    kafka_config::kafka_user_list kafkaUser;
    char *p;
    int test_rounds = 100;

     for (int i = 1; i < argc; i++)
    {
        if ((p = strstr(argv[i], "record-count")))
        {
            test_rounds = atoi(p + strlen("record-count") + 1);
            cout << "²âÊÔ" << test_rounds << "´Î " << endl;
        }
    }

    pf_get_file_length((S8 *)"kafka_config.txt", &ulLen);
    
    pscData = (S8*)pf_malloc(ulLen);
    
    if (pscData)
    {
        pf_read_flush_file((const S8 *)"kafka_config.txt", (const S8 *)pscData, ulLen);
        json2pb(kafkaUser, (const char *)pscData, ulLen);
    }
    
    printf("=== Find [ %d ] configs, file length is %d ==\n", kafkaUser.producer_list_size(), ulLen);

    for(idx = 0; idx < kafkaUser.producer_list_size(); idx++)
    {
        /* Fill gpstKafkaProduce[] with kafka_producer's pointer */
        gpstKafkaProduce[idx] = new kafka_producer;
        
        /*
        a)    configure
        b)    config ACL ()
        c)    set_config ()
        d)    create producer 
        */
       // index = KAFKA_PRODUCER_TOPIC_ID_MAX;
        kafka_config::kafka_producer_init_list* pProducerCfg = kafkaUser.mutable_producer_list(idx);

        if (gpstKafkaProduce[idx]->configure(pProducerCfg->brokers(), pProducerCfg->topics(), &my_handle_mq_send_err, pProducerCfg->idempotent()))
        {
            //printf("pProducerCfg configure %d ERROR\r\n", idx);      
            pl_log(ERR, "pProducerCfg configure %d ERROR ", idx);
            return -1;
        }
                              
        if(pProducerCfg->has_sec_config())
        {
            kafka_config::kafka_init_security* pKafkaSec = pProducerCfg->mutable_sec_config();
            //gpstKafkaProduce[idx]->configure_acl(pKafkaSec->security_protocol(),pKafkaSec->sasl_mechanism(),pKafkaSec->user_name(),pKafkaSec->user_passwd());
        }        
        
        for(idy = 0; idy < pProducerCfg->kafka_init_list_size(); idy++)
        {            
            kafka_config::kafka_init_data* pInitList = pProducerCfg->mutable_kafka_init_list(idy);    
            if (gpstKafkaProduce[idx]->set_config( pInitList->config_key(), pInitList->config_value()))    
            {         
                //printf("pProducerCfg set %d config %s to %s failed\r\n", idx, pInitList->config_key().c_str(), pInitList->config_value().c_str());      
                pl_log(ERR, "pProducerCfg set %d config %s to %s failed", idx, pInitList->config_key().c_str(), pInitList->config_value().c_str());
            }
        
        }
        //printf("set_config\r\n");

        //gpstKafkaProduce[idx]->show_config();  
        if (gpstKafkaProduce[idx]->create_producer())    
        {         
            //printf("pProducerCfg create_producer %d ERROR\r\n", idx);
            pl_log(ERR, "pProducerCfg create_producer %d ERROR ", idx);
            return -1;      
        }  
        //printf("create_produce\r\n");

        if(pProducerCfg->has_send_api_key())
        {
            gstrKafkaProducerKey[idx] = pProducerCfg->send_api_key();
            gpstKafkaProduce[idx]->set_config("custom.key", pProducerCfg->send_api_key());
        }
        else
        {
             //printf("has_send_api_key %d ERR\r\n", idx);
             pl_log(ERR, "pProducer has_send_api_key %d ERROR ", idx);
        }
        
    }
    
    string header = "drsu-test-kafka";
    string header_val = "00001";
//    string msg = "This is a test from DRSU 111";
    int pro_list_size = kafkaUser.producer_list_size();
    struct timeval tv;
    string data;
    long time_now;
    printf("pro_list_size = %d\r\n", pro_list_size);
    
    while(test_rounds)
    {   
        test_rounds--;
        printf("\n---test_rounds is %d---\n\n", test_rounds);
        for(idx = 0; idx < pro_list_size; idx++)
        {
            gettimeofday(&tv, NULL);    
            time_now = tv.tv_sec * 1000000 + tv.tv_usec;        
            std::stringstream ss;
            ss << time_now;
            data = ss.str() + "|" + std::to_string(idx);
//            data += std::to_string(idx);
            
            gpstKafkaProduce[idx]->send(data, header, header_val);
            printf("gpstKafkaProduce[%d]  send  data = %s \r\n header %s \r\n header_val %s \r\n", idx, data.c_str(), header.c_str(), header_val.c_str());
        }
        
        pf_usleep(20000);
    }
    
    return PF_RET_SUCCESS;
    
}

int pro_main (int argc, char* argv[]) 
{
    string brokers = "172.16.8.31:9092";
    //string topics = "test-acl-topic";
    string topics = "test_topic";
    //ACL
    string sasl_protocol = "SASL_PLAINTEXT";
    string sasl_mechanism = "SCRAM-SHA-512";
    string sasl_user = "Geely";
    string sasl_passwd = "geely123456";
    
    char *data;

    /* ÃüÁîĞĞÊäÈëÄ£Ê½:
    ./rd_producer  record-count=300 record-size=100000 ...
    1.(record-count=300)
    2.(record-size=100000)
    3.(mode=sync/async)
    4.(topic=test-producer-perf)
    5.(headers=key:value)
    6.(key=stringinfo)
    7.(properties=./producer_1.properties)
    */
    
    int test_rounds = 10; //default 10´Î
    int test_data_len = DEFAULT_TEST_DATA_LEN;
    int mode = 0; // 0: sync, else asyc;
    string topic;
    
    string headers;
    string header;
    string header_val;
    
    string keys;
    
    string file_path;
    
    FILE* fp = NULL;
    
    char *p;
    
    //cout << "rdkafka_producer_" << SVN_VER << endl;
    for (int i = 1; i < argc; i++)
    {
        if ((p = strstr(argv[i], "record-count")))
        {
            test_rounds = atoi(p + strlen("record-count") + 1);
            cout << "²âÊÔ" << test_rounds << "´Î¡" << endl;
        }
         
        if ((p = strstr(argv[i], "record-size")))
        {
            test_data_len = atoi(p + strlen("record-size") + 1);
            
            cout << "ÏûÏ¢³¤¶È " << test_data_len << endl;
            
            if ((test_data_len > 0) && (test_data_len < MAX_DATA_LEN))
            {
                data = (char *)malloc(test_data_len);
            }
            
            if (NULL == data)
            {
                cout << "·ÖÅä²»µ½ÄÚ´æ£¬ÍË³ö²âÊÔ" << endl;
                return PF_RET_FAILURE;
            }
        }
         
        if ((p = strstr(argv[i], "mode")))
        {
           if(0 == strcmp(p + strlen("mode") + 1, "async"))
           {
              mode = 1;
           } 
           // else : Ä¬ÈÏÎª sync Ä£Ê½ 
           cout << "mode : " << mode << endl;
        }
         
        if ((p = strstr(argv[i], "topic")))
        {
            topic = p + strlen("topic") + 1;
            cout << "topic Ãû×Ö: " << topic << endl;
        }
         
        if ((p = strstr(argv[i], "headers")))
        {
            headers = p + strlen("headers")+ 1;
            cout << "headers: " << headers << endl;
            
            char tmp[256] = {0};
            memset(tmp, 0, sizeof(tmp));
            memcpy(tmp, headers.c_str(), headers.length());
            header = strtok(tmp, ":");
            header_val = strtok(NULL, ":");
            cout << "header and val is : " << header << " " << header_val << endl;
        }
         
        if ((p = strstr(argv[i], "key")))
        {
            keys = p + strlen("key") + 1;
            cout << "key: " << keys << endl;
        }

        if ((p = strstr(argv[i], "properties")))
        {
            string this_path = p + strlen("properties") + 1;
            
            cout << "ÎÄ¼şÎ»ÖÃÏà¶ÔÎ»ÖÃ: " << this_path << endl;
            
            char tmp[1024] = {0};
            
            file_path = getcwd(tmp, sizeof(tmp));
            file_path += "/" + this_path;
            cout << "ÎÄ¼ş¾ø¶ÔÂ·¾¶: " << file_path << endl;
            
            fp = fopen(file_path.c_str(), "r");
            
            if (fp == NULL)
            {
             cout << " ÇëÌá¹©¹²ÕıÈ·ÅäÖÃÎÄ¼şµÄÎ»ÖÃ¼°Ãû³Æ" << endl;
            }
        }
        
        if ((p = strstr(argv[i], "brokers")))
        {
            brokers = p + strlen("brokers") + 1;
            cout << "brokers: " << brokers << endl;
        }
    }
    
    // ´´½¨Ò»¸ö producer ¶ÔÏó
    kafka_producer kp;
    //kp.set_statistics_inverval(0);

    if (kp.configure(brokers, topic, &my_handle_mq_send_err, false))
    {
        pf_free(data);
        return PF_RET_FAILURE;
    }
        
    if (mode)
    {
        kp.set_config("send.mode", "async");
    }
    
    // Èç¹û´æÔÚÅäÖÃÎÄ¼ş£¬Ôò°´ÅäÖÃÎÄ¼şµÄÄÚÈİ½øĞĞ²ÎÊıÅäÖÃ

    if (fp)
    {
        char content[2048] = {0};
        char para[256] = {0};
        char settings[256] = {0};
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
                        //ÉèÖÃ²ÎÊı
                        kp.set_config(tag, val);
                        
                        break;
                    }
                }
            }   
        }    
    }

    kp.show_config();
    
    
    // Æô¶¯ producer
    if (kp.create_producer())
    {
        return PF_RET_FAILURE;
    }
    
    // ¿ªÊ¼²âÊÔ

    struct timeval tv, tv_count, tv_count2;
    
    long time_now;
    
    gettimeofday(&tv_count, NULL);
    
    memset(data, 0, test_data_len);
    
    for (int i = 0; i < test_rounds; i++)
    {
        
        sleep(1);
	
        gettimeofday(&tv, NULL);
        
        time_now = tv.tv_sec * 1000000 + tv.tv_usec;
            
        memcpy(data, &time_now, sizeof(long));
        // ËÍ³öÊı¾İ:·Ö 4 ÖÖÇé¿ö  1 : ÎŞheader/key ; 2: ÎŞheader; 3: ÎŞ key ; 4.ÓĞ header + key
        
        if ((header.size() == 0) && (keys.size() == 0))
        {
            kp.send((S8*)data, test_data_len, NULL, 0, NULL, 0, NULL, 0);
        }
        
        if ((header.size() == 0) && (keys.size() > 0))
        {
            kp.send((S8*)data, test_data_len, NULL, 0, NULL, 0, (S8*)keys.c_str(), keys.size());
        }
        
        if ((header.size() > 0) && (keys.size() == 0))
        {
            kp.send((S8*)data, test_data_len, (S8*)header.c_str(), header.size(), (S8*)header_val.c_str(), header_val.size(), NULL, 0);
        }
        
        if ((header.size() > 0) && (keys.size() > 0))
        {
            kp.send((S8*)data, test_data_len, (S8*)header.c_str(), header.size(), (S8*)header_val.c_str(), header_val.size(), (S8*)keys.c_str(), keys.size());
        }
           
        if ((i+1) % 1000 == 0)
        {
            kp.flush();
        }
    }
    //cout << "call the pf_producer_rdkafka_throughput" << endl;
    //kp.pf_producer_rdkafka_throughput(); 
    //cout << "the pf_producer_rdkafka_throughput end" << endl;
    kp.flush();
    
    gettimeofday(&tv_count2, NULL);
    
    if (tv_count2.tv_usec < tv_count.tv_usec)
    {
        tv_count2.tv_usec += 1000000;
        tv_count2.tv_sec -= 1;
    }
    
    cout << "send out in  " << tv_count2.tv_sec - tv_count.tv_sec << " Seconds, "  \
                            << tv_count2.tv_usec - tv_count.tv_usec << " uSeconds"<< endl;

    free(data);
    
    if (fp) 
    {
        fclose(fp); 
    };
    
    return PF_RET_SUCCESS;
}


