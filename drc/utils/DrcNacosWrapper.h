#ifndef _PL_NACOS_WRAPPER_H_
#define _PL_NACOS_WRAPPER_H_
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <chrono>
#include <thread>
#include <vector>
#include <map>
#include <ctime>
#include <atomic>
#include <memory>
#include <functional>
#include <condition_variable>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "factory/NacosServiceFactory.h"
#include "ResourceGuard.h"
#include "naming/Instance.h"
#include "NacosString.h"
#include "Properties.h"
#include "constant/PropertyKeyConst.h"

using namespace nacos;

/*
1.服务注册，
2.服务反注册，
3.服务订阅及监听
4.服务取消订阅
5.获取某个服务的所有实例
*/

//nacos客户端属性
struct NACOS_CLIENT_PROPERTIES
{
    std::string nacos_server_addr;
    std::string nacos_namespace;
    std::string nacos_group;
    std::string client_receive_port;
    std::string subscription_poller_interval;
};


//集群实例配置
struct NACOS_CLUSTER_INSTANCE
{
    std::string cluster_name;
    std::string instance_ip;
    std::string instance_port;
    std::string instance_id;
    bool instance_ephemeral;
    std::string service_name;
    std::string heart_beat_interval;
    std::string heart_beat_timeout;
    std::string ip_delete_timeout;
    std::string register_source;
};

struct CLUSTER_INSTANCE_INFO
{
    std::mutex m_mutex;
    std::map<std::string, nacos::Instance> instanceMap; //<IP:PORT , 实例信息>
};

typedef std::map<std::string, nacos::Instance> INSTANCE_INFO_MAP; //<IP:PORT , 实例信息>


class DrcNacosWrapper : public EventListener
{
public:
    static std::shared_ptr<DrcNacosWrapper> GetInstance();
    DrcNacosWrapper();
	~DrcNacosWrapper();

public:

    int CreateNacosClient(struct NACOS_CLIENT_PROPERTIES & propertise); //创建服务

    int RegisterInstance(struct NACOS_CLUSTER_INSTANCE  & instanceCfg); //注册服务实例

    int SubscribeService(std::string ServiceName); //订阅服务

    int GetServiceAllInstances(std::string ServiceName, std::list <Instance> &instanceInfos); //获取某个服务的所有实例

    int GetHealthyInstance(std::string serviceName, std::string &instanceIP, std::string &instancePort);

    /*
    ** 以下接口功能目前用不到，为了避免封装结构太过繁杂，目前先不做处理；后续根据需求在做个定制开发；
    **
    int RegisterInstance(std::vector<struct NACOS_CLUSTER_INSTANCE>  & VecInstanceCfg); //注册服务实例

    int DeregisterInstance(struct NACOS_CLUSTER_INSTANCE  & instanceCfg); //取消服务实例

    int UnSubscribeService(std::string ServiceName); //取消订阅服务    
    */

private:
    void receiveNamingInfo(const ServiceInfo &serviceInfo);

    void updataClusterInstance(std::string serviceName);

    char *getSysPortRange();

    int getSystermPort(int & port);

private:
    int num;

    std::mutex m_mutex;

    std::thread mytobj;

    NacosServiceFactory *factory = nullptr;

    NamingService *namingSvc = nullptr;

    struct NACOS_CLIENT_PROPERTIES client_propertise;

    std::map<std::string ,NACOS_CLIENT_PROPERTIES> ServiceClientCfg; //<订阅的服务名称，连接配置>

    std::map<std::string, INSTANCE_INFO_MAP> ServiceInstanceMap; //<订阅的服务名称，对应的实例>

    std::map<std::string, std::string> pollerInstanceMap; //<订阅的服务名称，当前实例>

    static std::shared_ptr<DrcNacosWrapper> ptrNacosClient;
};


#endif