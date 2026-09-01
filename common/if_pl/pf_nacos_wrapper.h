#ifndef _PL_NACOS_WRAPPER_H_
#define _PL_NACOS_WRAPPER_H_
#include <atomic>
#include <thread>
#include "pl_type.h"

/* S32 pl_nacos_setconfig(std::string key, std::string value, std::string group, std::string nacos_ip);
 *     std::string key       : 配置项的名称，如 DRC_1_CONFIG_2
 *     std::string value     : 配置项的值, 如 12345 
 *     std::string group     : 配置项所属的 group 的名称，比如 "DRC_GROUP"
 *     std::string nacos_ip  : nacos 服务器/集群的 IP, 如 "172.16.17.56"
 * 返回值:
 * 　　 0:  ok
 *      其它: 失败　
 */

S32 pl_nacos_setconfig(std::string key, std::string value, std::string group, std::string nacos_ip);

/* S32 pl_nacos_setconfig(std::string key, std::string value, std::string nacos_namespace, std::string group, std::string nacos_ip);
 *     std::string key             : 配置项的名称，如 DRC_1_CONFIG_2
 *     std::string value           : 配置项的值, 如 12345 
 *     std::string nacos_namespace : 配置项所属的 namespace 的 ID， 如 "744081e6-3bfb-4ad8-85ac-9398993fb288"
 *     std::string group           : 配置项所属的 group 的名称，比如 "DRC_GROUP"
 *     std::string nacos_ip        : nacos 服务器/集群的 IP, 如 "172.16.17.56"
 *
 * 返回值:
 * 　　 0:  ok
 *      其它: 失败　
 */
S32 pl_nacos_setconfig(std::string key, std::string value, std::string nacos_namespace, std::string group, std::string nacos_ip);


/* std::string pl_nacos_getconfig(std::string key, std::string group, std::string nacos_ip)
 * 函数说明: 在 nacos 获取一个配置项的值
 * 入参:
 * 　　std::string key     : 配置项的名称
 *     std::string group   : 配置项所属的 group 的名称，比如 "DRC_GROUP"
 *     std::string nacos_ip  : nacos 服务器/集群的 IP, 如 "172.16.17.56"
 *
 * 返回值:
 * 　　std::string         : key 值
 *      
 */
std::string pl_nacos_getconfig(std::string key, std::string group, std::string nacos_ip);

/* std::string pl_nacos_getconfig(std::string key, std::string nacos_namespace, std::string group, std::string nacos_ip);
 * 函数说明: 在 nacos 获取一个配置项的值
 * 入参:
 * 　　std::string key             : 配置项的名称
 *     std::string nacos_namespace : 配置项所属的 namespace 的 ID， 如 "744081e6-3bfb-4ad8-85ac-9398993fb288"
 *     std::string group           : 配置项所属的 group 的名称，比如 "DRC_GROUP"
 *     std::string nacos_ip        : nacos 服务器/集群的 IP, 如 "172.16.17.56"   
 *
 * 返回值:
 * 　　std::string         : key 值
 *      
 */
std::string pl_nacos_getconfig(std::string key, std::string nacos_namespace, std::string group, std::string nacos_ip);


/* int pl_nacos_register_instance(std::string &server_addr, std::string &nacos_namespace, std::string &service_name, std::string &metadata_value, std::string &service_address, int service_port)
 *     std::string server_addr       : nacos 服务器/集群的 IP, 如 "172.16.17.56:18888"
 *     std::string nacos_namespace   : 服务所属的 namespace 的 ID， 如 "744081e6-3bfb-4ad8-85ac-9398993fb288"
 *     std::string service_name      : 服务的 service 的名称，比如 "DRC_GROUP"
 *     std::string metadata_value    : 服务的 元数据标识, 如 "DRA01"
 *     std::string service_address   : 服务的 IP地址, 如 "172.16.17.56"
 *     std::string service_port      : 服务的端口地址, 如 36000
 * 返回值:
 * 　　 0:  ok
 *      其它: 失败　
 */
int pl_nacos_register_instance(std::string &server_addr, std::string &nacos_namespace, std::string &service_name, std::string &metadata_value, std::string &service_address, int service_port);


typedef S32 (*HANDLE_KEY_CHANGED)(std::string);
typedef S32 (*HANDLE_KEYS_BY_ONE_CB)(std::string info, std::string key);

typedef struct {
	
	std::string key;
	HANDLE_KEY_CHANGED func;
	
}stKEY_WITH_HANDLER;

#define SLEEP_INTERVAL 500

class pl_nacos_listener
{
public:
    pl_nacos_listener();
    ~pl_nacos_listener();

    /* S32 listen_to_key(std::string key, std::string group, std::string nacos_ip, HANDLE_KEY_CHANGED callback);
     * 函数说明: 对配置项进行监控，使用缺省（public）的命名空间
     * 入参:
     * 　　std::string key               : 配置项名称
     *     std::string group             : 配置项所属的 group 的名称，比如 "DRC_GROUP"
     *     std::string nacos_ip          : nacos 服务器/集群的 IP, 如 "172.16.17.56"
     *     HANDLE_KEY_CHANGED callback   : 回调函数，当配置项发生变动时，此函数会被调用
     *
     * 返回值:
     *
     * 　　 0:  ok
     *      其它: 失败　
     */
    S32 listen_to_key(std::string key, std::string group, std::string nacos_ip, HANDLE_KEY_CHANGED callback);

    /* S32 listen_to_key(std::string key, std::string nacos_namespace, std::string group, std::string nacos_ip, HANDLE_KEY_CHANGED callback);
     * 函数说明: 对配置项进行监控，可以使用自定义的命名空间
     * 入参:
     * 　　std::string key               : 配置项名称
     *     std::string nacos_namespace   : 配置项所属的 namespace 的 ID， 如 "744081e6-3bfb-4ad8-85ac-9398993fb288"，如果填写 "" 表示使用 public 空间
     *     std::string group             : 配置项所属的 group 的名称，比如 "DRC_GROUP"
     *     std::string nacos_ip          : nacos 服务器/集群的 IP, 如 "172.16.17.56"
     *     HANDLE_KEY_CHANGED callback   : 回调函数，当配置项发生变动时，此函数会被调用
     *
     * 返回值:
     * 　　 0:  ok
     *      其它: 失败　
     */
    S32 listen_to_key(std::string key, std::string nacos_namespace, std::string group, std::string nacos_ip, HANDLE_KEY_CHANGED callback);
    
    /* S32 listen_to_multi_keys(std::string nacos_namespace, std::string group, std::string nacos_ip, stKEY_WITH_HANDLER* pkh_array, S32 numbs);
     * 函数说明: 对多个配置项进行监控，多个监控项共用监控线程，减少对线程资源的占用
     * 入参:
     * 　　std::string nacos_namespace   : 配置项所属的 namespace 的 ID， 如 "744081e6-3bfb-4ad8-85ac-9398993fb288"，如果填写 "" 表示使用 public 空间
     *     std::string group             : 配置项所属的 group 的名称，比如 "DRC_GROUP"
     *     std::string nacos_ip          : nacos 服务器/集群的 IP, 如 "172.16.17.56"
     *     stKEY_WITH_HANDLER* pkh_array : 指针，指向 stKEY_WITH_HANDLER 类型的数组，该数组存放 key - callback 的配对
     *     S32 numbs                     : pkh_array 指向的数组的成员个数，即配置项-回调函数（key-callback）配对的个数
     *
     * 返回值:
     *
     * 　　 0:  ok
     *      其它: 失败　
     */
    S32 listen_to_multi_keys(std::string nacos_namespace, std::string group, std::string nacos_ip, stKEY_WITH_HANDLER* pkh_array, S32 numbs);
    
    /* S32 listen_to_key(std::string key, std::string group, std::string nacos_ip, HANDLE_KEYS_BY_ONE_CB callback);
     * 函数说明: 对配置项进行监控，使用缺省（public）的命名空间。注意，这里的回调函数是 HANDLE_KEYS_BY_ONE_CB，该函数会传递 key 的值，和key 本身的字符串
     *           这样做的目的，是可以实现多个 key 都注册同一个回调函数，由回调函数再根据 key 值来采用不同的处理代码，能节省一些回调函数定义。
     *
     * 入参:
     * 　　std::string key               : 配置项名称
     *     std::string group             : 配置项所属的 group 的名称，比如 "DRC_GROUP"
     *     std::string nacos_ip          : nacos 服务器/集群的 IP, 如 "172.16.17.56"
     *     HANDLE_KEY_CHANGED callback   : 回调函数，当配置项发生变动时，此函数会被调用
     *
     * 返回值:
     *
     * 　　 0:  ok
     *      其它: 失败　
     */
    S32 listen_to_key(std::string key, std::string group, std::string nacos_ip, HANDLE_KEYS_BY_ONE_CB callback);
    
    /* S32 listen_to_key(std::string key, std::string nacos_namespace, std::string group, std::string nacos_ip, HANDLE_KEYS_BY_ONE_CB callback);
     * 函数说明: 对配置项进行监控，可以使用自定义的命名空间。注意，这里的回调函数是 HANDLE_KEYS_BY_ONE_CB，该函数会传递 key 的值，和key 本身的字符串
     *           这样做的目的，是可以实现多个 key 都注册同一个回调函数，由回调函数再根据 key 值来采用不同的处理代码，能节省一些回调函数定义。
     *
     * 入参:
     * 　　std::string key               : 配置项名称
     *     std::string nacos_namespace   : 配置项所属的 namespace 的 ID， 如 "744081e6-3bfb-4ad8-85ac-9398993fb288"，如果填写 "" 表示使用 public 空间
     *     std::string group             : 配置项所属的 group 的名称，比如 "DRC_GROUP"
     *     std::string nacos_ip          : nacos 服务器/集群的 IP, 如 "172.16.17.56"
     *     HANDLE_KEY_CHANGED callback   : 回调函数，当配置项发生变动时，此函数会被调用
     *
     * 返回值:
     * 　　 0:  ok
     *      其它: 失败　
     */
    S32 listen_to_key(std::string key, std::string nacos_namespace, std::string group, std::string nacos_ip, HANDLE_KEYS_BY_ONE_CB callback);
        
        
private:
    std::atomic<bool> m_execute;
    std::thread m_thd;
    S32 _listen_to_key(std::string key, std::string nacos_namespace, std::string group, std::string nacos_ip, HANDLE_KEY_CHANGED callback);
    S32 _listen_to_key(std::string key, std::string nacos_namespace, std::string group, std::string nacos_ip, HANDLE_KEYS_BY_ONE_CB callback);
};

#endif
