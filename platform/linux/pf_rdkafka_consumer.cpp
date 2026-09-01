
#define THIS_MODULE PLATFORM_EX

#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <csignal>
#include <cstring>
#include <atomic>
#include <thread>
#include <ctime>
#include <iomanip>
#include <sstream>

/*
 * Typically include path in a real application would be
 * #include <librdkafka/rdkafkacpp.h>
 */
#include <sys/time.h>
#include <librdkafka/rdkafkacpp.h>
#include <librdkafka/rdkafka.h>

#include "pl_type.h"
#include "pl.h"
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
 
// polling time in milisecond
#define MAX_POLL_TIME 60*1000 
#define MIN_POLL_TIME 500
#define DEFAULT_POLL_TIME 1000 
#define FEATURES_MAX 256
#define ERRBUF_MAX 512
#define IVL_STR_MAX 128
#define PKAFKA_CONSUMER_MAX 32
#define CONTENT_MAX 2048
#define PARA_MAX 256
#define SETTINGS_MAX 256

kafka_consumer* pKafkaConsumer[PKAFKA_CONSUMER_MAX] = {0};
U32 ulConsumerNum = 0;
PF_MUTEX_T stKafkaConsumerMutex = PTHREAD_MUTEX_INITIALIZER;

static S32 pl_debug_rdconsumer_stats_full_log_on = 0;

kafka_consumer::kafka_consumer()
{
    // Should we set default values here?
    m_execute = new (std::atomic<bool>);
	BOOL bFlag = FALSE;
 
    //mutex
    PF_MUTEX_LOCK(&stKafkaConsumerMutex); 
    if(ulConsumerNum < PKAFKA_CONSUMER_MAX)
    {
        pKafkaConsumer[ulConsumerNum] = this;
        ulConsumerNum++;
        bFlag = TRUE;
    }
    PF_MUTEX_UNLOCK(&stKafkaConsumerMutex);

    if(bFlag)
    {
        pl_log(WARN, "consumer 0x%x number=%d", this, ulConsumerNum);
    }
    else
    {
        pl_log(FATAL, "consumer 0x%x exceed PKAFKA_CONSUMER_MAX %d", this, PKAFKA_CONSUMER_MAX);
    }  

}
    
kafka_consumer::~kafka_consumer()
{
    /**
     * Stop consumer
     */
    int iFlag = -1;
    m_flag = FALSE;

    if (m_kconsumer)
    {
        if ( m_execute.load(std::memory_order_acquire) )
        {
            m_execute.store(false, std::memory_order_release);

            if (m_thd.joinable())
            {
                m_thd.join();
            }
        }
        
        m_kconsumer->close();
    }
    
    if (m_consumer)
    {    
        m_consumer->stop(m_topic, 0);
        m_consumer->poll(1000);
        
        delete m_consumer;
    }
    
    if (m_conf) 
    {
        delete m_conf;
        m_conf = NULL;
    }
    
    if (m_tconf)
    {
        delete m_tconf;
        m_tconf = NULL;
    }

    if (m_topic)
    {
        delete m_topic;
        m_topic = 0;
    }
    
    RdKafka::wait_destroyed(5000);
    
    //mutex
    PF_MUTEX_LOCK(&stKafkaConsumerMutex);
    U32 i = 0;
    for(i = 0; i < ulConsumerNum; i++)
    {
        if(this == pKafkaConsumer[i])
        {
            pKafkaConsumer[i] = NULL;
			iFlag = i;
            break;
        }
    }
    
    for(; i < ulConsumerNum-1; i++)
    {
        pKafkaConsumer[i] = pKafkaConsumer[i+1];
    }          
    
    if(i>=0)
    {
        //find the consumer address
        ulConsumerNum--;
    }          
	PF_MUTEX_UNLOCK(&stKafkaConsumerMutex);

    pl_log(FATAL, "rdkafka consumer good bye 0x%x, Flag=%d, Num=%d", this, iFlag, ulConsumerNum);
}

void kafka_consumer::print_consumer_static(void)
{ 
    pl_log(INF, "m_event_error_cnt=%d m_event_stats_cnt=%d m_event_log_cnt=%d m_default_cnt=%d m_stats=\r\n%s",\
                m_event_error_cnt, m_event_stats_cnt, m_event_log_cnt, m_default_cnt, m_stats.c_str());

    show_config();
}

S32 kafka_consumer::configure_basic(string brokers, string topics)
{
    m_brokers = brokers;
    m_topics = topics;    
    
    string errstr;
    
    /**
     * Create configuration object
     */
    m_conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
         
    if (!m_conf)
    {
        //cerr << "Can not create RD kafka configue !" << endl;
        pl_log(ERR, "Can not create RD kafka configue !");
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

    if (m_statistics_on)
    {

        rd_kafka_conf_t *rk_conf = m_conf->c_ptr_global();

        char errbuf[ERRBUF_MAX] = {0};

        char ivl_str[IVL_STR_MAX] = {0};

        sprintf(ivl_str, "%d", m_statistics_interval);

    
        if (rd_kafka_conf_set(rk_conf, "statistics.interval.ms", ivl_str, errbuf, sizeof(errbuf)) != RD_KAFKA_CONF_OK)
        {
            //cout << "error is " << errbuf << endl;
            pl_log(ERR, "set statistics.interval.ms failed %s", errbuf);
            return PF_RET_FAILURE;
        }
    } 
               
    return PF_RET_SUCCESS;
}

S32 kafka_consumer::configure_cb(BOOL kconsumer, APP_MSG_HANDLER app_msg_handler, APP_EVENT_HANDLER app_event_recv_function)
{
    PS_CPlus(CM_COM, CMCOM_ID_RDKAFKA_CONSUMER_CONFIGURE_EVENT_CB_CNT);

    string errstr;
     
    // set event call back
    if (m_conf->set("event_cb", (EventCb*)this, errstr) != RdKafka::Conf::CONF_OK) 
    {
        //std::cerr << errstr << std::endl;
        pl_log(ERR, "set event_cb failed %s", errstr.c_str());
        return PF_RET_FAILURE;
    }
    
    m_app_event_recv_function = app_event_recv_function;
    
    m_app_msg_handler = app_msg_handler;
    
    return PF_RET_SUCCESS;
}

S32 kafka_consumer::configure_cb(BOOL kconsumer, APP_MSG_HANDLER app_msg_handler, APP_FULL_EVENT_HANDLER app_event_recv_function)
{
    PS_CPlus(CM_COM, CMCOM_ID_RDKAFKA_CONSUMER_CONFIGURE_FULL_EVENT_CB_CNT);

    string errstr;
     
    // set event call back
    if (m_conf->set("event_cb", (EventCb*)this, errstr) != RdKafka::Conf::CONF_OK) 
    {
        //std::cerr << errstr << std::endl;
        pl_log(ERR, "set event_cb failed %s", errstr.c_str());
        return PF_RET_FAILURE;
    }
                                            
    m_app_full_event_cb = app_event_recv_function;
                                 
    m_app_msg_handler = app_msg_handler;
         
    return PF_RET_SUCCESS;
}

S32 kafka_consumer::configure_acl(string security_protocol, string sasl_mechanism, string user_name, string passwd)
{
    rd_kafka_conf_t *rk_conf = m_conf->c_ptr_global();

    if (rk_conf)
    {   
        CHAR features[FEATURES_MAX];
        size_t fsize = sizeof(features);

        //printf("librdkafka %s\n", rd_kafka_version_str());
        pl_log(INF, "librdkafka %s", rd_kafka_version_str());
       
        if (rd_kafka_conf_get(rk_conf, "builtin.features", features, &fsize) != RD_KAFKA_CONF_OK) 
        {
            //fprintf(stderr, "conf_get failed\n");
            pl_log(ERR, "conf_get failed %s", stderr);
            return PF_RET_FAILURE;
        }

        //printf("builtin.features: %s\n", features);
        pl_log(INF, "builtin.features: %s", features);
        
        CHAR errbuf[ERRBUF_MAX] = {0};
        
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
        //cerr << " can  not set ACL " << endl;
        pl_log(ERR, "can  not set ACL");
        return PF_RET_FAILURE;
    }
    
    return PF_RET_SUCCESS;
}

#ifdef PHASE_2
S32 kafka_consumer::configure(string brokers, string topics, S32 slPartition, BOOL use_callback, 
                              APP_MSG_HANDLER app_msg_handler, APP_EVENT_HANDLER app_event_recv_function)
{    
    m_use_kconsumer = false;
    m_use_callback = use_callback;
    m_partition = slPartition;
    m_topics = topics;
     
    string errstr;
    
    if (configure_basic(brokers, topics))
    {
        return PF_RET_FAILURE;
    }
          
    if (configure_cb(false, app_msg_handler, app_event_recv_function) < 0)
    {
        return PF_RET_FAILURE;
    }
    
    return PF_RET_SUCCESS;
}
#endif
/**
 * kconsumer needs group id
 */
S32 kafka_consumer::configure(string brokers, string topics, string group_id,
                               APP_MSG_HANDLER app_msg_handler, APP_EVENT_HANDLER app_event_recv_function)
{    
    string errstr;
    
    m_use_kconsumer = true;

    m_topics = topics;
        
    if (configure_basic(brokers, topics))
    {
        return PF_RET_FAILURE;
    }
        
    if (m_conf->set("group.id",  group_id, errstr) != RdKafka::Conf::CONF_OK) 
    {
        //cerr << errstr << endl;
        pl_log(ERR, "set group id failed: %s", errstr.c_str());
        return PF_RET_FAILURE;
    }
    
    if (configure_cb(true, app_msg_handler, app_event_recv_function) < 0)
    {
        return PF_RET_FAILURE;
    }
   
     if(m_conf->set("fetch.wait.max.ms", "5", errstr) != RdKafka::Conf::CONF_OK)
    {
       //cerr << "fetch.wait.max.ms failed: " << errstr << endl;
       pl_log(ERR, "fetch.wait.max.ms failed: %s", errstr.c_str());
       return PF_RET_FAILURE;
    }
    else
    {
        //cout << "set fetch.wait.max.ms ok !" << endl;
        pl_log(INF, "set fetch.wait.max.ms ok !");
    }
    
    
     if(m_conf->set("fetch.error.backoff.ms", "10", errstr) != RdKafka::Conf::CONF_OK)
    {
       //cerr << "fetch.wait.max.ms failed: " << errstr << endl;
       pl_log(ERR, "fetch.wait.max.ms failed: %s", errstr.c_str());	
       return PF_RET_FAILURE;
    }
    else
    {
        //cout << "set fetch.error.backoff.ms ok !" << endl;
        pl_log(INF, "set fetch.wait.max.ms ok !");
    }
    return PF_RET_SUCCESS;
}

S32 kafka_consumer::configure(string brokers, string topics, string group_id,
                               APP_MSG_HANDLER app_msg_handler, APP_FULL_EVENT_HANDLER app_event_recv_function)
{    
    string errstr;
    
    m_use_kconsumer = true;

    m_topics = topics;
        
    if (configure_basic(brokers, topics))
    {
        return PF_RET_FAILURE;
    }
        
    if (m_conf->set("group.id",  group_id, errstr) != RdKafka::Conf::CONF_OK) 
    {
        //cerr << errstr << endl;
        pl_log(ERR, "set group id failed: %s", errstr.c_str());
        return PF_RET_FAILURE;
    }
    
    if (configure_cb(true, app_msg_handler, app_event_recv_function) < 0)
    {
        return PF_RET_FAILURE;
    }
   
    if(m_conf->set("fetch.wait.max.ms", "5", errstr) != RdKafka::Conf::CONF_OK)
    {
        //cerr << "fetch.wait.max.ms failed: " << errstr << endl;
        pl_log(ERR, "fetch.wait.max.ms failed: %s", errstr.c_str());
        return PF_RET_FAILURE;
    }
    else
    {
        //cout << "set fetch.wait.max.ms ok !" << endl;
        pl_log(INF, "set fetch.wait.max.ms ok !");
    }
    
    
    if(m_conf->set("fetch.error.backoff.ms", "10", errstr) != RdKafka::Conf::CONF_OK)
    {
        //cerr << "fetch.wait.max.ms failed: " << errstr << endl;
        pl_log(ERR, "fetch.wait.max.ms failed: %s", errstr.c_str());
        return PF_RET_FAILURE;
    }
    else
    {
        //cout << "set fetch.error.backoff.ms ok !" << endl;
        pl_log(INF, "set fetch.wait.max.ms ok !");
    }
    return PF_RET_SUCCESS;
}


S32 kafka_consumer::create_normal_consumer()
{
    string errstr;
    
    /*
     * Create consumer using accumulated global configuration.
     */
    m_consumer = RdKafka::Consumer::create(m_conf, errstr);
    m_flag = FALSE;
    
    if (!m_consumer) 
    {
        //cerr << "Failed to create consumer: " << errstr << endl;
        pl_log(ERR, "Failed to create consumer: %s", errstr.c_str());
        return PF_RET_FAILURE;
    }
    
    m_tconf = RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC);
    
    if (!m_tconf)
    {
        //cerr << "Can not create RD kafka topic configue !" << endl;
        pl_log(ERR, "Can not create RD kafka topic configue !");
        return PF_RET_FAILURE;
    }
     
    if (m_conf->set("default_topic_conf", m_tconf, errstr) != RdKafka::Conf::CONF_OK)
    {
        //cerr << " set default_topic_conf failed !" << errstr << endl;
        pl_log(ERR, "set default_topic_conf failed : %s", errstr.c_str());
        return PF_RET_FAILURE;
    }

    /**
     * Create topic handle.  m_topic is a handle, m_topics is a string
     */
             
    m_topic = RdKafka::Topic::create(m_consumer, m_topics, m_tconf, errstr);

    if (!m_topic)
    {
        //cerr << " create m_topic failed !" << endl;
        pl_log(ERR, " create m_topic failed !");
        return PF_RET_FAILURE;
    }
    
        // start to read:
    RdKafka::ErrorCode resp = m_consumer->start(m_topic, 0, m_start_offset);
    
    if (resp != RdKafka::ERR_NO_ERROR) 
    {
        //cerr << "Failed to start consumer: " << RdKafka::err2str(resp) << endl;
        pl_log(ERR, "Failed to start consumer: %s ", RdKafka::err2str(resp).c_str());
        return PF_RET_FAILURE;
    }
    
    m_flag = TRUE;
    return PF_RET_SUCCESS;
}

S32 kafka_consumer::create_kconsumer(void)
{
    string errstr;

    m_flag = FALSE;
    
    m_kconsumer = RdKafka::KafkaConsumer::create(m_conf, errstr);
    
    if (!m_kconsumer)
    {
        //cerr << "Failed to create kafka consumer: " << errstr << endl;
        pl_log(ERR, "Failed to create kafka consumer: %s", errstr.c_str());
        return PF_RET_FAILURE;
    }
    
    std::vector<std::string> t_topics(1);
    t_topics[0] = m_topics;
    RdKafka::ErrorCode err = m_kconsumer->subscribe(t_topics);

    if (err) 
    {
        //cerr << "Failed to subscribe to " << m_topics.size() << " topics: " << RdKafka::err2str(err) << endl;
        pl_log(ERR, " Failed to subscribe to topics : %s", RdKafka::err2str(err).c_str());
        return PF_RET_FAILURE;
    }
    
    m_flag = TRUE;
    return PF_RET_SUCCESS;
}

S32 kafka_consumer::create_consumer(void)
{
    if (m_use_kconsumer)
    {
        return create_kconsumer();
    }
    else
    {
        return create_normal_consumer();
    }
}


/**
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

int kafka_consumer:: print_consumer_throughput(std::string str_stats)
{
    if (pl_debug_rdconsumer_stats_full_log_on)
    {
        pl_log(WARN, "--- rd consumer statistics: %s", str_stats.c_str());
        return PF_RET_FAILURE;
    }
    
    m_stats = str_stats;


/*
    string::size_type p;
    unsigned long long rxmsgs;
    std::string topic_name;
    std::string consumer_name;
    std::string str_tmp;

    if ((p = str_stats.find("\"topic\":\"")) != std::string::npos)
    {
        str_tmp = str_stats.substr(p + strlen("\"topic\":\""));
        topic_name = str_tmp.substr(0, str_tmp.find("\""));
	}

    if ((p = str_stats.find("\"client_id\":")) != std::string::npos)
    {
        str_tmp = str_stats.substr(p + strlen("\"client_id\":"));
        consumer_name = str_tmp.substr(0, str_tmp.find(","));
	}

    // 1. Total recv messages
    if((p = str_stats.rfind("\"rxmsgs\":")) != 0)
    {
        rxmsgs = strtoull(str_stats.c_str() + p + strlen("\"rxmsgs\":"), NULL, 10);
        pl_log(INF, "%s[ %s]: rxmsgs ( %lld)", consumer_name.c_str(), topic_name.c_str(), rxmsgs);
    }
    // 2. Total recv bytes
    p = 0;
    unsigned long long rxmsg_bytes;
    if((p = str_stats.rfind("\"rxmsg_bytes\":")) != 0)
    {
        rxmsg_bytes = strtoull(str_stats.c_str() + p + strlen("\"rxmsg_bytes\":"), NULL, 10);
        pl_log(INF, "%s[ %s]: rxmsg_bytes ( %lld)", consumer_name.c_str(), topic_name.c_str(), rxmsg_bytes);
    }

*/ 
    return PF_RET_SUCCESS;
}

void pf_consumer_rdkafka_throughput(void)
{
    int i = 0;

    for(i=0; i<ulConsumerNum; i++)
    {
        if(pKafkaConsumer[i])
        {
            pKafkaConsumer[i]->print_consumer_static();
        }
    }	
	
    pl_log(INF, "consumer_event_cb_cnt=%d consumer_configure_event_cb_cnt=%d consumer_configure_full_event_cb_cnt=%d consumer_rebalance_cb_cnt=%d consumer_cb_cnt=%d",
                PS_CGet(CM_COM, CMCOM_ID_RDKAFKA_CONSUMER_EVENT_CB_CNT), \
                PS_CGet(CM_COM, CMCOM_ID_RDKAFKA_CONSUMER_CONFIGURE_EVENT_CB_CNT), \
                PS_CGet(CM_COM, CMCOM_ID_RDKAFKA_CONSUMER_CONFIGURE_FULL_EVENT_CB_CNT), \
                PS_CGet(CM_COM, CMCOM_ID_RDKAFKA_CONSUMER_REBALANCE_CB_CNT), \
                PS_CGet(CM_COM, CMCOM_ID_RDKAFKA_CONSUMER_CB_CNT)); 
}

void kafka_consumer::event_cb (RdKafka::Event &event)
{
    PS_CPlus(CM_COM, CMCOM_ID_RDKAFKA_CONSUMER_EVENT_CB_CNT);

    switch (event.type())
    {
        case RdKafka::Event::EVENT_ERROR:
            m_event_error_cnt++;
            //cerr << "ERROR (" << RdKafka::err2str(event.err()) << "): " << event.str() << endl;
            pl_log(ERR, "ERROR ( %s ):%s ", RdKafka::err2str(event.err()).c_str(), event.str().c_str());
            if (event.err() == RdKafka::ERR__ALL_BROKERS_DOWN)
            {
                //todo: should info APP
                //cerr << " ERR__ALL_BROKERS_DOWN !" << endl;
                pl_log(ERR, "ERR__ALL_BROKERS_DOWN !");
            }

            if (m_app_event_recv_function)
            {
                m_app_event_recv_function(event.err());
            }
			
            break;

        case RdKafka::Event::EVENT_STATS:
            m_event_stats_cnt++;
            print_consumer_throughput(event.str());
            break;

        case RdKafka::Event::EVENT_LOG:
            m_event_log_cnt++;
            //fprintf(stderr, "LOG-%i-%s: %s\n", event.severity(), event.fac().c_str(), event.str().c_str());
            pl_log(ERR, "LOG-%i-%s:%s", event.severity(), event.fac().c_str(), event.str().c_str());
            break;

        default:
            m_default_cnt++;
            //cerr << "EVENT " << event.type() << " (" << RdKafka::err2str(event.err()) << "): " << event.str() << endl;       
            pl_log(INF, "EVENT %s( %s  ):%s",event.type(), RdKafka::err2str(event.err()).c_str(), event.str().c_str());
            break;
    }
 

    if (m_app_full_event_cb)
    {
        m_app_full_event_cb(event);
    }

}


  /**
   * @brief Group rebalance callback for use with RdKafka::KafkaConsumer
   *
   * Registering a [rebalance_cb] turns off librdkafka's automatic
   * partition assignment/revocation and instead delegates that responsibility
   * to the application's [rebalance_cb].
   *
   * The rebalance callback is responsible for updating librdkafka's
   * assignment set based on the two events: 
   *     RdKafka::ERR__ASSIGN_PARTITIONS
   * and RdKafka::ERR__REVOKE_PARTITIONS 
   *     but should also be able to handle arbitrary rebalancing failures 
   *     where [err] is neither of those.
   * 
   * @remark In this latter case (arbitrary error), the application must
   *         call unassign() to synchronize state.
   *
   * For eager/non-cooperative `partition.assignment.strategy` assignors,
   * such as `range` and `roundrobin`, 
   * the application must use assign assign() to set and unassign() 
   *               to clear the entire assignment.
   * 
   * For the cooperative assignors, such as `cooperative-sticky`, the
   * application must use 
   * incremental_assign() for ERR__ASSIGN_PARTITIONS 
   * and
   * incremental_unassign() for ERR__REVOKE_PARTITIONS.
   *
   * Without a rebalance callback this is done automatically by librdkafka
   * but registering a rebalance callback gives the application flexibility
   * in performing other operations along with the 
   * assinging/revocation,
   * such as 
   * fetching offsets from an alternate location (on assign)
   * or manually committing offsets (on revoke).
   *
   * @sa RdKafka::KafkaConsumer::assign()
   * @sa RdKafka::KafkaConsumer::incremental_assign()
   * @sa RdKafka::KafkaConsumer::incremental_unassign()
   * @sa RdKafka::KafkaConsumer::assignment_lost()
   * @sa RdKafka::KafkaConsumer::rebalance_protocol()
   */
   
void kafka_consumer::rebalance_cb (RdKafka::KafkaConsumer *consumer,
                     RdKafka::ErrorCode err,
                     std::vector<RdKafka::TopicPartition*> &partitions) 
{
    PS_CPlus(CM_COM, CMCOM_ID_RDKAFKA_CONSUMER_REBALANCE_CB_CNT);

    //cerr << "RebalanceCb: " << RdKafka::err2str(err) << ": ";
    pl_log(INF, "RebalanceCb: %s", RdKafka::err2str(err).c_str());

    // print partition info
    for (unsigned int i = 0 ; i < partitions.size() ; i++)
    {
        //cerr << partitions[i]->topic() << "[" << partitions[i]->partition() << "], ";
        //cerr << "\n";
        pl_log(INF, "%s, [ %d ] ", partitions[i]->topic().c_str(), partitions[i]->partition());
    }

    RdKafka::Error *error = NULL;
    RdKafka::ErrorCode ret_err = RdKafka::ERR_NO_ERROR;

    if (err == RdKafka::ERR__ASSIGN_PARTITIONS) 
    {
        if (consumer->rebalance_protocol() == "COOPERATIVE")
        {
            error = consumer->incremental_assign(partitions);
        }
        else
        {
            ret_err = consumer->assign(partitions);
        }
      
        m_partition_cnt += (S32)partitions.size();
    } 
    else 
    {
        if (consumer->rebalance_protocol() == "COOPERATIVE") 
        {
            error = consumer->incremental_unassign(partitions);
            m_partition_cnt -= (S32)partitions.size();
        } 
        else 
        {
            ret_err = consumer->unassign();
            m_partition_cnt = 0;
        }
    }
    
    m_eof_cnt = 0; // FIXME: Won't work with COOPERATIVE

    if (error) 
    {
        //cerr << "incremental assign failed: " << error->str() << "\n";
        pl_log(ERR, "incremental assign failed: %s", error->str().c_str());
        delete error;
    } 
    else if (ret_err)
    {
        //cerr << "assign failed: " << RdKafka::err2str(ret_err) << "\n";
        pl_log(ERR, "assign failed: %s", RdKafka::err2str(ret_err).c_str());
    }

}
 
void kafka_consumer::consume_cb (RdKafka::Message &msg, void* opaque)
{
    PS_CPlus(CM_COM, CMCOM_ID_RDKAFKA_CONSUMER_CB_CNT);

    if(m_app_msg_handler)
    {
        m_app_msg_handler(&msg, opaque);
    }
    else
    {
        pl_log(ERR, "m_app_msg_handler is null pointer");
    }

}

S32 kafka_consumer::recv(void)
{
    if(NULL == m_app_msg_handler)
    {
        //cout << "m_app_msg_handler is null pointer" << endl;
        pl_log(ERR, "m_app_msg_handler is null pointer");
        return PF_RET_FAILURE;
    }
    
    S32 ret = 0;
    if (m_kconsumer)
    {
        RdKafka::Message *msg = m_kconsumer->consume(1000);
        ret = m_app_msg_handler(msg, NULL);
        delete msg;
    }
    else
    {
#ifdef PHASE_2  //现阶段先不使用
        if (m_use_callback)
        {
            m_consumer->consume_callback(m_topic, m_partition, 1000, this, &m_use_callback);
        }
        else
        {
            RdKafka::Message *msg = m_consumer->consume(m_topic, m_partition, 1000);
            m_app_msg_handler(msg, NULL);
            delete msg;
        }
#endif        
    }
    return ret;
}

S32 kafka_consumer::set_config(string config_name, string config_value)
{
    CHAR errbuf[ERRBUF_MAX] = {0};
    
    rd_kafka_conf_t *rk_conf = m_conf->c_ptr_global();

    if (rd_kafka_conf_set(rk_conf, config_name.c_str(), config_value.c_str(), errbuf, sizeof(errbuf)))
    {
        //cerr << "fail to set " << config_name << endl;
        //cout << errbuf << endl;
        pl_log(ERR, "fail to set %s, reason: %s", config_name.c_str(), errbuf);
        return PF_RET_FAILURE;
    }
    else
    {
        //cout << "set " << config_name << " ok !" << endl;
        pl_log(INF, "set %s ok", config_name.c_str());
    }
    
    return PF_RET_SUCCESS;
}

string kafka_consumer::get_config(string para)
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

S32 kafka_consumer::show_config(void)
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
         
        //-----consumer's  
        get_config("group.id");
        get_config("group.instance.id");               
        get_config("partition.assignment.strategy");      
        get_config("session.timeout.ms");
        get_config("heartbeat.interval.ms");
        get_config("group.protocol.type");
        get_config("coordinator.query.interval.ms");
        get_config("max.poll.interval.ms");    
        get_config("enable.auto.commit");
        get_config("auto.commit.interval.ms");
        
        get_config("enable.auto.offset.store");
        get_config("queued.min.messages");
        get_config("queued.max.messages.kbytes");
        get_config("fetch.wait.max.ms");
        get_config("fetch.message.max.bytes");
        get_config("max.partition.fetch.bytes");
        get_config("fetch.max.bytes");   
        get_config("fetch.min.bytes");
        get_config("fetch.error.backoff.ms");   
        //get_config("offset.store.method"); Deprecated
        
        get_config("isolation.level");   
        get_config("consume_cb");
        get_config("rebalance_cb");
        get_config("offset_commit_cb");
        get_config("enable.partition.eof");   
        get_config("check.crcs");
        get_config("allow.auto.create.topics");
        // get_config("auto.commit.enable"); Deprecated
        // get_config("enable.auto.commit"); Deprecated
        get_config("auto.commit.interval.ms");
        
        get_config("auto.offset.reset");
        //get_config("offset.store.path"); Deprecated
        //get_config("offset.store.sync.interval.ms"); Deprecated
        //get_config("broker"); Deprecated
        get_config("consume.callback.max.messages");
        //-- end of consumer
    }
    return PF_RET_SUCCESS;
}




S32 kafka_consumer::seek_offset(string topic, S32 slPartition, S64 offset, S32 use_seek) 
{
    RdKafka::TopicPartition *next = RdKafka::TopicPartition::create(topic, slPartition, offset);
    RdKafka::ErrorCode err;

    if (!use_seek) 
    {
        std::vector<RdKafka::TopicPartition*> parts;
        parts.push_back(next);
        err = m_kconsumer->assign(parts);
        if (err)
        {
            //cout << "assign() failed: " << RdKafka::err2str(err) << endl;
            pl_log(ERR, " assign() %d failed: %s ", offset, RdKafka::err2str(err).c_str());
        } 
        else
        {
            //cout << "assigned ok " << endl;
            pl_log(INF, "assigned %d ok", offset);
        } 
    } 
    else 
    {
        err = m_kconsumer->seek(*next, 5000);
    
        if (err)
        {
            //cout << "seek() failed: " << RdKafka::err2str(err) << endl;
            pl_log(ERR, " seek() %d failed: %s ", offset, RdKafka::err2str(err).c_str());
        }
        else
        { 
            //m_kconsumer->commitSync();
            //cout <<  " seek ok " << endl;
            pl_log(INF, " seek %d ok", offset);
        }
    }

    delete next;
 
    return PF_RET_SUCCESS;
}

S32 kafka_consumer::set_consume_latest() 
{
    
    RdKafka::ErrorCode err;
    std::vector<RdKafka::TopicPartition*> parts;
    RdKafka::TopicPartition *next;
    int32_t num_of_partitions = 0;
    /* Retrieve list of topics */

    RdKafka::Metadata *md;

    m_kconsumer->metadata(true, NULL, &md, 10000);

    RdKafka::Metadata::TopicMetadataIterator it;
          
    for (it = md->topics()->begin(); it != md->topics()->end(); ++it) 
    {
        //cout << "  topic \""<< (*it)->topic() << "\" with "
        //      << (*it)->partitions()->size() << " partitions:" << endl;
        pl_log(INF, " topic [ %s ] with [ %d ]partitions:", (*it)->topic().c_str(), (*it)->partitions()->size());
              
        if (0 == m_topics.compare((*it)->topic()))
        {
            num_of_partitions = (*it)->partitions()->size();
            //cout << "Find partion size is " << num_of_partitions << endl;
            pl_log(INF, "Find partion size is %d", num_of_partitions);
            break;
        }
    }     
//----
    //TopicMetadataVector  *topics()
    // typedef std::vector<const TopicMetadata*> TopicMetadataVector
    const RdKafka::Metadata::TopicMetadataVector *p_topic_vector = md->topics();
    const RdKafka::TopicMetadata* ptopic_md = (*p_topic_vector)[0];
    //cout << "------------>" << ptopic_md->topic() << endl;
    pl_log(INF, "------------>topic : %s", ptopic_md->topic().c_str());
//--
    delete md;
        

    long int low = 0;
    long int high = 0;
    S32 trys = 0;
    
    for (int i = 0; i < num_of_partitions; i++)
    {
        while (1)
        {
            err = m_kconsumer->query_watermark_offsets(m_topics, i, &low, &high, 5000);

            if (err)
            {
                //cout << "query offset failed @ partition-" << i << " ,reason : " << RdKafka::err2str(err) << endl;
                pl_log(ERR, "query offset failed @ partition- %d, reason: %s", i, RdKafka::err2str(err).c_str());

                if (trys++ == 10)
                {
                    return (S32) err;
                }
            }
            else
            {
                trys = 0;
                //cout << "latest offset is " << high << endl;
                pl_log(INF, "latest offset is %d", high);

                seek_offset(m_topics, i, high, 0);
                next = RdKafka::TopicPartition::create(m_topics, i, high);
                parts.push_back(next);
                break;
            }
        }
     }
    
    m_kconsumer->commitSync(parts);
    
    for (int i = 0; i < (int) parts.size(); i++)
    {
        delete parts[i];
    }
    
    m_kconsumer->unsubscribe();
    std::vector<std::string> t_topics(1);
    t_topics[0] = m_topics;
    
    err = m_kconsumer->subscribe(t_topics);
    
    if (err)
    {
        //cout << " in reset offset : " << "subscribe failed " << endl;
        pl_log(ERR, " in reset offset : subscribe failed : %d", err);
    }
    
    return PF_RET_SUCCESS;
}

S32 kafka_consumer::set_config_by_file(string file)
{
    FILE* fp = fopen(file.c_str(), "r");
    
    if (fp)
    {
        char content[CONTENT_MAX] = {0};
        char para[PARA_MAX] = {0};
        char settings[SETTINGS_MAX] = {0};
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
                        pl_log(INF, "TAG AND VAL: %s, %s", tag.c_str(), val.c_str());
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


S32 kafka_consumer::start_recving(void)
{
    m_execute.store(true, std::memory_order_release);

    m_thd = std::thread([=]()
    {
        while (m_execute.load(std::memory_order_acquire))
        {
            recv();
        }
        
        //cout <<  "consumer recving thread say goodbye---------" << endl;
        pl_log(FATAL, "consumer recving thread say goodbye 0x%x---------", this);
        PS_CPlus(CM_PES, CMPES_ID_RDKAFKA_COSUMER_THREAD_FAIL);
    });

    return PF_RET_SUCCESS;
}

S32 kafka_consumer::set_statistics_inverval(S32 i_second)
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

S32 kafka_consumer::set_self_name(std::string name)
{
    string errstr;

    if (m_conf->set("client.id", name, errstr) != RdKafka::Conf::CONF_OK) 
    {
        //std::cerr << errstr << std::endl;
        pl_log(ERR, "Set self name failed %s", errstr.c_str());
        return PF_RET_FAILURE;
    }

    return PF_RET_SUCCESS;
}

S32 kafka_consumer::seek_offset_by_timestamp(std::string topic, std::string time_str , S64* offset)
{
    std::istringstream in(time_str); // put the date in an istringstream
    
    std::tm t;

    t.tm_isdst = -1; // let std::mktime try to figure out if DST is in effect

    in >> std::get_time(&t, "%Y-%m-%d %H:%M:%S"); // extract it into a std::tm
    
    int64_t timestamp = std::mktime(&t);   // get epoch

    std::vector<RdKafka::TopicPartition*> query_parts;
    
    query_parts.push_back(RdKafka::TopicPartition::create(m_topics, 0, timestamp));

    RdKafka::ErrorCode err = m_kconsumer->offsetsForTimes(query_parts, 5000);

    if (err != RdKafka::ERR_NO_ERROR)
    {
        //cout << "get_offset_by_timestamp: err is %s" << RdKafka::err2str(err).c_str() << endl;
        pl_log(ERR, "get_offset_by_timestamp: err is %s", RdKafka::err2str(err).c_str());
        RdKafka::TopicPartition::destroy(query_parts);
        return PF_RET_FAILURE;
    }
    // If offset is not exist, for example, topic retention time is up, then the messages deleted by kafka itself, 
    // the err code is SUCCESS, but return offset is -1 !
    *offset = query_parts[0]->offset();
    
    seek_offset(topic, 0, *offset, 0);
    RdKafka::Message *msg = m_kconsumer->consume(5000);
                       
    if (!msg)
    {
        //cout << "seek by timestamp: consume() returned NULL" << endl;
        pl_log(ERR, "seek by timestamp: consume() returned NULL");
        return PF_RET_FAILURE;
    }
    
    if (msg->err())
    {
        //cout << "seek by timestamp: consume() returned error: " <<  msg->errstr() << endl;
        pl_log(ERR, "seek by timestamp: consume() returned error: %s", msg->errstr().c_str());
    }
    
    if (msg)
    {
        delete msg;
    }
    
    seek_offset(topic, 0, *offset, 1);
    
    msg = m_kconsumer->consume(5000);
                                                                                                             
    if (!msg)
    {
        //cout << "seek by timestamp2: consume() returned NULL" << endl;
        pl_log(ERR, "seek by timestamp2: consume() returned NULL");
        return PF_RET_FAILURE;
    }
    if (msg->err())
    {
        //cout << "seek by timestamp2: consume() returned error: " <<  msg->errstr() << endl;
        pl_log(ERR, "seek by timestamp2: consume() returned error: %s", msg->errstr().c_str());
    }
    if (msg)
    {
        delete msg;
    }
    
    RdKafka::TopicPartition::destroy(query_parts);
    *offset +=1;
    return PF_RET_SUCCESS; 
}

S32 kafka_consumer::create_consumer(std::vector <S32> &partition_list)
{
    string errstr;
    m_kconsumer = RdKafka::KafkaConsumer::create(m_conf, errstr);
                                             
    if (!m_kconsumer)
    {
        //cerr << "Failed to create kafka consumer: " << errstr << endl;
        pl_log(ERR, "Failed to create kafka consumer: %s", errstr.c_str());
        return PF_RET_FAILURE;
    }

    rd_kafka_topic_partition_list_t *c_topics;
    rd_kafka_resp_err_t err;
    
    U32 p_num = partition_list.size();
    c_topics = rd_kafka_topic_partition_list_new(p_num);

    for(U32 i = 0; i < p_num; i++)
    {
        rd_kafka_topic_partition_list_add(c_topics, m_topics.c_str(), partition_list[i]);
        if ((err = rd_kafka_assign(m_kconsumer->c_ptr(), c_topics)))
        {
            //cerr << "Failed to assign : " << rd_kafka_err2str(err);
            pl_log(ERR, "create_consumer->  Failed to assign: %s: ", rd_kafka_err2str(err));
            return PF_RET_FAILURE;
        }
    }

    rd_kafka_topic_partition_list_destroy(c_topics);
    return PF_RET_SUCCESS;
}
