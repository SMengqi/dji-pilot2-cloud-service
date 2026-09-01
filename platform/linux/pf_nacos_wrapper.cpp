#include <iostream>
#include <atomic>
#include <thread>
#include "factory/NacosServiceFactory.h"
#include "ResourceGuard.h"
#include "constant/PropertyKeyConst.h"
#include "listen/Listener.h"
#include "pf_nacos_wrapper.h"

#include "Nacos.h"
#include <naming/NamingProxy.h>
#include <naming/NacosNamingService.h>
#include <utils/NamingUtils.h>


#define THIS_MODULE PLATFORM_EX
#include "pl.h"


using namespace std;
using namespace nacos;
#define GET_TIMEOUT 3000
#define TRY_ROUNDS  10


S32 nacos_setconfig(string key, string value, string nacos_namespace, string group, string nacos_ip)
{
    Properties props;
    props[PropertyKeyConst::SERVER_ADDR] = nacos_ip;
    props[PropertyKeyConst::NAMESPACE] = nacos_namespace;
    NacosServiceFactory *factory = new NacosServiceFactory(props);
    ResourceGuard <NacosServiceFactory> _guardFactory(factory);
    ConfigService *n = factory->CreateConfigService();
    ResourceGuard <ConfigService> _serviceFactory(n);
    bool bSucc = false;

    bSucc = n->publishConfig(key, group, value);
           
    //count << "Publishing Key result:" << bSucc << endl;
    
	return (bSucc ? 0 : -1);
}

/* S32 pl_nacos_setconfig(std::string key, std::string value, std::string group, std::string nacos_ip);
 *     std::string key       : 配置项的名称，如 DRC_1_CONFIG_2
 *     std::string value     : 配置项的值, 如 12345 
 *     std::string group     : 配置项所属的 group 的名称，比如 "DRC_GROUP"
 *     std::string nacos_ip  : nacos 服务器/集群的 IP, 如 "172.16.17.56"
 * 返回值:
 * 　　 0:  ok
 *      其它: 失败　
 */
S32 pl_nacos_setconfig(string key, string value, string group, string nacos_ip) 
{
    return nacos_setconfig(key, value, "Public", group, nacos_ip);
}

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
S32 pl_nacos_setconfig(string key, string value, string nacos_namespace, string group, string nacos_ip) 
{
	return nacos_setconfig(key, value, nacos_namespace, group, nacos_ip);
}

string nacos_getconfig(string key, string nacos_namespace, string group, string nacos_ip)
{
    Properties props;
    props[PropertyKeyConst::SERVER_ADDR] = nacos_ip;
    props[PropertyKeyConst::NAMESPACE] = nacos_namespace;
    NacosServiceFactory *factory = new NacosServiceFactory(props);
    ResourceGuard <NacosServiceFactory> _guardFactory(factory);
    ConfigService *n = factory->CreateConfigService();
    ResourceGuard <ConfigService> _serviceFactory(n);
    NacosString ss = "";
    try {
        ss = n->getConfig(key, group, GET_TIMEOUT);
    }
    catch (NacosException &e) {
        pl_log(ERR, "Request failed with curl code: %d, Reason: %s", e.errorcode(), e.what());
    }
    
    //count << ss << endl;
    
    return ss;
}

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
string pl_nacos_getconfig(string key, string group, string nacos_ip)
{
    return nacos_getconfig(key, "Public", group, nacos_ip);
}


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
string pl_nacos_getconfig(string key, string nacos_namespace, string group, string nacos_ip)
{
    return nacos_getconfig(key, nacos_namespace, group, nacos_ip);
}





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

int pl_nacos_register_instance(std::string &server_addr, std::string &nacos_namespace, std::string &service_name, std::string &metadata_value, std::string &service_address, int service_port)
{
    Instance instance;
    Properties configProps;

    configProps[PropertyKeyConst::SERVER_ADDR] = server_addr;
    configProps[PropertyKeyConst::NAMESPACE] = nacos_namespace;
    configProps[PropertyKeyConst::UDP_RECEIVER_PORT] = service_port;
    //configProps[PropertyKeyConst::LOG_PATH] = "./";
    //configProps[PropertyKeyConst::LOG_LEVEL] = "DEBUG";

    INacosServiceFactory *factory = NacosFactoryFactory::getNacosFactory(configProps);
    if(!factory) {
		std::cout << "nacos factory error server_addr" << server_addr << nacos_namespace << service_port << std::endl;
        return -1;
    }

    //ResourceGuard <INacosServiceFactory> _guardFactory(factory);
    NamingService *namingSvc = factory->CreateNamingService();
    if(!namingSvc) {
		std::cout << "nacos CreateNamingService error server_addr" << server_addr << nacos_namespace << service_port << std::endl;
        return -1;
    }

    //ResourceGuard <NamingService> _serviceFactory(namingSvc);
    
    instance.clusterName = "DefaultCluster";
    instance.ip = service_address;
    instance.port = service_port;
    instance.instanceId = "1";
    instance.ephemeral = true;
    instance.healthy = true;
    map<NacosString, NacosString> metadata;
    metadata.insert(make_pair("preserved.register.source", metadata_value));
    instance.metadata = metadata;
    instance.namespaceId = nacos_namespace;
    instance.serviceName = service_name;
  
    try {
        namingSvc->registerInstance(service_name, instance);
 
	    // 启动一个线程执行心跳任务
		//std::thread t(nacos_heartbeat, std::ref(instance.serviceName), std::ref(std::ref(instance.ip)), instance.port);
        //t.detach();
    }
    catch (NacosException &e) {
        cout << "encounter exception while registering service instance, raison:" << e.what() << endl;
        return -1;
    }
    return 0;
}


class _wrapper_listener : public Listener
{
private:	
    HANDLE_KEY_CHANGED callback_function;
    HANDLE_KEYS_BY_ONE_CB mulit_key_one_cb;
    string m_key;
    
public:
    _wrapper_listener(HANDLE_KEY_CHANGED fun)
    {
	    callback_function = fun;
    }
	
	_wrapper_listener(HANDLE_KEYS_BY_ONE_CB fun, string key)
    {
	    mulit_key_one_cb = fun;
	    m_key = key;
    }
    
    void receiveConfigInfo(const NacosString &configInfo) 
    {
        //count << "===================================" << endl;
        if (m_key.empty())
        {
		    callback_function(configInfo);
		}
		else
		{
			mulit_key_one_cb(configInfo, m_key);
		}
    }
};

pl_nacos_listener::pl_nacos_listener()
{
    m_execute = false;
}

pl_nacos_listener::~pl_nacos_listener()
{
    if (m_execute.load(std::memory_order_acquire)) 
    {
        m_execute.store(false, std::memory_order_release);
        if (m_thd.joinable())
        {
            m_thd.join();
        }
    }
}

S32 pl_nacos_listener::_listen_to_key(string key, string nacos_namespace, string group, string nacos_ip, HANDLE_KEY_CHANGED callback)
{
    m_thd = std::thread([=]()
    {
	Properties props;
        props[PropertyKeyConst::SERVER_ADDR] = nacos_ip;
        props[PropertyKeyConst::NAMESPACE] = nacos_namespace;
        NacosServiceFactory *factory = new NacosServiceFactory(props);
        ResourceGuard <NacosServiceFactory> _guardFactory(factory);
        ConfigService *n = factory->CreateConfigService();
        ResourceGuard <ConfigService> _serviceFactory(n);

        _wrapper_listener *theListener = new _wrapper_listener(callback);//You don't need to free it, since it will be deleted by the function removeListener
		n->addListener(key, group, theListener);//All changes on the key dqid will be received by MyListener
		m_execute.store(true, std::memory_order_release);
		
        while (m_execute.load(std::memory_order_acquire)) 
        {
                 
            std::this_thread::sleep_for(
            std::chrono::milliseconds(SLEEP_INTERVAL));
        }
        //count <<  "Listening thread say goodbye---------" << endl;
    });
    
    return 0;	
}

S32 pl_nacos_listener::_listen_to_key(string key, string nacos_namespace, string group, string nacos_ip, HANDLE_KEYS_BY_ONE_CB callback)
{
    m_thd = std::thread([=]()
    {
	Properties props;
        props[PropertyKeyConst::SERVER_ADDR] = nacos_ip;
        props[PropertyKeyConst::NAMESPACE] = nacos_namespace;
        NacosServiceFactory *factory = new NacosServiceFactory(props);
        ResourceGuard <NacosServiceFactory> _guardFactory(factory);
        ConfigService *n = factory->CreateConfigService();
        ResourceGuard <ConfigService> _serviceFactory(n);

        _wrapper_listener *theListener = new _wrapper_listener(callback, key);//You don't need to free it, since it will be deleted by the function removeListener
		n->addListener(key, group, theListener);//All changes on the key dqid will be received by MyListener
		m_execute.store(true, std::memory_order_release);
		
        while (m_execute.load(std::memory_order_acquire)) 
        {
                 
            std::this_thread::sleep_for(
            std::chrono::milliseconds(SLEEP_INTERVAL));
        }
        //count <<  "Listening thread say goodbye---------" << endl;
    });
    
    return 0;	
}
S32 pl_nacos_listener::listen_to_key(string key, string group, string nacos_ip, HANDLE_KEY_CHANGED callback)
{               
    return _listen_to_key(key, "Public", group, nacos_ip, callback);
}

S32 pl_nacos_listener::listen_to_key(string key, string group, string nacos_ip, HANDLE_KEYS_BY_ONE_CB callback)
{               
    return _listen_to_key(key, "Public", group, nacos_ip, callback);
}

S32 pl_nacos_listener::listen_to_key(string key, string nacos_namespace, string group, string nacos_ip, HANDLE_KEY_CHANGED callback)
{               
    return _listen_to_key(key, nacos_namespace, group, nacos_ip, callback);
}

S32 pl_nacos_listener::listen_to_key(string key, string nacos_namespace, string group, string nacos_ip, HANDLE_KEYS_BY_ONE_CB callback)
{               
    return _listen_to_key(key, nacos_namespace, group, nacos_ip, callback);
}

S32 pl_nacos_listener::listen_to_multi_keys(string nacos_namespace, string group, string nacos_ip, stKEY_WITH_HANDLER* pkh_array, S32 numbs)
{
    m_thd = std::thread([=]()
    {
		Properties props;
        props[PropertyKeyConst::SERVER_ADDR] = nacos_ip;
        
        if (nacos_namespace.empty())
        {
			props[PropertyKeyConst::NAMESPACE] = "Public";
		}
        else
        {
			props[PropertyKeyConst::NAMESPACE] = nacos_namespace;
		}
		
        NacosServiceFactory *factory = new NacosServiceFactory(props);
        ResourceGuard <NacosServiceFactory> _guardFactory(factory);
        ConfigService *n = factory->CreateConfigService();
        ResourceGuard <ConfigService> _serviceFactory(n);
        
        stKEY_WITH_HANDLER* p = pkh_array;
        
	    for (int i = 0; i < numbs ; i ++)
	    {
            _wrapper_listener *theListener = new _wrapper_listener(p->func);//You don't need to free it, since it will be deleted by the function removeListener
		    n->addListener(p->key, group, theListener);//All changes on the key dqid will be received by MyListener	
		    sleep(1);
		    p++;
	    }

		m_execute.store(true, std::memory_order_release);
		
        while (m_execute.load(std::memory_order_acquire)) 
        {
                 
            std::this_thread::sleep_for(
            std::chrono::milliseconds(SLEEP_INTERVAL));
        }
        //count <<  "Listening thread say goodbye---------" << endl;
    });
    
	return 0;		
}
