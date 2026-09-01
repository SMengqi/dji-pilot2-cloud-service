#include <iostream>
#include <map>
#include "DrcNacosWrapper.h"

std::shared_ptr<DrcNacosWrapper> DrcNacosWrapper::ptrNacosClient = nullptr;


DrcNacosWrapper::DrcNacosWrapper()
{
    ;
}

DrcNacosWrapper::~DrcNacosWrapper()
{
    if(factory)
    {
        delete factory;
    }
}


std::shared_ptr<DrcNacosWrapper> DrcNacosWrapper::GetInstance()
{
    if (!ptrNacosClient)
    {
    	ptrNacosClient = std::shared_ptr<DrcNacosWrapper>(new DrcNacosWrapper());
    }

    return ptrNacosClient;
}

/*
* 函数：getSystermPort
* Note:
*      随机获取系统内的端口
*/
int DrcNacosWrapper::getSystermPort(int & port)
{
    int fd = -1;
    socklen_t addr_size = 0;

    struct sockaddr_in sin; 
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(0);
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
   
    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(fd < 0)
    {
        printf("socket() error:%s\n", strerror(errno));
        return -1;
    }
   
    if(bind(fd, (struct sockaddr *)&sin, sizeof(sin)) != 0)
    {
        printf("bind() error:%s\n", strerror(errno));
        close(fd);
        return -1;
    }

    addr_size = sizeof(sin);
    if(getsockname(fd, (struct sockaddr *)&sin, &addr_size) != 0)
    {
        printf("getsockname() error:%s\n", strerror(errno));
        close(fd);
        return -1;
    }

    port = sin.sin_port;
    if(fd != -1)
        close(fd);

    //std::cout<<"port:"<<port<<std::endl;

    return 0;
}

/*
* 函数：getSysPortRange
* Note:
*      获取系统的端口范围 
*/
char * DrcNacosWrapper::getSysPortRange()
{
	int fd[2];
    pipe(fd);
    int pid = fork();
    if(pid){
            char *buf = (char *)malloc(sizeof(char)*128);
            bzero(&buf, sizeof(char));
            int n = read(fd[0], buf, 128);
            return strtok(strstr(buf,"= ") + strlen("= "),"\r\n");
    }else{
            dup2(fd[1],STDOUT_FILENO);
            close(fd[1]);  close(fd[0]);
            system("sysctl net.ipv4.ip_local_port_range");
            exit(0);
    }
    return NULL;
}


/*
* 函数：CreateNacosClient
* Note:
*      创建客户端服务，该函数只用调用一次
*      client_receive_port:端口可以指定或者随机生成；指定端口的弊端是比较繁琐，但单台服务下多DRC也会容易冲突；随机生成端口也存在冲突的可能；
*/
int DrcNacosWrapper::CreateNacosClient(struct NACOS_CLIENT_PROPERTIES & propertise)
{
    if(factory!=nullptr || namingSvc!=nullptr)
    {
        return -1;
    }

    if(propertise.client_receive_port == "0")
    {
        int port;
        if(this->getSystermPort(port))
        {
            return -1;
        }
        propertise.client_receive_port = std::to_string(port);
    }

    Properties configProps;
    configProps[PropertyKeyConst::SERVER_ADDR] = propertise.nacos_server_addr;
    configProps[PropertyKeyConst::NAMESPACE] = propertise.nacos_namespace;
    configProps[PropertyKeyConst::UDP_RECEIVER_PORT] = propertise.client_receive_port;

    if(!propertise.subscription_poller_interval.empty())
    {
        configProps[PropertyKeyConst::SUBSCRIPTION_POLL_INTERVAL] = propertise.subscription_poller_interval;
    }

    //configProps[PropertyKeyConst::LOG_PATH] = "./";
    //configProps[PropertyKeyConst::LOG_LEVEL] = "DEBUG";


    factory = new NacosServiceFactory(configProps);
    //ResourceGuard <NacosServiceFactory> _guardFactory(factory);
    namingSvc = factory->CreateNamingService();

    if(factory==nullptr || namingSvc==nullptr)
    {
        return -1;
    }

    client_propertise = propertise;

    return 0;
}

/*
* RegisterInstance
* Note:
*   注册服务 
    metadata 扩展信息::
    preserved.heart.beat.timeout : 该实例在不发送心跳后，从健康到不健康的时间。（单位:毫秒） 
    preserved.ip.delete.timeout : 该实例在不发送心跳后，被服务端下掉该实例的时间。（单位:毫秒） 
    preserved.heart.beat.interval : 该实例在客户端上报心跳的间隔时间。（单位:毫秒） 
    preserved.instance.id.generator: 该实例的id生成策略，值为snowflake时，从0开始增加。 
    preserved.register.source : 保留键，目前未使用。
  注释：
    NACOS默认心跳5秒；如果心跳消失，15秒后实例健康状态置为false，30秒后被SERVER清掉；如果需要调整，
    则这三个参数建议同时修改，举例如下：
            instance_param.heart_beat_interval = "1000";
            instance_param.heart_beat_timeout = "3000";
            instance_param.ip_delete_timeout = "5000";

*/
int DrcNacosWrapper::RegisterInstance(struct NACOS_CLUSTER_INSTANCE  &instanceCfg)
{
    
    if(namingSvc == nullptr)
    {
        std::cout << "register instance fail, raison: namingSvc = nullptr"<< std::endl;
        return -1;
    }
    
    if(instanceCfg.instance_port == "0")
    {
        int port;
        if(this->getSystermPort(port))
        {
            return -1;
        }
        instanceCfg.instance_port = std::to_string(port);

    }

    Instance instance;
    instance.ip = instanceCfg.instance_ip;
    instance.port = std::atoi(instanceCfg.instance_port.c_str());
    instance.instanceId = instanceCfg.instance_id;
    instance.ephemeral = instanceCfg.instance_ephemeral;
    instance.clusterName = instanceCfg.cluster_name;

    std::map<NacosString, NacosString> metadata;

    if(!instanceCfg.heart_beat_interval.empty())
    {
        metadata.insert(std::make_pair("preserved.heart.beat.interval", instanceCfg.heart_beat_interval));
    }
    if(!instanceCfg.heart_beat_timeout.empty())
    {
        metadata.insert(std::make_pair("preserved.heart.beat.timeout", instanceCfg.heart_beat_timeout));
    }
    if(!instanceCfg.ip_delete_timeout.empty())
    {
        metadata.insert(std::make_pair("preserved.ip.delete.timeout", instanceCfg.ip_delete_timeout));            
    }
    if(!instanceCfg.register_source.empty())
    {
        metadata.insert(std::make_pair("preserved.register.source", instanceCfg.register_source)); 
    }

    if(!metadata.empty())
    {
        instance.metadata = metadata;
    }

    try 
    {   
        namingSvc->registerInstance(instanceCfg.service_name, client_propertise.nacos_group, instance);		
    }
    catch (NacosException &e) 
    {
        std::cout << "registerInstance fail, raison:"<< e.what() << std::endl;
        return -1;
    }

    return 0;
}

/*
* 函数：SubscribeService
* Note:
*      订阅服务 
*/
int DrcNacosWrapper::SubscribeService(std::string ServiceName)
{
    if(namingSvc == nullptr)
    {
        std::cout << "subscribe fail, raison: namingSvc = nullptr"<< std::endl;
        return -1;
    }

    try 
    {
        if(ServiceInstanceMap.find(ServiceName) != ServiceInstanceMap.end())
        {
            std::cout << "service not repeat subscribe, service name:"<< ServiceName << std::endl;
            return -1;
        }

        //namingSvc->subscribe(ServiceName, this);
        namingSvc->subscribe(ServiceName, client_propertise.nacos_group, this);

    }
    catch (NacosException &e) 
    {
        std::cout << "subscribe fail, raison:"<< e.what() << std::endl;
        return -1;
    }

    return 0;
}


/*
* 函数：GetServiceAllInstances
* Note:
*      获取服务的所有实例
*/
int  DrcNacosWrapper::GetServiceAllInstances(std::string ServiceName, std::list <Instance> &instanceInfos)
{
    try 
    {
        //instanceInfos = namingSvc->getAllInstances(ServiceName);
        instanceInfos = namingSvc->getAllInstances(ServiceName,client_propertise.nacos_group);
    }
    catch (NacosException &e) 
    {
        std::cout << "subscribe fail, raison:"<< e.what() << std::endl;
        return -1;
    }

    return 0;
}

/*
* 函数：updataClusterInstance
* Note:
*      更新集群实例信息
*/
void DrcNacosWrapper::updataClusterInstance(std::string serviceName)
{
    std::list <Instance> instances;

    GetServiceAllInstances(serviceName,instances);

    for (std::list<Instance>::iterator it = instances.begin();it != instances.end(); it++) 
    {
        std::cout << "Instance:"<<std::endl;
        std::cout<<"    ip:"<<it->ip<<std::endl;
        std::cout<<"    port:"<<it->port<<std::endl;
        std::cout<<"    healthy:"<<it->healthy<<std::endl;
        std::cout<<"    weight:"<<it->weight<<std::endl;
    } 

    if(instances.size()<=0)
    {//当ETOD异常下线后，清空订阅列表
        std::unique_lock<std::mutex> t_lock(m_mutex);
        auto IterService = ServiceInstanceMap.find(serviceName);
        std::cout << "DRC_INFO::Subscribe Instance Map Size="<<IterService->second.size()<<std::endl;
        if(IterService != ServiceInstanceMap.end())
        { //Offline
            auto & instanceMap = IterService->second;
            for(auto iNode : instanceMap)
            {
                std::cout<<"    Instance offline:"<<iNode.first<<std::endl;
            }
            IterService->second.clear();
        }
        t_lock.unlock();
        return ;
    }

    std::unique_lock<std::mutex> t_lock(m_mutex);
    auto IterService = ServiceInstanceMap.find(serviceName);
    if(IterService == ServiceInstanceMap.end())
    { //插入实例数据
        INSTANCE_INFO_MAP instanceMap;
        for (std::list<Instance>::iterator it = instances.begin();it != instances.end(); it++) 
        {
            std::string Key = it->ip + ":" + std::to_string(it->port);
            Instance instanceInfo = *it;
            instanceMap.insert({Key,instanceInfo});
        }
        ServiceInstanceMap.insert({serviceName,instanceMap});
        pollerInstanceMap.insert({serviceName,""});
        t_lock.unlock();
    }
    else
    {
        for (std::list<Instance>::iterator it = instances.begin();it != instances.end(); it++) 
        {
            std::string Key = it->ip + ":" + std::to_string(it->port);
            if(IterService->second.find(Key) == IterService->second.end())
            {//新增实例
                IterService->second.insert({Key,*it});
            }
            else 
            {
                if(it->healthy==false)
                {//实例下线
                    IterService->second.erase(Key);                  
                }
                else
                {//更新实例
                    IterService->second[Key] = *it;
                }
            }
        }
        t_lock.unlock();
    }
}

/*
* 函数：getHealthyInstance
* Note:
*      获取可以用的服务实例接口：ip port
*/
int DrcNacosWrapper::GetHealthyInstance(std::string serviceName, std::string &instanceIP, std::string &instancePort)
{
    std::unique_lock<std::mutex> t_lock(m_mutex);
    auto IterService = ServiceInstanceMap.find(serviceName);
    if(IterService == ServiceInstanceMap.end())
    {
        t_lock.unlock();
        std::cout << "DrcNacosWrapper::GetHealthyInstance: get config error, server name:" << serviceName << std::endl;
        return -1;
    }

    auto instance_info = IterService->second;
    if(instance_info.size()<=0)
    {
        return -1;
    }

    if(instance_info.size()==1)
    {
        instanceIP = instance_info.begin()->second.ip;
        instancePort = std::to_string(instance_info.begin()->second.port);
        pollerInstanceMap[serviceName] = instanceIP + ":" + instancePort;
        t_lock.unlock();
        return 0;
    }

    for (auto it = instance_info.begin();it != instance_info.end(); it++) 
    {
        if(pollerInstanceMap[serviceName].length() < 3)
        {
            instanceIP = it->second.ip;
            instancePort = std::to_string(it->second.port);
            pollerInstanceMap[serviceName] = instanceIP + ":" + instancePort;
            break;
        }

        if(pollerInstanceMap[serviceName] != it->first)
        {
            instanceIP = it->second.ip;
            instancePort = std::to_string(it->second.port);
            pollerInstanceMap[serviceName] = instanceIP + ":" + instancePort;
            break;
        }
    }

    t_lock.unlock();

    return 0;
}

/*
* 函数：receiveNamingInfo
* Note:
*      服务订阅的回调函数
*/
void DrcNacosWrapper::receiveNamingInfo(const ServiceInfo &serviceInfo)
{
    struct tm pstTmInfo;
    time_t pulTime = time(NULL);
    pstTmInfo = *localtime(&pulTime); //2022-10-25 00:45:00

    char local_timestamp[256] = {0};
    int ulSpLen = sprintf((char*)local_timestamp, "%04d-%02d-%02d %02d:%02d:%02d",
        pstTmInfo.tm_year + 1900, pstTmInfo.tm_mon + 1, pstTmInfo.tm_mday,
        pstTmInfo.tm_hour, pstTmInfo.tm_min, pstTmInfo.tm_sec);    

    std::string strTime;
    strTime.assign(local_timestamp, ulSpLen);

    std::cout << "===================================" << std::endl;
    //std::cout << "Watched service UPDATED: " << serviceInfo.toInstanceString() << std::endl;
    std::cout<<"updateTime:"<<strTime<<std::endl;
    std::cout<<"serverName:"<<serviceInfo.getKey()<<std::endl;

    this->updataClusterInstance(serviceInfo.getKey());
    
}
