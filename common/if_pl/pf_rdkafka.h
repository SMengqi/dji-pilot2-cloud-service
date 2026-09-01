#ifndef _PL_RDKAFKA_H_
#define _PL_RDKAFKA_H_


#include <librdkafka/rdkafkacpp.h>
#include <librdkafka/rdkafka.h>
#include <thread>
#include <atomic>

#include "pl_type.h"

#define SEND_MODE "send.mode"    
#define MODE_SYNC "sync"
#define MODE_ASYNC "async"

#define FIXED_KEY "custom.key"

/* MR1534:为了减少应用层的负担，增加几个配置项，可以让平台代码自动调用 poll 或者 flush 函数
 * 注意：
 * 1. 这些参数仅对 ASYNC 模式有效
 * 2. 需要 create 成功后，才可以进行设置
 *
 * 取值说明：
 * SEND_WITH_POLL: 如果是 0, 表示 send 时，不调用 poll(0) 函数, 其它值表示自动调用 poll(0)
 * SEND_WITH_FLUSH: 如果是 0, 表示 send 时，不调用 flush(time_out) 函数, 其它值表示自动调用 flush(time_out)
 * SEND_FLUSH_INT: 间隔多少次进行一次 flush 操作，数字要是大于 0 的整数。需要 SEND_WITH_FLUSH 设置非 0 后才有效
 *
 * 示例代码：
 *
 * kp.create_producer();
 *
 * kp.set_config(SEND_WITH_POLL, "1");  // send 时调用 poll
 * kp.set_config(SEND_WITH_FLUSH, "0");  // send 时，不需要调用 flush
 * kp.set_config(SEND_FLUSH_INT, "100");  // 每 100 次 send 后，调用一次 flush，但由于SEND_WITH_FLUSH 为0,此设置无意义
*/
#define SEND_WITH_POLL "send_with_poll"
#define SEND_WITH_FLUSH "send_with_flush"
#define SEND_FLUSH_INT "flush_interval"

using std::string;

#define PL_KAFKA_PARTITION_UA  ((S32)-1)

typedef S32 (*HANDLE_MQ_SEND_ERR)(S32);

typedef S32 (*HANDLE_MQ_EVENT_CB)(RdKafka::Event &event);

typedef struct {
    std::string header_name;
    S8 *value;
    U32 vlen;
}PL_KAFKA_HEADER;

// default statistics log output interval, in milisecond
 #define DEFAULT_STAT_INTVAL 10000   

/* 为将开发过程变得更加合理，将开放接口函数的动作分为几个阶段，
 * 第一阶段（ PHASE_1 )为配合目前已明确使用的接口，要重点进行测试
 * 第二阶段 ( PHASE_2 )为可能的使用场景，将视以后的需求再开放
 * 使用宏定义来区分不同阶段
 * 属于第一阶段的接口：　PHASE_1　
 * 属于第二阶段的接口：　PHASE_2
 */

#define PHASE_1 1

// kafka 单条信息的大小有限制，一般缺省都是 1000000, 但这个是包含　kafka 协议头的，据网上资料大概小于　256 字节

#define PRODUCER_MAX_SEND_LEN (1000000 - 256)
#define PRODUCER_MAX_KEY_LEN 512
#define PRODUCER_MAX_HEADER_LEN 512
#define PRODUCER_MAX_HEADER_VAL_LEN 512

#define ERR_PARA 2

void pf_consumer_rdkafka_throughput(void);
void pf_producer_rdkafka_throughput(void);



class kafka_producer : public RdKafka::DeliveryReportCb,
                       public RdKafka::EventCb
{
  public:
    
    kafka_producer();
    ~kafka_producer();

    /* send msg to the internal queue (internal queue may buffer the messages then send them out at proper time :
     * (s32ernal queue size > batch.size) or (buffering time > queue.buffering.max.ms(linger.ms))
     */
         

    /* S32 configure (string brokers, string topics)
     * 函数说明：生产者配置（简单模式），如果程序中不考虑数据是否发送成功等因素，可以使用此接口，可以用于快速验证的场景
     * 入参：
     * 　　　string brokers:　kafka 集群信息，如 172.16.17.53:9092, 172.16.17.54:9092
     *   　　string topics : topic 信息
     * 
     * 返回值：
     * 　　 ０：ok
     *      其它：失败　
     */       
    S32 configure(string brokers, string topics);


    /* S32 configure(string brokers, string topics, HANDLE_MQ_SEND_ERR app_cb, BOOL idempotent)
     * 函数说明：生产者配置（复杂模式)，建议正式项目采用
     * 入参：
     *   string brokers:　kafka 集群信息，如 172.16.17.53:9092, 172.16.17.54:9092
     *   string topics : topic 信息
     * 　HANDLE_MQ_SEND_ERR app_cb：　发送消息后的回调函数指针，用户可以选择创建一个自己的函数，来应对消息传递的结果
     *   BOOL idempotent : 是否采用幂等方式（此方式可保证顺序写入和＂确保发出并只发一次消息＂，但对系统消耗较大，效率较低），建议设置　false
     * 
     * 返回值：
     * 　　　　０：ok
     *        其它：失败　
     */          
    S32 configure(string brokers, string topics, HANDLE_MQ_SEND_ERR app_cb, BOOL idempotent);

   /* S32 configure(string brokers, string topics, HANDLE_MQ_SEND_ERR app_cb, HANDLE_MQ_EVENT_CB event_cb, BOOL idempotent)
    * 函数说明：生产者配置（复杂模式)，建议正式项目采用
    * 入参：
    *   string brokers:　kafka 集群信息，如 172.16.17.53:9092, 172.16.17.54:9092
    *   string topics : topic 信息
    *　 HANDLE_MQ_SEND_ERR app_cb：　发送消息后的回调函数指针，用户可以选择创建一个自己的函数，来应对消息传递的结果（post-send）
    *   HANDLE_MQ_EVENT_CB event_cb：　producer 事件的回调函数指针，用户可以选择创建一个自己的定制函数，来对 producer event 进行处理
    *   BOOL idempotent : 是否采用幂等方式（此方式可保证顺序写入和＂确保发出并只发一次消息＂，但对系统消耗较大，效率较低），建议设置　false
    *                                         * 
    * 返回值：
    * 　　　　０：ok
    *        其它：失败　
    */          
    S32 configure(string brokers, string topics, HANDLE_MQ_SEND_ERR app_cb, HANDLE_MQ_EVENT_CB event_cb, bool idempotent);


    
    /* S32 configure_acl(string security_protocol, string sasl_mechanism, string user_name, string user_passwd)
     * 函数说明：配置 ACL 信息（此函数根据实际 kafka 集群的配置选配，如果  kafka 集群无　ACL，则不用调用它）
     * 入参：
     * 　　　string security_protocol: SASL 协议名称，　PLAINTEXT / SASL_SSL / SASL_PLAINTEXT
     * 　　　string sasl_mechanism　： SASL 机制名称，　PLAIN / SCRAM-SHA256 / SCRAM-SHA512
     *    string user_name :  用户名
     *    string user_passwd :　密码
     * 返回值：
     * 　　　　０：ok
     *        其它：失败　 　
     */   
    S32 configure_acl(string security_protocol, string sasl_mechanism, string user_name, string user_passwd);
      
    /* S32 create_producer(void)
     * 函数说明：产生并运行 producer 实例
     * 入参：
     * 　　无
     * 返回值：
     * 　　　０：ok
     *      其它：失败　
     */ 
    S32 create_producer(void);  
    
    /* S32 send(const S8* msg, S32 msg_len, const S8* header_name, S32 name_len, const S8* header_value, S32 value_len, const S8* key, S32 key_len)
     * 函数说明：发送数据　　（kafka 消息头的格式类似　name ：ｖalue, 有一个名字，然后一个值）
     * 入参：　
     *       S8* msg：　指向消息的指针
     * 　　　　　　S32 msg_len：消息长度
     * 　　　　　　S8* header_name：　指向消息头名称的指针
     * 　　　　　　S32 name_len：　消息头名称的长度
     * 　　　　　　S8* header_value：指向消息头的值的指针
     * 　　　　　　S32 value_len：消息头的值的长度
     * 　　　　　　S8* key：指向 key 的指针
     * 　　　　　　key_len：　key 的长度
　　　　　* 返回值：
     * 　　　０：ok
     *      其它：失败
     */
    S32 send(const S8* msg, S32 msg_len, 
                     const S8* header_name, S32 name_len,
                     const S8* header_value, S32 value_len,
                     const S8* key, S32 key_len);
                         
    /* S32 send(string msg, string header_name, string header_value, string key)
     * 函数说明：发送数据　　（kafka 消息头的格式类似　name ：ｖalue, 有一个名字，然后一个值）
     * 入参：　
     *       string msg：　消息
     * 　　　　　　string header_name：　消息头
     * 　　　　　　string header_value：　消息头的值
     * 　　　　　　string key：　ＫＥＹ
　　　　　* 返回值：
     * 　　　０：ok
     *      其它：失败
     */                         
    S32 send(string &msg, string &header_name, string &header_value, string &key);

  
    /* S32 send(const S8* msg, S32 msg_len, const S8* header_name, S32 name_len, const S8* header_value, S32 value_len)
     * 函数说明：发送数据　　（kafka 消息头的格式类似　name ：ｖalue, 有一个名字，然后一个值）
     * 入参：　
     *       S8* msg：　指向消息的指针
     * 　　　　　　S32 msg_len：消息长度
     * 　　　　　　S8* header_name：　指向消息头名称的指针
     * 　　　　　　S32 name_len：　消息头名称的长度
     * 　　　　　　S8* header_value：指向消息头的值的指针
     * 　　　　　　S32 value_len：消息头的值的长度
   　* 返回值：
     * 　　　０：ok
     *      其它：失败
     */
    S32 send(const S8* msg, S32 msg_len, const S8* header_name, S32 name_len, const S8* header_value, S32 value_len);
    
     
    /* S32 send(string msg, string header_name, string header_value)
     * 函数说明：发送数据　　（kafka 消息头的格式类似　name ：ｖalue, 有一个名字，然后一个值）
     * 入参：　
     *       string msg：　消息
     * 　　　　　　string header_name：　消息头
     * 　　　　　　string header_value：　消息头的值
 　　* 返回值：
     * 　　　０：ok
     *      其它：失败
     */    
     S32 send(string &msg, string &header_name, string &header_value);

    /* S32 send(const S8* msg, int msg_len,
     *                      const std::vector <PL_KAFKA_HEADER> & header_vector,
     *                      const std::string& key))
     *
     * 函数说明：发送数据　　（支持多个消息头的发送，支持 key）
     * 入参：　
     *       const char* msg  ：　指向消息的指针
     *       S32  msg_len     ：  消息长度
     * 　　　const std::vector <PL_KAFKA_HEADER> & header_vector ：　header vector 的引用
     * 　　　const std::string& key ：　key 的引用
     *
 　　* 返回值：
     * 　　　０：ok
     *      其它：失败
     */
     S32 send(const S8* msg, S32 msg_len, 
                         const std::vector <PL_KAFKA_HEADER> & header_vector,
                         const std::string& key);

    /* S32 send(std::string& topic,
                         S32 partition, 
                         const S8* msg, int msg_len,
                         const std::vector <PL_KAFKA_HEADER> & header_vector,
                         const std::string& key))
     *
     * 函数说明：发送数据　　（支持 topic, partion, 支持多个消息头的发送，支持 key）
     * 入参：
     *   　string& topic    : topic 的引用
     *    partition        :  分区 ( PL_KAFKA_PARTITION_UA: assigned by system)
     *    const char* msg  ：　指向消息的指针
          S32  msg_len     ：  消息长度
     * 　　　const std::vector <PL_KAFKA_HEADER> & header_vector ：　header vector 的引用
     * 　　　const std::string& key ：　key 的引用

 　　* 返回值：
     * 　　　０：ok
     *      其它：失败
     */
     S32 send(std::string& topic,
                         S32 partition,               
                         const S8* msg, S32 msg_len, 
                         const std::vector <PL_KAFKA_HEADER> & header_vector,
                         const std::string& key);

    /* string get_config(string para)
     * 函数说明：获取某个 rdkafka 的配置项
     * 入参：　
     *      string para：　配置项的名称，具体名称请参考　librdkafka 提供的　CONFIGURATION.md　
     * 返回值：
     * 　　　　正常时，返回配置项信息
     * 　　　　出错时，如果配置项未被配置或输入的配置项不存在，则字符串中无信息．(str.length() = 0)
     */
    string get_config(string para); 
    
    /* S32 set_config(string config_name, string config_value)
     * 函数说明：设置某个 rdkafka 的配置项
     * 入参：　
     *      string config_name： 配置项的名称，具体名称请参考　librdkafka 提供的　CONFIGURATION.md　
     * 　　　　　string config_value: 配置项的名称，具体名称/范围请参考 librdkafka 提供的　CONFIGURATION.md　
     * 返回值：
     * 　　　　０： ok
     * 　　　　其它：失败
     */    
    S32 set_config(string config_name, string config_value);
        
    /* S32　show_config(void)
     * 函数说明：显示 rdkafka 的所有的有关生产者的配置项
     * 入参：　
     *      string config_name： 配置项的名称，具体名称请参考　librdkafka 提供的　CONFIGURATION.md　
     * 　　　　　string config_value: 配置项的名称，具体名称/范围请参考 librdkafka 提供的　CONFIGURATION.md　
     * 返回值：
     * 　　　　０： ok
     * 　　　　其它：失败
     */      
    S32 show_config(void);

    /* void flush()
     * 函数说明：　刷新　rdkafka 内部的消息队列，调用后，rdkafka 会把数据立刻从　buffer 中推到 socket 中，实际发送出去．建议定期（１０秒）或过若干条数据(1000 条)后
     * 　　　　　　调用一次．如果不这样做的话，rdkafka　的发送　ｂｕｆｆｅｒ 在某些大流量的情况下会满，产生堵塞，虽然可自行恢复，但总体效果不佳，延迟变大
     *
    */ 
    void flush();
        
                      
    /* S32 set_config_by_file(string file)
     * 函数说明：　设置一个配置文件的路径（含文件自身名字），如果文件存在，则按文件内部的配置项进行配置
     * 入参：
     * 　　　string file　：　配置文件的绝对路径＋文件名本身，如　/home/broadxt/test/kafka_producer_setting.conf
     *    
     */
    S32 set_config_by_file(string file);
         
#ifdef PHASE_2    //现阶段，先不开放这些成员
    S32 send(string msg);
    
    S32 send(string msg, string header_name, string header_value);
    
    S32 send(string msg, string key);  
 
    S32 send(const S8* msg, S32 msg_len);
                        
    S32 send(const S8* msg, S32 msg_len,
                         const S8* key, S32 key_len);
#endif
        

                         
#ifdef PHASE_2  //现阶段，先不开放这些成员 
    S32 send(string msg, S32 slPartition);
    S32 send(string msg, string header_name, string header_value, S32 slPartition);
    S32 send(string msg, string key, S32 slPartition);  
    S32 send(string msg, string header_name, string header_value, string key, S32 slPartition);
    
    
    S32 send(const S8* msg, S32 msg_len, S32 slPartition);
    
    S32 send(const S8* msg, S32 msg_len, 
                         const S8* header_name, S32 name_len,
                         const S8* header_value, S32 value_len, 
                         S32 slPartition);
                         
    S32 send(const S8* msg, S32 msg_len,
                         const S8* key, S32 key_len,
                         S32 slPartition);
                         

#endif                               


    // experimental: 
    void start_polling(S32 interval);


    /* S32　set_statistics_inverval(S32 i_second)
     * 函数说明：设置统计间隔时间，以秒为单位
     * 入参：　
     *      S32 i_second： 统计间隔时间，以秒为单位。如果设置为 0，则关闭统计功能。!注意! 此函数的位置，要写在 configure 之前，在建立对象后，就马上调用它。　
     * 　　　　　　
     * 返回值：
     * 　　　　０： ok
     * 　　　　其它：失败（比如设置了负值）
     */
     S32 set_statistics_inverval(S32 i_second);

    /* S32　set_self_name(std::string name)
     * 函数说明：设置 producer 的名字，如果不设置的话，默认名字是 rdkafka。!注意! 此函数需要在 configure 调用后，再调用这个函数。
     * 入参：　
     *      std::string name：生产者的名字，可以起一个独特的名字，这样可以区别与其它生产者，方便追踪问题或数据统计　
     * 　　　　　
     * 返回值：
     * 　　　　０： ok
     * 　　　　其它：失败
     */
    S32 set_self_name(std::string name);            
    void print_producer_static(void);
    int print_producer_throughput(std::string str_stats);

  private:
    string m_brokers;
    string m_topics;
    volatile BOOL m_do_not_send = false;
        
    RdKafka::Conf *m_conf = NULL;
    RdKafka::Producer *m_producer = NULL;
      
    // derived from RdKafka::DeliveryReportCb
    void dr_cb (RdKafka::Message &message);
    // derived from RdKafka::Event
    void event_cb (RdKafka::Event &event);
    HANDLE_MQ_SEND_ERR m_app_cb = NULL;
    
    HANDLE_MQ_EVENT_CB m_event_cb = NULL;

    // experimental: start/stop a thread for polling
   
    std::atomic<BOOL> m_execute;

    std::thread m_thd;
 
    U32 m_event_error_cnt = 0;
    U32 m_event_stats_cnt = 0;
    U32 m_event_log_cnt = 0;
    U32 m_default_cnt = 0;
    //producer is ready or not
    U32 m_flag = FALSE;

    string m_status;
   
    S32 configure_basic(string brokers, string topics);
    S32 configure_cb(void);
      
    BOOL m_sync_mode = true;
    
    S32 send(const S8* msg, S32 msg_len, 
                     const S8* header_name, S32 name_len,
                     const S8* header_value, S32 value_len,
                     S8* key, S32 key_len,
                     S32 slPartition);

    S32 send_action(S32 partition, S32 flag, void *payload, S32 len, const void* key, S32 key_len, S64 timestamp, RdKafka::Headers* headers, void* msg_opaque);

    S32 rdkafak_send_interface(std::string& topic, S32 partition, S32 flag, void *payload, S32 len, const void* key, S32 key_len, S64 timestamp, RdKafka::Headers* headers, void* msg_opaque);


    string m_custom_key;

    S32 m_statistics_on = 1; // None zero is ON, but statistics log may cost some CPU timeslice, user can choice to OFF the log
    S32 m_statistics_interval = DEFAULT_STAT_INTVAL; // Default statistics log interval, in mili-seconds

    S32 m_send_with_poll = 1;
    S32 m_send_with_flush = 0;
    S32 m_flush_cnt = 0;
    S32 m_flush_interval = 1000; // default 
};


typedef void (*APP_EVENT_HANDLER)(S32 err_code);  // call back function for APPs

typedef void (*APP_FULL_EVENT_HANDLER)(RdKafka::Event &event);  // call back for all events

typedef int (*APP_MSG_HANDLER)(RdKafka::Message* message, void* opaque);

class kafka_consumer : public RdKafka::RebalanceCb,
                       public RdKafka::EventCb,
                       public RdKafka::ConsumeCb
{
  public:
    
    kafka_consumer();
    ~kafka_consumer();
    
    /* S32 configure(string brokers, string topics, string strGroupId, APP_MSG_HANDLER app_msg_handler, APP_EVENT_HANDLER app_event_recv_function)
     *   函数说明：消费者配置（复杂模式）
     *   string brokers:　kafka 集群信息，如 172.16.17.53:9092, 172.16.17.54:9092
     *   string topics : topic 信息
     *   string strGroupId: 消费组的 ID, 如　consumer-group-1
     * 　 APP_MSG_HANDLER app_msg_handler：　消息处理函数指针，用户需要创建此指针对应的函数，来进行消息处理
     *   APP_EVENT_HANDLER app_event_recv_function：　异常处理函数指针，用户需要创建此指针对应的函数，来进行异常消息处理
     * 返回值：
     * 　　　　０：ok
     *        其它：失败　
     */   
    S32 configure(string brokers, string topics, string strGroupId, APP_MSG_HANDLER app_msg_handler, APP_EVENT_HANDLER app_event_recv_function);
    
    /* S32 configure(string brokers, string topics, string strGroupId, APP_MSG_HANDLER app_msg_handler, APP_FULL_EVENT_HANDLER app_event_recv_function)
     *   函数说明：消费者配置（复杂模式）
     *   string brokers:　kafka 集群信息，如 172.16.17.53:9092, 172.16.17.54:9092
     *   string topics : topic 信息
     *   string strGroupId: 消费组的 ID, 如　consumer-group-1
     * 　 APP_MSG_HANDLER app_msg_handler：　消息处理函数指针，用户需要创建此指针对应的函数，来进行消息处理
     *   APP_EVENT_HANDLER app_event_recv_function：　异常处理函数指针，用户需要创建此指针对应的函数，来进行异常消息处理
     * 返回值：
     * 　　　　０：ok
     *        其它：失败　
     */   
    S32 configure(string brokers, string topics, string strGroupId, APP_MSG_HANDLER app_msg_handler, APP_FULL_EVENT_HANDLER app_event_recv_function);
   
    /* S32 configure_acl(string security_protocol, string sasl_mechanism, string user_name, string user_passwd)
     *   函数说明：配置 ACL 信息（此函数根据实际 kafka 集群的配置选配）
     * 返回值：
     * 　　　　０：ok
     *        其它：失败　 　
     */
    S32 configure_acl(string security_protocol, string sasl_mechanism, string user_name, string user_passwd);

    /* S32 create_consumer(void)
     *   函数说明：产生消费者实例并运行
     * 返回值：
     * 　　　　０：ok
     *        其它：失败　
     */
    S32 create_consumer(void);

    /* S32 create_consumer(std::vector <S32> &partition_list)
     *   函数说明：产生消费者实例并运行，可以指定分区（一个或多个），之后只从指定分区中获取消息
     * 入参： std::vector <S32> &partition_list： 存放指定分区的 vector
     *     
     * 返回值：
     * 　　　　０：ok
     *        其它：失败　
     */
    S32 create_consumer(std::vector <S32> &partition_list);
    
    /* S32 recv(void)
     *   函数说明：接收数据，需要使用者构建一个　while(1)循环，来读取数据．
     * 　　while(running)
     *   {
     *      recv();
     *   }
     * 返回值：
     * 　　　　０：ok 
     *        如果接收数据过程中出现问题，会在 event 回调函数中提示用户
    */
    S32 recv(void);

    /* string get_config(string para)
     * 函数说明：获取某个 rdkafka 的配置项
     * 入参：　
     * 　　　　　　　string para：　配置项的名称，具体名称请参考　librdkafka 提供的　CONFIGURATION.md　
     * 返回值：
     * 　　　　正常时，返回配置项信息
     * 　　　　出错时，如果配置项未被配置或输入的配置项不存在，则字符串中无信息．(str.length() = 0)
     */
    string get_config(string para); 
    
    /* S32 set_config(string config_name, string config_value)
     * 函数说明：设置某个 rdkafka 的配置项
     * 入参：　
     * 　　　　　　　string config_name： 配置项的名称，具体名称请参考　librdkafka 提供的　CONFIGURATION.md　
     * 　　　　　　　string config_value: 配置项的名称，具体名称/范围请参考 librdkafka 提供的　CONFIGURATION.md　
     * 返回值：
     * 　　　　０： ok
     * 　　　　其它：失败
     */     
    S32 set_config(string config_name, string config_value);
    
    /* S32　show_config(void)
     * 函数说明：显示 rdkafka 的所有的有关消费者的配置项
     * 入参：　
     * 　　　　　　　string config_name： 配置项的名称，具体名称请参考　librdkafka 提供的　CONFIGURATION.md　
     * 　　　　　　　string config_value: 配置项的名称，具体名称/范围请参考 librdkafka 提供的　CONFIGURATION.md　
     * 返回值：
     * 　　　　０： ok
     * 　　　　其它：失败
     */        
    S32 show_config(void);
      
    /* S32 set_consume_latest(void)
     * 函数说明：将消费读取点调整到最新的读取点．使用场景：　在使用同样的　consumer group id 的条件下，如果消费者长期不上线，而对应的 topic 的消息个数却一直在增加，
     * 　　　　　在消费者重新开始运行时，又不愿意去消费之前的历史数据，则可以使用这个函数来重新置位．调用之后，消费者将从最新的数据开始消费．
     * 入参：
     * 　　　　无　
     * 返回值：
     * 　　　　０： ok
     * 　　　　其它：失败
     */ 
    S32 set_consume_latest(void);   
    
    /* S32 set_config_by_file(string file)
     * 函数说明：　设置一个配置文件的路径（含文件自身名字），如果文件存在，则按文件内部的配置项进行配置
     * 入参：
     * 　　　string file　：　配置文件的绝对路径＋文件名本身，如　/home/broadxt/test/kafka_consumer_setting.conf
     *    
     */
    S32 set_config_by_file(string file);
    
#ifdef PHASE_2      
    /* configure(string brokers, string topics, S32 slPartition, BOOL use_callback, APP_MSG_HANDLER app_msg_handler, APP_EVENT_HANDLER app_event_recv_function)
     * 函数说明：消费者配置（简单模式）
     * 　入参：
     * 　　string brokers:　kafka 集群信息，如 172.16.17.53:9092, 172.16.17.54:9092
     *   string topics : topic 信息
     * 　　S32 slPartition : 分区号，从　０　开始
     * 　　BOOL use_callback：　是否采用自动回调函数．采用自动回调函数的话，可以不采用　while(1) 循环来接收数据，只需调用一次，在有数据后，可自动回调函数来读取数据
     * 　　APP_MSG_HANDLER app_msg_handler：　消息处理函数指针，用户需要创建此指针对应的函数，来进行消息处理
     * 　　APP_EVENT_HANDLER app_event_recv_function：　异常处理函数指针，用户需要创建此指针对应的函数，来进行异常消息处理
     * 　返回值：
     * 　　　　０：ok
     *        其它：失败　
     */      
    S32 configure(string brokers, string topics, S32 slPartition, BOOL use_callback, APP_MSG_HANDLER app_msg_handler, APP_EVENT_HANDLER app_event_recv_function);
        
#endif

    /* S32　start_recving(void);
     * 函数说明：启动一个线程来接收数据，相当于平台代码为使用者启动了一个线程，线程内部是一个 while（1）+ recv 的循环， 用于接收数据
     * 入参：　
     * 　　　　无　
     * 返回值：
     * 　　　　０： ok
     * 　　　　其它：失败
     */    
    S32 start_recving(void);

    /* S32　set_statistics_inverval(S32 i_second)
     * 函数说明：设置统计间隔时间，以秒为单位
     * 入参：　
     *      S32 i_second： 统计间隔时间，以秒为单位。如果设置为 0，则关闭统计功能。!注意! 此函数的位置，要写在 configure 之前，在建立对象后，就马上调用它。　
     * 　　　　　　
     * 返回值：
     * 　　　　０： ok
     * 　　　　其它：失败（比如设置了负值）
     */
    S32 set_statistics_inverval(S32 i_second);

    /* S32　set_self_name(std::string name)
     * 函数说明：设置 consumer 的名字，如果不设置的话，默认名字是 rdkafka。!注意! 此函数需要在 configure 调用后，再调用这个函数。
     * 入参：　
     *      std::string name：消费者的名字，可以起一个独特的名字，这样可以区别与其它消费者，方便追踪问题或数据统计　
     * 　　　　　
     * 返回值：
     * 　　　　０： ok
     * 　　　　其它：失败
     */
    S32 set_self_name(std::string name);

    S32 seek_offset_by_timestamp(std::string topic, std::string time_str , S64* offset);
    
    void print_consumer_static(void); 
    int print_consumer_throughput(std::string str_stats);
  private:
    string m_brokers;
    string m_topics;
    string m_groups;
    S32 m_partition_cnt = 0;
    S32 m_eof_cnt = 0;
    
    string m_stats;
    U32 m_event_error_cnt = 0;
    U32 m_event_stats_cnt = 0;
    U32 m_event_log_cnt = 0;
    U32 m_default_cnt = 0;
    //consumer is ready or not
    U32 m_flag = FALSE;

    S32 m_partition = RdKafka::Topic::PARTITION_UA;
    S64 m_start_offset = RdKafka::Topic::OFFSET_BEGINNING;
        
    RdKafka::Conf *m_conf = NULL;  //  for global
    RdKafka::Conf *m_tconf = NULL;  // for topic
    
    RdKafka::Consumer *m_consumer = NULL;
    BOOL m_use_callback = false;
    BOOL m_quiet = false;
        
    // accroding to the demo of rdKafka, RdKafka::KafkaConsumer has more advanced feature
    RdKafka::KafkaConsumer *m_kconsumer = NULL;
    
    RdKafka::Topic *m_topic = NULL;

    std::atomic<bool> m_execute;
    std::thread m_thd;
      
    // derived from RdKafka::RebalanceCb    
    void rebalance_cb (RdKafka::KafkaConsumer *consumer, RdKafka::ErrorCode err, std::vector<RdKafka::TopicPartition*> &partitions);
    
    // derived from RdKafka::Event
    void event_cb (RdKafka::Event &event);
    
    
    S32 configure_basic (string brokers, string topics);
    S32 configure_cb (BOOL kconsumer, APP_MSG_HANDLER app_msg_handler, APP_EVENT_HANDLER app_event_recv_function);
    S32 configure_cb (BOOL kconsumer, APP_MSG_HANDLER app_msg_handler, APP_FULL_EVENT_HANDLER app_event_recv_function);
   // S32 m_msg_consume (RdKafka::Message* message, void* opaque);
    
    // derived form RdKafka::ConsumeCb  
    void consume_cb (RdKafka::Message &msg, void* opaque);
    
    APP_EVENT_HANDLER m_app_event_recv_function = NULL;
    APP_MSG_HANDLER m_app_msg_handler = NULL;
    APP_FULL_EVENT_HANDLER m_app_full_event_cb = NULL;    
    
    BOOL m_use_kconsumer = true;
    S32 create_normal_consumer(void);
    
    S32 create_kconsumer(void);
    
    /* S32 seek_offset(string topic, S32 slPartition, S64 offset, S32 use_seek)
     * 函数说明：根据指定的　topic 和分区信息，以及　offset, 修改消费记录读取点．
     * 入参：　string topic： topic 名称　
     * 　　　　　　　S32 slPartition：　分区信息
     * 　　　　　　　S64 offset：　offset 信息
     * 　　　　　　　S32 use_seek：　采用　seek 还是　assign, 　区别在于　assign 是临时性修改消费读取点，seek 是实际修改消费读取点，但使用 seek　前必须先调用 assgin.
     * 返回值：
     * 　　　　０： ok
     * 　　　　其它：失败
     */    
    S32 seek_offset(string topic, S32 slPartition, S64 offset, S32 use_seek);

    S32 m_statistics_interval = DEFAULT_STAT_INTVAL; // Default statistics log interval, in mili-seconds
    S32 m_statistics_on = 1;
};




#endif
