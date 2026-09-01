#define THIS_MODULE PLATFORM_EX

#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <csignal>
#include <cstring>
#include <atomic>
#include <thread>
#include <fstream>
/*
 * Typically include path in a real application would be
 * #include <librdkafka/rdkafkacpp.h>
 */
#include <librdkafka/rdkafkacpp.h>
#include <librdkafka/rdkafka.h>

#include "pl.h"
#include "pl_type.h"
#include "pf_rdkafka.h"

using std::cout;
using std::endl;
using std::cerr;
using std::string;

/* 为将开发过程变得更加合理，将开放接口函数的动作分为几个阶段，
 * 第一阶段（ PHASE_1 )为配合目前已明确使用的接口，要重点进行测试
 * 第二阶段 ( PHASE_2 )为可能的使用场景，将视以后的需求再开放
 * 使用宏定义来区分不同阶段
 * 属于第一阶段的接口：　PHASE_1　
 * 属于第二阶段的接口：　PHASE_2
 */

static int pl_debug_rdproducer_stats_full_log_on = 0;

// polling time in milisecond
#define MAX_POLL_TIME 60*1000 
#define MIN_POLL_TIME 500
#define DEFAULT_POLL_TIME 1000 
#define PKAFKA_PRODUCER_MAX 32
#define ERRBUFMAX 512
#define IVL_STRMAX 128
#define FEATURESMAX 256
#define ERRBUFMAX 512
#define CONTENTMAX 2048
#define PARAMAX 256
#define SETTINGSMAX 256


//#define RD_PRODUCER_DEBUG 1

kafka_producer* pKafkaProducer[PKAFKA_PRODUCER_MAX] = {0};
U32 ulProducerNum = 0;
PF_MUTEX_T stKafkaProducerMutex = PTHREAD_MUTEX_INITIALIZER;

void kafka_producer::start_polling(S32 interval)
{
    if ((interval < MIN_POLL_TIME) || (interval > MAX_POLL_TIME))
    {
        interval = DEFAULT_POLL_TIME;
    }
    
    if ( m_execute.load(std::memory_order_acquire) ) 
    {
        m_execute.store(false, std::memory_order_release);
            
        if (m_thd.joinable())
        {
            m_thd.join();
        }
    }
        
    m_execute.store(true, std::memory_order_release);
        
    m_thd = std::thread([=]()
    {
        while (m_execute.load(std::memory_order_acquire)) 
        {
            m_producer->poll(1000/*block for max 1000ms*/);
            //cout << "polling" << endl;   
            pl_log(INF, "polling");                
            std::this_thread::sleep_for(
            std::chrono::milliseconds(interval));
        }
        //cout <<  "producer polling thread say goodbye---------" << endl;
        pl_log(FATAL, " producer polling thread say goodbye 0x%x---------", this);
    });
}


kafka_producer::kafka_producer()
{
    // Should we set default values here?
    #ifdef PHASE_2
    m_execute = false;
    #endif
	BOOL bFlag = FALSE;
    //mutex
    PF_MUTEX_LOCK(&stKafkaProducerMutex);
    if(ulProducerNum < PKAFKA_PRODUCER_MAX)
    {
        pKafkaProducer[ulProducerNum] = this;
        ulProducerNum++;
        bFlag = TRUE;
    }

    PF_MUTEX_UNLOCK(&stKafkaProducerMutex);

    if(bFlag)
    {
        pl_log(WARN, "producer 0x%x number=%d", this, ulProducerNum);
    }
    else
    {
        pl_log(FATAL, "producer 0x%x exceed PKAFKA_PRODUCER_MAX %d", this, PKAFKA_PRODUCER_MAX);
    }

}
    
kafka_producer::~kafka_producer()
{
    int iFlag = -1;
    U32 i;

    m_flag = FALSE;
    #ifdef PHASE_2
    if (m_execute.load(std::memory_order_acquire)) 
    {
        m_execute.store(false, std::memory_order_release);
        if (m_thd.joinable())
        {
            m_thd.join();
        }
    }
    #endif
    if (m_conf)
    {
        delete m_conf;
    }

    if (m_producer)
    {
        delete m_producer;
    }
    
    RdKafka::wait_destroyed(5000);
        
    //mutex
    PF_MUTEX_LOCK(&stKafkaProducerMutex);
    for(i = 0; i < ulProducerNum; i++)
    {
        if(this == pKafkaProducer[i])
        {
            pKafkaProducer[i] = NULL;
			iFlag = i;
            break;
        }
    }

    for(; i < ulProducerNum-1; i++)
    {
        pKafkaProducer[i] = pKafkaProducer[i+1];
    }
    
    if(iFlag>=0)
    {	
        //find the producer address
        ulProducerNum--;
    }
    PF_MUTEX_UNLOCK(&stKafkaProducerMutex);
    pl_log(FATAL, "rdkafka producer good bye 0x%x, Flag=%d, Num=%d", this, iFlag, ulProducerNum);

}

void kafka_producer::print_producer_static(void)
{
    pl_log(INF, "m_event_error_cnt=%d m_event_stats_cnt=%d m_event_log_cnt=%d m_default_cnt=%d m_status=\r\n%s", \
                 m_event_error_cnt, m_event_stats_cnt, m_event_log_cnt, m_default_cnt ,m_status.c_str());
    
    show_config(); 
}

S32 kafka_producer::configure_basic(string brokers, string topics)
{
    m_brokers = brokers;
    m_topics = topics;    
    
    string errstr;

/*
       rd_kafka_conf_t *conf;

        S8 features[256];
        size_t fsize = sizeof(features);

        printf("librdkafka %s\n", rd_kafka_version_str());

        conf = rd_kafka_conf_new();
        
        if (rd_kafka_conf_get(conf, "builtin.features", features, &fsize) !=
            RD_KAFKA_CONF_OK) {
                fprintf(stderr, "conf_get failed\n");
                return 1;
        }

        printf("builtin.features %s\n", features);

    
        rd_kafka_conf_t *rk_conf = rd_kafka_conf_new();;
*/
        
    
    /*
     * Create configuration object
     */
    m_conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
         
    if (m_conf == NULL)
    {
        //cerr << "Can not create RD kafka configue !" << endl;
        pl_log(ERR, "Can not create RD kafka configure !");
        return PF_RET_FAILURE;
    }
    
        
    /* Set bootstrap broker(s) as a comma-separated list of
     * host or host:port (default port 9092).
     * librdkafka will use the bootstrap brokers to acquire the full
     * set of brokers from the cluster. */
    if (m_conf->set("bootstrap.servers", m_brokers, errstr) != RdKafka::Conf::CONF_OK) 
    {
        //cerr << "bootstrap.servers set failed: " << errstr << endl;
        pl_log(ERR, "bootstrap.servers set failed: %s", errstr.c_str());
        return PF_RET_FAILURE;
    }
    //-----------
    
    if(m_conf->set("acks", "1", errstr) != RdKafka::Conf::CONF_OK)
    {
        //cerr << "Set ack failed: " << errstr << endl;
        pl_log(ERR, " Set ack failed: %s ", errstr.c_str());
        return PF_RET_FAILURE;
    }
    else
    {
        //cout << "Set acks ok !" << endl;
        pl_log(INF, " Set acks ok !" );
    }
    
    if(m_conf->set("batch.num.messages", "500", errstr) != RdKafka::Conf::CONF_OK)
    {
        //cerr << "Set batch.num.messages failed: " << errstr << endl;
        pl_log(ERR, " Set batch.num.messages failed: %s ", errstr.c_str());
    }
    else
    {
        //cout << "Set batch.num.messages ok !" << endl;
        pl_log(INF, "Set batch.num.messages ok !" );
    }

    if(m_conf->set("linger.ms", "1", errstr) != RdKafka::Conf::CONF_OK)
    {
        //cerr << "Set linger.ms failed: " << errstr << endl;
        pl_log(ERR, "Set linger.ms failed: %s ", errstr.c_str());
        return PF_RET_FAILURE;
    }
    else
    {
        //cout << "Set linger.ms ok !" << endl;
        pl_log(INF, " Set linger.ms ok !" );
    }

    if (m_statistics_on)
    {
        rd_kafka_conf_t *rk_conf = m_conf->c_ptr_global();

        char errbuf[ERRBUFMAX] = {0};

        char ivl_str[IVL_STRMAX] = {0};

        sprintf(ivl_str, "%d", m_statistics_interval);
    
        if (rd_kafka_conf_set(rk_conf, "statistics.interval.ms", ivl_str, errbuf, sizeof(errbuf)) != RD_KAFKA_CONF_OK)
        {
            //cout << "error is " << errbuf << endl;
            pl_log(ERR, "error is %s", errbuf);
            return PF_RET_FAILURE;
        }
    }

    return PF_RET_SUCCESS;
}

string kafka_producer::get_config(string para)
{
    string config;
    
    if(m_flag)
    {
        if (m_conf->get(para.c_str(), config) != RdKafka::Conf::CONF_OK)
        {
            //cout << "get " << para << " failed" << endl;
            pl_log(TRC, "get %s failed", para.c_str());   
        }
        else
        {
            //cout << para << ": " << config << endl;
            pl_log(UINF, "get_config: %s: %s", para.c_str(), config.c_str());
        }
    }    
    return config;
}

S32 kafka_producer::show_config(void)
{
    if(m_flag)
    {
        get_config("builtin.features");
        get_config("client.id");
        get_config("metadata.broker.list");
        get_config("bootstrap.servers");
        get_config("message.max.bytes");
        get_config("message.copy.max.bytes");   
        get_config("receive.message.max.bytes");
        get_config("max.in.flight.requests.per.connection");
        get_config("max.in.flight");   
        get_config("metadata.request.timeout.ms");
        
        get_config("topic.metadata.refresh.interval.ms");
        get_config("metadata.max.age.ms");
        get_config("topic.metadata.refresh.fast.interval");
        get_config("topic.metadata.refresh.fast.cn");
        get_config("topic.metadata.refresh.sparse");
        get_config("topic.metadata.propagation.max.ms");
        get_config("topic.blacklist");
        get_config("debug");
        get_config("socket.timeout.ms");
        get_config("socket.blocking.max.ms");
        
        get_config("socket.send.buffer.bytes"); 
        get_config("socket.receive.buffer.bytes");
        get_config("socket.keepalive.enable");
        get_config("socket.nagle.disable");
        get_config("socket.max.fails");
        get_config("broker.address.ttl");
        get_config("broker.address.family");
        //get_config("reconnect.backoff.jitter.ms");  Deprecated
        get_config("reconnect.backoff.ms");
        get_config("reconnect.backoff.max.ms");
        
        get_config("statistics.interval.ms");
        get_config("enabled_events");
        get_config("error_cb");
        get_config("throttle_cb");
        get_config("stats_cb");
        get_config("log_cb");
        get_config("log_level");
        get_config("log.queue");
        get_config("log.thread.name");
        get_config("enable.random.seed");
        
        get_config("log.connection.close");
        get_config("background_event_cb");
        get_config("socket_cb");
        get_config("connect_cb");
        get_config("closesocket_cb");  
        get_config("open_cb");
        get_config("opaque");
        get_config("default_topic_conf");
        get_config("internal.termination.signal");
        get_config("api.version.request");
        
        get_config("api.version.request.timeout.ms");
        get_config("api.version.fallback.ms");
        get_config("broker.version.fallback");
        get_config("security.protocol");
        get_config("ssl.cipher.suites");
        get_config("ssl.curves.list");
        get_config("ssl.sigalgs.list");
        get_config("ssl.key.location");
        get_config("ssl.key.password");
        get_config("ssl.key.pem");
        
        get_config("ssl_key");
        get_config("ssl.certificate.location");
        get_config("ssl.certificate.pem");
        get_config("ssl_certificate");
        get_config("ssl.ca.location");
        get_config("ssl_ca");
        get_config("ssl.ca.certificate.stores");
        get_config("ssl.crl.location");
        get_config("ssl.keystore.location");
        get_config("ssl.keystore.password");
        
        get_config("enable.ssl.certificate.verification");    
        get_config("ssl.endpoint.identification.algorithm");
        get_config("ssl.certificate.verify_cb");
        get_config("sasl.mechanisms");
        get_config("sasl.mechanism"); 
        get_config("sasl.kerberos.service.name");
        get_config("sasl.kerberos.principal");
        get_config("sasl.kerberos.kinit.cmd");
        get_config("sasl.kerberos.keytab"); 
        get_config("sasl.kerberos.min.time.before.relogin");
        
        get_config("sasl.username");
        get_config("sasl.password");
        get_config("sasl.oauthbearer.config"); 
        get_config("enable.sasl.oauthbearer.unsecure.jwt");
        get_config("oauthbearer_token_refresh_cb");
        get_config("plugin.library.paths");
        get_config("interceptors"); 
        get_config("client.rack");
        get_config("opaque");
        get_config("transactional.id");
        
        get_config("transaction.timeout.ms");
        get_config("enable.idempotence");
        get_config("enable.gapless.guarantee");
        get_config("queue.buffering.max.messages");
        get_config("queue.buffering.max.kbytes");
        get_config("queue.buffering.max.ms");
        get_config("linger.ms");
        get_config("message.send.max.retries");
        get_config("retries");
        get_config("retry.backoff.ms");
        
        get_config("queue.buffering.backpressure.threshold");
        get_config("compression.codec");
        get_config("compression.type");
        get_config("batch.num.messages");
        get_config("batch.size");
        get_config("delivery.report.only.error");
        get_config("dr_cb");
        get_config("dr_msg_cb");
        get_config("sticky.partitioning.linger.ms");
        
        // topic configuration: for producer
        get_config("request.required.acks");
        get_config("acks"); 
        get_config("request.timeout.ms");
        get_config("message.timeout.ms");  
        get_config("delivery.timeout.ms");
        get_config("queuing.strategy");
        get_config("produce.offset.report");
        get_config("partitioner");
        get_config("partitioner_cb");
        get_config("msg_order_cmp");
              
               
        get_config("compression.codec");
        get_config("compression.type");
        get_config("compression.level");
        get_config("acks");
    }
    
    return PF_RET_SUCCESS;
}

S32 kafka_producer::configure_cb(void)
{
    /* Set the delivery report callback.
     * This callback will be called once per message to inform
     * the application if delivery succeeded or failed.
     * See dr_msg_cb() above.
     * The callback is only triggered from ::poll() and ::flush().
     *
     * IMPORTANT:
     * Make sure the DeliveryReport instance outlives the Producer object,
     * either by putting it on the heap or as in this case as a stack variable
     * that will NOT go out of scope for the duration of the Producer object.
     */
    string errstr;
     
    if (m_conf->set("dr_cb", (DeliveryReportCb*)this, errstr) != RdKafka::Conf::CONF_OK) 
    {
        //std::cerr << errstr << std::endl;
        pl_log(ERR, "errstr is %s", errstr.c_str());
        return PF_RET_FAILURE;
    }
    // set event call back
    if (m_conf->set("event_cb", (EventCb*)this, errstr) != RdKafka::Conf::CONF_OK) 
    {
        //std::cerr << errstr << std::endl;
        pl_log(ERR, "errstr is %s", errstr.c_str());
        return PF_RET_FAILURE;
    }
    
    return PF_RET_SUCCESS;
}

S32 kafka_producer::configure_acl(string security_protocol, string sasl_mechanism, string user_name, string passwd)
{
    rd_kafka_conf_t *rk_conf = m_conf->c_ptr_global();

    if (rk_conf)
    {   
        CHAR features[FEATURESMAX];
        size_t fsize = sizeof(features);

        //printf("librdkafka %s\n", rd_kafka_version_str());
        pl_log(INF, " librdkafka %s\n", rd_kafka_version_str());
       
        if (rd_kafka_conf_get(rk_conf, "builtin.features", features, &fsize) != RD_KAFKA_CONF_OK) 
        {
            //fprintf(stderr, "conf_get failed\n");
            pl_log(ERR, "conf_get failed");
            return PF_RET_FAILURE;
        }

        //printf("builtin.features: %s\n", features);
        pl_log(INF, "builtin.features: %s ", features);
                
        CHAR errbuf[ERRBUFMAX] = {0};
        
        if (rd_kafka_conf_set(rk_conf, "security.protocol", security_protocol.c_str(), errbuf, sizeof(errbuf)))
        {
            //cerr << "fail to set security.protocol" << endl;
            //cout << errbuf << endl;
            pl_log(ERR, "fail to set security.protocol: %s", errbuf);
            return PF_RET_FAILURE;
            
        }
        
        if (rd_kafka_conf_set(rk_conf, "sasl.mechanism", sasl_mechanism.c_str(), errbuf, sizeof(errbuf)))
        {
            //cerr << "fail to set sasl.mechanism" << endl;
            //cout << errbuf << endl;
            pl_log(ERR, "fail to set sasl.mechanism: %s", errbuf);
            return PF_RET_FAILURE;
            
        }
                
        if (rd_kafka_conf_set(rk_conf,  "sasl.username", user_name.c_str(), errbuf, sizeof(errbuf)))
        {
            //cerr << "fail to set sasl.username" << endl;
            //cout << errbuf << endl;
            pl_log(ERR, "fail to set sasl.username: %s", errbuf);
            return PF_RET_FAILURE;
            
        }
        
        if (rd_kafka_conf_set(rk_conf, "sasl.password", passwd.c_str(), errbuf, sizeof(errbuf)))
        {
            //cerr << "fail to set sasl.password" << endl;
            //cout << errbuf << endl;
            pl_log(ERR, "fail to set sasl.password: %s", errbuf); 
            return PF_RET_FAILURE;
            
        }

    }
    else 
    {
        //cout << " can  not set ACL " << endl;
        pl_log(ERR, " Pl_rd_kafka_producer: can  not set ACL ");
        return PF_RET_FAILURE;
    }
    
    return PF_RET_SUCCESS;
}

S32 kafka_producer::create_producer(void)
{
    /*
     * Create producer instance.
     */
    string errstr;
    
    m_producer = RdKafka::Producer::create(m_conf, errstr);
  
    if (!m_producer) 
    {
        //std::cerr << "Failed to create producer: " << errstr << std::endl;
        pl_log(ERR, " Failed to create producer: %s", errstr.c_str());
        m_flag = FALSE;
        delete m_conf;
        return PF_RET_FAILURE;
    }
    
    m_flag = TRUE;
    return PF_RET_SUCCESS;
}

S32 kafka_producer::configure(string brokers, string topics)
{
    string errstr;
    
    if (configure_basic(brokers, topics) < 0)
    {
        return PF_RET_FAILURE;
    }

    if (configure_cb() < 0)
    {
        return PF_RET_FAILURE;
    }
    
    return PF_RET_SUCCESS;
}


S32 kafka_producer::configure(string brokers, string topics, HANDLE_MQ_SEND_ERR app_cb, BOOL idempotent)
{
    string errstr;
    
    if (configure_basic(brokers, topics) < 0)
    {
        return PF_RET_FAILURE;
    }

    if (configure_cb() < 0)
    {
        return PF_RET_FAILURE;
    }
    
    if (idempotent)
    {
        /* Enable the idempotent producer */
        if (m_conf->set("enable.idempotence", "true", errstr) != RdKafka::Conf::CONF_OK) 
        {
           cerr << "enable.idempotence set failed: " << errstr << endl;
           pl_log(ERR, "enable.idempotence set failed: %s", errstr.c_str());
           return PF_RET_FAILURE;
        }
    }

    if (app_cb)
    {
        m_app_cb = app_cb;
    }
        
    return PF_RET_SUCCESS;
}


int kafka_producer::configure(string brokers, string topics, HANDLE_MQ_SEND_ERR app_cb, HANDLE_MQ_EVENT_CB event_cb, bool idempotent)
{
    string errstr;
    
    if (configure_basic(brokers, topics) < 0)
    {
        return PF_RET_FAILURE;
    }

    if (configure_cb() < 0)
    {
        return PF_RET_FAILURE;
    }
    
    if (idempotent)
    {
        /* Enable the idempotent producer */
        if (m_conf->set("enable.idempotence", "true", errstr) != RdKafka::Conf::CONF_OK) 
        {
            //cerr << "enable.idempotence set failed: " << errstr << endl;
            pl_log(ERR, "enable.idempotence set failed: %s ", errstr.c_str());
            return PF_RET_FAILURE;
        }
    }

    if (app_cb)
    {
        m_app_cb = app_cb;
    }

    if (event_cb)
    {
        m_event_cb = event_cb;
    }

    return PF_RET_SUCCESS;
}

S32 kafka_producer::send_action(S32 partition, S32 flag, void *payload, S32 len, const void* key, S32 key_len, S64 timestamp, RdKafka::Headers* headers, void* msg_opaque)
{
    return rdkafak_send_interface(m_topics, partition, flag, payload, len , key, key_len, timestamp, headers, msg_opaque);
}

S32 kafka_producer::rdkafak_send_interface(std::string& topic, S32 partition, S32 flag, void *payload, S32 len, const void* key, S32 key_len, S64 timestamp, RdKafka::Headers* headers, void* msg_opaque)
{
    /*
     * Send/Produce message.
     * This is an asynchronous call, on success it will only
     * enqueue the message on the internal producer queue.
     * The actual delivery attempts to the broker are handled
     * by background threads.
     * The previously registered delivery report callback
     * is used to signal back to the application when the message
     * has been delivered (or failed permanently after retries).
     */
    retry:
    RdKafka::ErrorCode err = m_producer->produce(
                        /* Topic name */
                        topic,
                        /* Any Partition: the builtin partitioner will be
                         * used to assign the message to a topic based
                         * on the message key, or random partition if
                         * the key is not set. */
                        partition, //RdKafka::Topic::PARTITION_UA,
                        
                        /* Make a copy of the value */
                        flag,//RdKafka::Producer::RK_MSG_COPY /* Copy payload */,
                        /* Value */
                        payload, len,//pconst_cast<signed char *>(msg), msg_len,
                        /* Key */
                        key, key_len,//key.c_str(), key.size(),
                        /* Timestamp (defaults to current time) */
                        timestamp, //0,
                        /* Message headers, if any */
                        headers,
                        /* Per-message opaque value passed to
                         * delivery report */
                        msg_opaque);

    if (err != RdKafka::ERR_NO_ERROR)
    {
        //cerr << "% Failed to produce to topic " << m_topics << ": " <<RdKafka::err2str(err) << endl;
        pl_log(WARN, " Failed to produce to topic [ %s], reason :%s ", m_topics.c_str(), RdKafka::err2str(err).c_str());
        // Headers are automatically deleted if produce success, or U should delete it by yourself
        if (headers)
        {
            delete headers;
            headers = NULL;
        }
        PS_CPlus(CM_PES, CMPES_ID_RDKAFKA_PRODUCER_SEND_FAIL);

        if (err == RdKafka::ERR__QUEUE_FULL) 
        {
            /* If the internal queue is full, wait for
             * messages to be delivered and then retry.
             * The internal queue represents both
             * messages to be sent and messages that have
             * been sent or failed, awaiting their
             * delivery report callback to be called.
             *
             * The internal queue is limited by the
             * configuration property
             * queue.buffering.max.messages */
            m_producer->poll(1000/*block for max 1000ms*/);
            PS_CPlus(CM_PES, CMPES_ID_RDKAFKA_PRODUCER_QUEUE_FULL_FAIL);
            goto retry;
        }
    } 
    else 
    {
        #ifdef DEBUG
        cerr << "% Enqueued message ok" << " for topic " << m_topics << endl;
        #endif
        PS_CPlus(CM_COM, CMCOM_ID_RDKAFKA_PRODUCER_SEND_CNT);
        PS_CPlusV(CM_COM, CMCOM_ID_RDKAFKA_PRODUCER_SEND_TOTAL_SIZE, len);
    }

    /* A producer application should continually serve
     * the delivery report queue by calling poll()
     * at frequent intervals.
     * Either put the poll call in your main loop, or in a
     * dedicated thread, or call it after every produce() call.
     * Just make sure that poll() is still called
     * during periods where you are not producing any messages
     * to make sure previously produced messages have their
     * delivery report callback served (and any other callbacks
     * you register). */
    if (m_sync_mode)
    {
        m_producer->poll(0);

        while (!m_do_not_send && m_producer->outq_len() > 0) 
        {
            #ifdef RD_PRODUCER_DEBUG
            cerr << "Waiting for " << m_producer->outq_len() << endl;
            #endif
            m_producer->poll(1000);
        }
    }
    else // async mode
    {
        if (m_send_with_poll)
        {
            m_producer->poll(0);
        }
    
        if (m_send_with_flush)
        {
            if (++m_flush_cnt == m_flush_interval)
            {
                m_flush_cnt = 0;
                m_producer->flush(100); //max 100ms
            }
        }
    }

    return PF_RET_SUCCESS;
}

/*
Message =>
        Length => varint
        Attributes => int8
        TimestampDelta => varlong
        OffsetDelta => varint
        KeyLen => varint
        Key => data
        ValueLen => varint
        Value => data
        Headers => [Header] <------------ NEW Added Array of headers
         
Header =>
        Key => string (utf8) <------------------------------- NEW UTF8 encoded string (uses varint length)
        Value => bytes  <------------------------------------ NEW header value as data (uses varint length)
 
 
*/
S32 kafka_producer::send(const S8* msg, S32 msg_len, 
                         const S8* header_name, S32 name_len,
                         const S8* header_value, S32 value_len,
                          S8* key, S32 key_len, 
                         S32 slPartition)
{
    RdKafka::Headers *headers = NULL;
  
    S8* send_key = key;

    //　参数范围检测，检查消息长度 / 消息头长度　/ 消息头的值的长度　/ key 的长度
    if ((msg_len > PRODUCER_MAX_SEND_LEN)
     || (name_len > PRODUCER_MAX_HEADER_LEN)
     || (value_len > PRODUCER_MAX_HEADER_VAL_LEN)
     || (key_len > PRODUCER_MAX_KEY_LEN))    
    {
        return -ERR_PARA;
    }
    
    // check if user provide a fixed key
    if ((NULL == key) && (m_custom_key.length() > 0))
    {
        send_key = (S8 *)m_custom_key.c_str();
        key_len = m_custom_key.length();
    }

    if (header_name && name_len > 0)
    {
        headers = RdKafka::Headers::create();
        
        if (!headers)
        {
            //cerr << " Can NOT create headers !" << endl;
            pl_log(ERR, " Can NOT create headers !");
            return PF_RET_FAILURE;
        }

        if (headers->add((const CHAR*)header_name, (const CHAR*)header_value)) 
        {
            //cerr << " Add header failed " << endl;
            pl_log(ERR, " Add header failed ");
            return PF_RET_FAILURE;
        }
    }

    return send_action( (slPartition < 0 ? RdKafka::Topic::PARTITION_UA: slPartition),
                        RdKafka::Producer::RK_MSG_COPY /* Copy payload */,
                        /* Value */
                        const_cast<S8 *>(msg), msg_len,
                        /* Key */
                        send_key, key_len,
                        /* Timestamp (defaults to current time) */
                        0,
                        /* Message headers, if any */
                        headers,
                        /* Per-message opaque value passed to
                         * delivery report */
                        NULL);
}

S32 kafka_producer::send(const S8* msg, S32 msg_len, 
                         const S8* header_name, S32 name_len,
                         const S8* header_value, S32 value_len,
                         const S8* key, S32 key_len)
{
    return send(msg, msg_len, header_name, name_len, header_value, value_len, (S8*)key, key_len, -1);
}
                         
S32 kafka_producer::send(string &msg, string &header_name, string &header_value, string &key)
{
    return send((const S8*)msg.c_str(), msg.size(),
                (const S8*)header_name.c_str(), header_name.size(),
                (const S8*)header_value.c_str(), header_value.size(),
                (const S8*)key.c_str(), key.size());
}

S32 kafka_producer::send(string &msg, string &header_name, string &header_value)
{
    return send((const S8*)msg.c_str(), msg.size(),
                (const S8*)header_name.c_str(), header_name.size(),
                (const S8*)header_value.c_str(), header_value.size(),
                NULL, 0);
}

S32 kafka_producer::send(const S8* msg, S32 msg_len, 
                         const S8* header_name, S32 name_len,
                         const S8* header_value, S32 value_len)
{
    return send(msg, msg_len,
                header_name, name_len,
                header_value, value_len,
                NULL, 0);            
}    

#ifdef PHASE_2
S32 kafka_producer::send(string msg)
{
    return send((const S8*)msg.c_str(), msg.size(),
                NULL, 0,
                NULL, 0,
                NULL, 0);
}

S32 kafka_producer::send(string msg, string key)
{
    return send(msg.c_str(), msg.size(),
                NULL, 0,
                NULL, 0,
                key.c_str(), key.size());
}

S32 kafka_producer::send(const S8* msg, S32 msg_len)
{
    return send(msg, msg_len, 
                NULL, 0,
                NULL, 0,
                NULL, 0);
}

S32 kafka_producer::send(const S8* msg, S32 msg_len,
                         const S8* key, S32 key_len)
{
    return send(msg, msg_len,
                NULL, 0,
                NULL, 0,
                key, key_len);      
}

S32 kafka_producer::send(string msg, string header_name, string header_value, string key, S32 slPartition)
{
    return send(msg.c_str(), msg.size(),
                header_name.c_str(), header_name.size(),
                header_value.c_str(), header_value.size(),
                key.c_str(), key.size(),
                slPartition);
}

S32 kafka_producer::send(string msg, S32 slPartition)
{
    return send(msg.c_str(), msg.size(),
                NULL, 0,
                NULL, 0,
                NULL, 0,
                slPartition);
}

S32 kafka_producer::send(string msg, string header_name, string header_value, S32 slPartition)
{
    return send(msg.c_str(), msg.size(),
                header_name.c_str(), header_name.size(),
                header_value.c_str(), header_value.size(),
                NULL, 0,
                slPartition);
}

S32 kafka_producer::send(string msg, string key, S32 slPartition)
{
    return send(msg.c_str(), msg.size(),
                NULL, 0,
                NULL, 0,
                key.c_str(), key.size(),
                slPartition);
}
                    
S32 kafka_producer::send(const S8* msg, S32 msg_len, S32 slPartition)
{
    return send(msg, msg_len, 
                NULL, 0,
                NULL, 0,
                NULL, 0,
                slPartition);
}
    
S32 kafka_producer::send(const S8* msg, S32 msg_len, 
                         const S8* header_name, S32 name_len,
                         const S8* header_value, S32 value_len, 
                         S32 slPartition)
{
    return send(msg, msg_len,
                header_name, name_len,
                header_value, value_len,
                NULL, 0,
                slPartition);            
}                            
                         
S32 kafka_producer::send(const S8* msg, S32 msg_len,
                         const S8* key, S32 key_len,
                         S32 slPartition)
{
    return send(msg, msg_len,
                NULL, 0,
                NULL, 0,
                key, key_len,
                slPartition);      
}
#endif

void kafka_producer::flush()
{
    /* Wait for final messages to be delivered or fail.
     * flush() is an abstraction over poll() which
     * waits for all messages to be delivered. */
    #ifdef RD_PRODUCER_DEBUG
    cerr << "% Flushing final messages..." << endl;
    #endif
    m_producer->flush(10*1000 /* wait for max 10 seconds */);
}

void kafka_producer::dr_cb (RdKafka::Message &message) 
{ 
    PS_CPlus(CM_COM, CMCOM_ID_RDKAFKA_PRODUCER_DR_CB_CNT);
    if (m_app_cb)
    {
        m_app_cb(message.err());
        return;
    }

    
    /* If message.err() is non-zero the message delivery failed permanently
     * for the message. */
    if (message.err())
    {
        m_do_not_send = true;
        //cerr << "% Message delivery failed: " << message.errstr() << endl;
        pl_log(WARN, "Message delivery failed: %s", message.errstr().c_str());
        PS_CPlus(CM_PES, CMPES_ID_RDKAFKA_PRODUCER_CALLBACK_FAIL);
    }
    else
    {
        #ifdef RD_PRODUCER_DEBUG
        cerr << "% Message delivered to topic " << message.topic_name() <<" [" << message.partition() 
        << "] at offset " << message.offset() << endl;
        #endif
        m_do_not_send = false;
    }
}

int kafka_producer::print_producer_throughput(std::string str_stats)
{  
    if (pl_debug_rdproducer_stats_full_log_on)
    {
        pl_log(INF, "--- rd producer statistics: %s", str_stats.c_str());
        return PF_RET_SUCCESS;
    }
    
     m_status = str_stats;

/*
    string::size_type p;
    unsigned long long txmsgs;
    std::string topic_name;
    std::string producer_name;
    std::string str_tmp; 

    if ((p = str_stats.find("\"topic\":\"")) != std::string::npos)
    {
        str_tmp = str_stats.substr(p + strlen("\"topic\":\""));
        topic_name = str_tmp.substr(0, str_tmp.find("\""));
    }

    if ((p = str_stats.find("\"client_id\":")) != std::string::npos)
    {
        str_tmp = str_stats.substr(p + strlen("\"client_id\":"));
        producer_name = str_tmp.substr(0, str_tmp.find(","));
    }

    // 1. Total send messages
    if((p = str_stats.rfind("\"txmsgs\":")) != 0)
    {
        txmsgs = strtoull(str_stats.c_str() + p + strlen("\"txmsgs\":"), NULL, 10);
        pl_log(INF, "%s[ %s]: txmsgs ( %lld)", producer_name.c_str(), topic_name.c_str(), txmsgs);
    }

    // 2. Total send bytes
    unsigned long long txmsg_bytes;
    if((p = str_stats.rfind("\"txmsg_bytes\":")) != 0)
    {
        txmsg_bytes = strtoull(str_stats.c_str() + p + strlen("\"txmsg_bytes\":"), NULL, 10);
        pl_log(INF, "%s[ %s]: txmsg_bytes ( %lld)", producer_name.c_str(), topic_name.c_str(), txmsg_bytes);
    }
*/	   
    return PF_RET_SUCCESS;
}

void pf_producer_rdkafka_throughput(void)
{
    int i;
    for(i=0; i<ulProducerNum; i++)
    {
        if(pKafkaProducer[i])
        {
            pKafkaProducer[i]->print_producer_static();
        }
    }
	
    pl_log(INF, "producer_send_cnt=%d producer_send_total_size=%d producer_event_cb_cnt=%d producer_rd_cb_cnt=%d", \
                 PS_CGet(CM_COM, CMCOM_ID_RDKAFKA_PRODUCER_SEND_CNT), \
                 PS_CGet(CM_COM, CMCOM_ID_RDKAFKA_PRODUCER_SEND_TOTAL_SIZE), \
                 PS_CGet(CM_COM, CMCOM_ID_RDKAFKA_PRODUCER_EVENT_CB_CNT), \
                 PS_CGet(CM_COM, CMCOM_ID_RDKAFKA_PRODUCER_DR_CB_CNT));
}

void kafka_producer::event_cb (RdKafka::Event &event)
{
    PS_CPlus(CM_COM, CMCOM_ID_RDKAFKA_PRODUCER_EVENT_CB_CNT);

    switch (event.type())
    {
        case RdKafka::Event::EVENT_ERROR:
            m_event_error_cnt++;
            //cerr << "ERROR (" << RdKafka::err2str(event.err()) << "): " << event.str() << endl;
            pl_log(ERR, "ERROR! %s ", event.str().c_str());

            if (event.err() == RdKafka::ERR__ALL_BROKERS_DOWN)
            {
                //todo: should info APP
                //cerr << " ERR__ALL_BROKERS_DOWN !" << endl;
                pl_log(ERR, "ERR__ALL_BROKERS_DOWN!");
            }
            break;

        case RdKafka::Event::EVENT_STATS:
            m_event_stats_cnt++;
            //cerr << "\"Producer statistics\": " << event.str() << endl;
            print_producer_throughput(event.str());
            break;

        case RdKafka::Event::EVENT_LOG:
            m_event_log_cnt++;
            //fprintf(stderr, "LOG-%i-%s: %s\n", event.severity(), event.fac().c_str(), event.str().c_str());
            pl_log(ERR, "LOG-%i-%s: %s", event.severity(), event.fac().c_str(), event.str().c_str());
            break;

        default:
            m_default_cnt++;
            //cerr << "EVENT " << event.type() << " (" << RdKafka::err2str(event.err()) << "): " << event.str() << endl;
            pl_log(ERR, "EVENT %s ( %s ): %s", event.type(), RdKafka::err2str(event.err()).c_str(), event.str().c_str());
            break;
    }
    
    if (m_event_cb)
    {
        m_event_cb(event);
        return;
    }

}

S32 kafka_producer::set_config(string config_name, string config_value)
{
    CHAR errbuf[ERRBUFMAX] = {0};
    rd_kafka_conf_t *rk_conf = m_conf->c_ptr_global();
 
    if (0 == config_name.compare(SEND_WITH_POLL))
    {
        m_send_with_poll = atoi(config_value.c_str());
        //cout << " user set send_with_poll to " << m_send_with_poll << endl;
        pl_log(INF, "user set send_with_poll to %d", m_send_with_poll);
        return PF_RET_SUCCESS;
    }

    if (0 == config_name.compare(SEND_WITH_FLUSH))
    {
        m_send_with_flush = atoi(config_value.c_str());
        //cout << " user set m_send_with_flush to " << m_send_with_flush << endl;
        pl_log(INF, "user set m_send_with_flush to %d", m_send_with_flush);
        return PF_RET_SUCCESS;
    }

    if (0 == config_name.compare(SEND_FLUSH_INT))
    {
        m_flush_interval = atoi(config_value.c_str());
        //cout << " user set m_flush_interval to " << m_flush_interval << endl;
        pl_log(INF, "user set m_flush_interval to %d", m_flush_interval);
        if (m_flush_interval <= 0)
        {
            //cout << " user set a wrong number " << endl;
            pl_log(INF, "user set a wrong number");
            return PF_RET_FAILURE;
        }
        return PF_RET_SUCCESS;
    }
    
    // "send.mode" is NOT a rdkafka's internal config, should be handled differently
    if (0 == config_name.compare(SEND_MODE))
    {
        if (0 == config_value.compare(MODE_ASYNC))
        {
            m_sync_mode = false;
            return PF_RET_SUCCESS;
        }
        
        if (0 == config_value.compare(MODE_SYNC))
        {
            m_sync_mode = true;
            return PF_RET_SUCCESS;
        }
        
        //cerr << "User's parameter is NOT supported, available value is" << MODE_ASYNC << " or " << MODE_SYNC << endl;
        pl_log(ERR, " User's parameter is NOT supported, available value is %s or %s", MODE_ASYNC, MODE_SYNC);
        return PF_RET_FAILURE;
    }

    if (0 == config_name.compare(FIXED_KEY))
    {
        m_custom_key = config_value;
        
        //cout << "User SET a fixed KEY" << endl;
        pl_log(INF, "User SET a fixed KEY");

        return PF_RET_SUCCESS;
    } 

    if (rd_kafka_conf_set(rk_conf, config_name.c_str(), config_value.c_str(), errbuf, sizeof(errbuf)))
    {
        //cerr << "fail to set " << config_name << endl;
        //cout << errbuf << endl;
        pl_log(ERR, "pl-rdkafka-producer: fail to set %s", errbuf);
        return PF_RET_FAILURE;
    }
    else
    {
        //cout << "set " << config_name << " ok !" << endl;
        pl_log(INF, "pl-rdkafka-producer: set %s ok", errbuf);
    }
    
    return PF_RET_SUCCESS;
}
#ifdef PHASE_2
S64 kafka_producer::get_offset_by_timestamp(S32 slPartition, S64 timestamp)
{
    std::vector<RdKafka::TopicPartition*> query_parts;
    
    query_parts.push_back(RdKafka::TopicPartition::create(m_topics, slPartition, timestamp)); // in milisecond: e.g 1618019554525
    
    RdKafka::ErrorCode err = m_producer->offsetsForTimes(query_parts, 5000);
 
    if (err != RdKafka::ERR_NO_ERROR)
    {
        //cout << "err is " << RdKafka::err2str(err) << endl;
        pl_log(WARN, "get_offset_by_timestamp: err is %s", RdKafka::err2str(err).c_str());
        return PF_RET_FAILURE;
    }
    // If offset is not exist, for example, topic retention time is up, then the messages deleted by kafka itself, 
    // the err code is SUCCESS, but return offset is -1 !
    return query_parts[0]->offset();
}
#endif

S32 kafka_producer::set_config_by_file(string file)
{
    FILE* fp = fopen(file.c_str(), "r");
    
    if (fp)
    {
        char content[CONTENTMAX] = {0};
        char para[PARAMAX] = {0};
        char settings[SETTINGSMAX] = {0};
        int index = 0;
        int j = 0;
        //cout << "start to process settings " << endl;
        pl_log(INF, "start to process settings ");
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
                        //cout << "get settings : " << settings << endl;
                        string tag;
                        string val;
                        tag = strtok(settings, "=");
                        val = strtok(NULL, "=");
                        //cout << "tag and val : " << tag << " " << val << endl;
                        pl_log(INF, "pl-rdkafka-producer-config-file: %s : %s ", tag.c_str(), val.c_str());
                        //　设置参数
                        set_config(tag, val);
                        
                        break;
                    }
                }
            }
            
        }   
    }
    
    if (fp != NULL)
    {
        fclose(fp);
    }
    return PF_RET_SUCCESS;
}


S32 kafka_producer::set_statistics_inverval(S32 i_second)
{
    if (i_second < 0)
    {
        pl_log(WARN, "statistics inverval should bigger or equal to 0!");
        return PF_RET_FAILURE;
    }

    m_statistics_interval = i_second * 1000; // convert second to mili-second

    if (m_statistics_interval == 0)
    {
        m_statistics_on = 0;
    }

    return PF_RET_SUCCESS;
}

S32 kafka_producer::set_self_name(std::string name)
{
    string errstr;

    if (m_conf->set("client.id", name, errstr) != RdKafka::Conf::CONF_OK) 
    {
        std::cerr << errstr << std::endl;
        pl_log(ERR, "Set self name failed");
        return PF_RET_FAILURE;
    }

    return PF_RET_SUCCESS;
}


S32 kafka_producer::send(const S8* msg, S32 msg_len, 
                         const std::vector <PL_KAFKA_HEADER> & header_vector,
                         const std::string& key)

{
    RdKafka::Headers *headers = NULL;
    headers = RdKafka::Headers::create();

    for (unsigned int i = 0; i < header_vector.size(); i++)
    {    
        if (headers->add(header_vector[i].header_name, (char*)(header_vector[i].value), header_vector[i].vlen)) 
        {
             //cerr << " Add header failed " << endl;
             pl_log(ERR, "Add header failed ");
             return PF_RET_FAILURE;
        }
    }
    return send_action(RdKafka::Topic::PARTITION_UA,
                        /* Make a copy of the value */
                        RdKafka::Producer::RK_MSG_COPY /* Copy payload */,
                        /* Value */
                        const_cast<signed char *>(msg), msg_len,
                        /* Key */
                        key.c_str(), key.size(),
                        /* Timestamp (defaults to current time) */
                        0,
                        /* Message headers, if any */
                        headers,
                        /* Per-message opaque value passed to
                         * delivery report */
                        NULL);
}

S32 kafka_producer::send(std::string& topic,
                         S32 partition,               
                         const S8* msg, S32 msg_len, 
                         const std::vector <PL_KAFKA_HEADER> & header_vector,
                         const std::string& key)
{
    RdKafka::Headers *headers = NULL;
    headers = RdKafka::Headers::create();

    for (unsigned int i = 0; i < header_vector.size(); i++)
    {    
        if (headers->add(header_vector[i].header_name, (char*)(header_vector[i].value), header_vector[i].vlen)) 
        {
             cerr << " Add header failed " << endl;
             return -1;
        }
    }
    return rdkafak_send_interface(topic,
                        partition,

                        /* Make a copy of the value */
                        RdKafka::Producer::RK_MSG_COPY /* Copy payload */,
                        /* Value */
                        const_cast<signed char *>(msg), msg_len,
                        /* Key */
                        key.c_str(), key.size(),
                        /* Timestamp (defaults to current time) */
                        0,
                        /* Message headers, if any */
                        headers,
                        /* Per-message opaque value passed to
                         * delivery report */
                        NULL);
}


