#define THIS_MODULE MODULE_NETWORK
#include  <stdio.h>     
#include  <sys/types.h>     
#include  <sys/param.h>     
 
#include  <sys/ioctl.h>     
#include  <sys/socket.h>     
#include  <net/if.h>     
#include  <netinet/in.h>     
#include  <net/if_arp.h>
#include <linux/rtnetlink.h>    //for rtnetlink
#include  <arpa/inet.h> 
#include  <unistd.h>
#include <stdlib.h> //for malloc(), free()  
#include "pl.h" //for pf_malloc, pf_free
#include  <string.h>
#include  <string>
#include "platform_net_tools.h"
#include "ipv4address.h"

#include <fcntl.h>    
#include <semaphore.h>

bool FindIpInformation( const std::string& ipaddr , IpInformation& info )
{
    std::list<InterfaceInformation> intfInformationList;

    if( true == GetAllIpInformation( intfInformationList ) )
    {
        std::list<InterfaceInformation>::iterator itor = intfInformationList.begin();
        for( ; itor != intfInformationList.end() ; itor++ )
        {
            std::list<IpPair>::iterator ipitor = itor->m_ipAddressList.begin();
            for( ; ipitor != itor->m_ipAddressList.end() ; ipitor++ )
            {
                if( ipitor->m_ip == ipaddr )
                {
                    info.m_ip = ipaddr;
                    info.m_ipValue = IPv4Address::ConvertToInteger( ipaddr.c_str() );
                    info.m_mask = ipitor->m_mask;
                    info.m_maskValue = IPv4Address::ConvertToInteger( info.m_mask.c_str() );
                    info.m_interfaceName = itor->m_interfaceName;
                    info.m_interfaceIndex = itor->m_interfaceIndex;
                    memcpy( info.m_mac , itor->m_mac , 6 );
                    return true;
                }
            }
        }
    }

    PS_CPlus(CM_NES, CMNES_ID_TOOLS_FIND_IP_FAIL);
    return false;
}

bool GetAllIpInformation( std::list<InterfaceInformation>& ipInformationList )
{
    int  MAXINTERFACES=16;
    char *ip=NULL;
    int fd, intrface, retn = 0;
    struct ifreq buf[MAXINTERFACES];
    struct ifconf ifc;
    if ((fd = socket (AF_INET, SOCK_DGRAM, 0)) >= 0)
    {
        ifc.ifc_len = sizeof(buf);
        ifc.ifc_buf = (caddr_t) buf;
        if (!ioctl (fd, SIOCGIFCONF, (char *) &ifc))
        {
            intrface = ifc.ifc_len / sizeof (struct ifreq);
            char mac[64]={0};
        
            while (intrface-->0)
            {
                InterfaceInformation info;
                memset( info.m_mac , 0 , sizeof(info.m_mac) );
                bool haveintf = false;
                if (!(ioctl (fd, SIOCGIFADDR, (char *) &buf[intrface])))
                {
                    info.m_interfaceName = buf[intrface].ifr_name;
                    info.m_interfaceIndex  = buf[intrface].ifr_ifindex;

                    IpPair pair;
                    pair.m_ip = inet_ntoa(((struct sockaddr_in*)(&buf[intrface].ifr_addr))->sin_addr);
                    
                    
                    if (!(ioctl (fd, SIOCGIFNETMASK, (char *) &buf[intrface])))
                    {
                        pair.m_mask = inet_ntoa(((struct sockaddr_in*)(&buf[intrface].ifr_netmask))->sin_addr);
                        info.m_ipAddressList.push_back( pair );
                    }
                    
                    if (!(ioctl (fd, SIOCGIFHWADDR, (char *) &buf[intrface])))
                    {
                        unsigned char* byte = (unsigned char*)&buf[intrface].ifr_hwaddr.sa_data[0];
                        sprintf( mac , "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x",
                        byte[0],byte[1],byte[2],byte[3],byte[4],byte[5]);
                        info.m_macAddress = mac;
                        memcpy( (char*)&info.m_mac[0] , byte , 6 );
                    }
                    
                    ipInformationList.push_back( info );
                }
            }
        }
        else
        {
            PS_CPlus(CM_NES, CMNES_ID_TOOLS_GETALLIP_IOCTRL_FAIL);
            close(fd);
            return false;
        }
        close (fd);
    }
    else
    {
        PS_CPlus(CM_NES, CMNES_ID_TOOLS_GETALLIP_SOCKET_FAIL);
        return false;
    }
    
    return true;
}

enum ParseState
{
    ParseState_ExpectIP             = 0,

    ParseState_ExpectMac            ,
    ParseState_Mac                    ,
    ParseState_NewLine                ,

};

bool IsNumber( char c )
{
    if( c >= '0' && c <= '9' )
        return true;
    return false;
}

bool IsHex( char c )
{
    if( ( c >= 'A' && c <= 'F' ) || ( c >= 'a' && c <= 'f' ) || ( c >= '0' && c <= '9' ) )
        return true;
    return false;
}

bool MacConvertToInteger( const std::string& macString , uint8_t* mac )
{
    int  n;
    char dummy;

    const char* str = macString.c_str();
    uint32_t local[6];
    n = sscanf(macString.c_str(), "%2x:%2x:%2x:%2x:%2x:%2x", &local[0], &local[1], &local[2], &local[3], 
        &local[4] , &local[5] );
    if( n == 6 )
    {
        for( uint32_t index = 0 ; index < 6; index++ )
        {
            mac[index] = local[index];
        }
        return true;
    }
    else
    {
        PS_CPlus(CM_NES, CMNES_ID_TOOLS_MAC_CONVERT_FAIL);
        return false;
    }
} 
bool ParseArpTable( std::string& arpString , std::list<ArpInformation>& arpInformationList )
{
    int i = 0;
    int size = arpString.size();
    int start = 0 , end = 0;
    
    int state = ParseState_NewLine;
    const char* str = arpString.c_str();
    ArpInformation arp;
    while( i < size )
    {
        switch( state )
        {
        case ParseState_ExpectIP:
            if( IsNumber( str[i] ) || str[i]== '.' )
            {
                end++;
            }
            else
            {
                if( end > start )
                {
                    std::string ip;
                    ip.assign( &str[start] , end - start );
                    uint32_t value = IPv4Address::ConvertToInteger( ip.c_str() );
                    if( value == 0 )
                    {
                        state = ParseState_NewLine;
                    }
                    else
                    {
                        state = ParseState_ExpectMac;
                        arp.dwAddr = value;
                    }
                    start = end+1;
                    end = start;
                }
            }
            
            break;
        case ParseState_ExpectMac:
            if( true == IsHex( str[i] ) )
            {
                start = i;
                end = i;
                state = ParseState_Mac;
            }
            break;
        case ParseState_Mac:
            {
                if( str[i] != ':' && false == IsHex( str[i] ) )
                {
                    if( end > start )
                    {
                        std::string mac;
                        mac.assign( &str[start] , end - start + 1 );
                        if( MacConvertToInteger( mac , &arp.physAddr[0]))
                        {
                            state = ParseState_NewLine;
                            arpInformationList.push_back( arp );
                        }
                        else
                        {
                            start = i+1;
                            end   = i+1;
                            state = ParseState_ExpectMac;
                        }
                    }
                    else
                    {
                        state = ParseState_ExpectMac;
                    }
                }
                else
                {
                    end++;
                }
            }
            break;
        case ParseState_NewLine:
            if( str[i] == '\n' )
            {
                start = i+ 1;
                end = i + 1;
                state = ParseState_ExpectIP;
            }
            break;
        }
        i++;
    }
}

bool GetArpTableInformation( std::list<ArpInformation>& arpInformationList )
{
    FILE* fp = fopen( "/proc/net/arp", "rb" );
    if( NULL != fp )
    {
        std::string str;
        char buffer[1024]={0};
        
        int n = 0;
        do 
        {
            n = fread( buffer , 1 , sizeof(buffer)-1 ,  fp );
            if( n > 0 )
                str += buffer;
        }while( n > 0);
        
        fclose(fp );
        
        pl_log(INF, "%s" , str.c_str() );
        ParseArpTable( str , arpInformationList );
        
        std::list<ArpInformation>::iterator itor = arpInformationList.begin();
        for( ; itor != arpInformationList.end() ; itor++ )
        {
            pl_log(INF, "arp table:%d" , itor->dwAddr );
        }
        
    }
    return true;
}


#define BUFSIZE 8192  
   
struct route_info{  
 u_int dstAddr;  
 u_int srcAddr;  
 u_int gateWay;  
 char ifName[IF_NAMESIZE];  
};  
int readNlSock(int sockFd, char *bufPtr, int seqNum, int pId)  
{  
  struct nlmsghdr *nlHdr;  
  int readLen = 0, msgLen = 0;  
  do{  
    
    if((readLen = recv(sockFd, bufPtr, BUFSIZE - msgLen, 0)) < 0)  
    {  
        pl_log(ERR, "SOCK READ: ");  
        PS_CPlus(CM_NES, CMNES_ID_TOOLS_READNI_RECV_FAIL);
        return -1;  
    }  
     
    nlHdr = (struct nlmsghdr *)bufPtr;  
    
    if((NLMSG_OK(nlHdr, readLen) == 0) || (nlHdr->nlmsg_type == NLMSG_ERROR))  
    {  
        pl_log(ERR, "Error in recieved packet");  
        PS_CPlus(CM_NES, CMNES_ID_TOOLS_READNI_NLMSG_FAIL);
        return -1;  
    }  
     
      
    if(nlHdr->nlmsg_type == NLMSG_DONE)   
    {  
        break;  
    }  
    else  
    {  
        bufPtr += readLen;  
        msgLen += readLen;  
    }  
     
      
    if((nlHdr->nlmsg_flags & NLM_F_MULTI) == 0)   
    {  
        break;  
    }  
  } while((nlHdr->nlmsg_seq != seqNum) || (nlHdr->nlmsg_pid != pId));  
  
  return msgLen;  
}  

void parseRoutes(struct nlmsghdr *nlHdr, struct route_info *rtInfo,char *gateway)  
{  
    struct rtmsg *rtMsg;  
    struct rtattr *rtAttr;  
    int rtLen;  
    struct in_addr dst;  
    struct in_addr gate;  

    rtMsg = (struct rtmsg *)NLMSG_DATA(nlHdr);  
    // If the route is not for AF_INET or does not belong to main routing table  
    //then return.   
    if((rtMsg->rtm_family != AF_INET) || (rtMsg->rtm_table != RT_TABLE_MAIN))  
    {
        PS_CPlus(CM_NES, CMNES_ID_TOOLS_PARSE_ROUTES_FAIL);
        return;  
    }

    rtAttr = (struct rtattr *)RTM_RTA(rtMsg);  
    rtLen = RTM_PAYLOAD(nlHdr);  
    for(;RTA_OK(rtAttr,rtLen);rtAttr = RTA_NEXT(rtAttr,rtLen))
    {  
        switch(rtAttr->rta_type) 
        {  
            case RTA_OIF:  
                if_indextoname(*(int *)RTA_DATA(rtAttr), rtInfo->ifName);  
                break;  
            case RTA_GATEWAY:  
                rtInfo->gateWay = *(u_int *)RTA_DATA(rtAttr);  
                break;  
            case RTA_PREFSRC:  
                rtInfo->srcAddr = *(u_int *)RTA_DATA(rtAttr);  
                break;  
            case RTA_DST:  
                rtInfo->dstAddr = *(u_int *)RTA_DATA(rtAttr);  
                break;  
        }  
    }  
    dst.s_addr = rtInfo->dstAddr;  
    if (strstr((char *)inet_ntoa(dst), "0.0.0.0"))  
    {  
        //printf("oif:%s",rtInfo->ifName);  
        gate.s_addr = rtInfo->gateWay;  
        sprintf(gateway, (char *)inet_ntoa(gate));  
        //printf("%s\n",gateway);  
        gate.s_addr = rtInfo->srcAddr;  
        //printf("src:%s\n",(char *)inet_ntoa(gate));  
        gate.s_addr = rtInfo->dstAddr;  
        //printf("dst:%s\n",(char *)inet_ntoa(gate));   
    }  

    return;  
}

int get_gateway(char *gateway)  
{  
    struct nlmsghdr *nlMsg;  
    struct rtmsg *rtMsg;  
    struct route_info *rtInfo;  
    char msgBuf[BUFSIZE];  

    int sock, len, msgSeq = 0;  
  
    if((sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE)) < 0)  
    {   
        PS_CPlus(CM_NES, CMNES_ID_TOOLS_GATEWAY_SOCK_FAIL);
        return -1;  
    }  
      
    pf_memset(msgBuf, 0, BUFSIZE);  
      
    nlMsg = (struct nlmsghdr *)msgBuf;  
    rtMsg = (struct rtmsg *)NLMSG_DATA(nlMsg);  
      
    nlMsg->nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg)); // Length of message.  
    nlMsg->nlmsg_type = RTM_GETROUTE; // Get the routes from kernel routing table .  
   
    nlMsg->nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST; // The message is a request for dump.  
    nlMsg->nlmsg_seq = msgSeq++; // Sequence of the message packet.  
    nlMsg->nlmsg_pid = getpid(); // PID of process sending the request.  
      
    if(send(sock, nlMsg, nlMsg->nlmsg_len, 0) < 0)
    {  
        PS_CPlus(CM_NES, CMNES_ID_TOOLS_GATEWAY_SEND_FAIL);
        return -1;  
    }  
      
    if((len = readNlSock(sock, msgBuf, msgSeq, getpid())) < 0) 
    {  
        PS_CPlus(CM_NES, CMNES_ID_TOOLS_GATEWAY_READNL_FAIL);
        pl_log(ERR, "Read From Socket Failed¡­");  
        return -1;  
    }  
   
    rtInfo = (struct route_info *)malloc(sizeof(struct route_info));  
    for(;NLMSG_OK(nlMsg,len);nlMsg = NLMSG_NEXT(nlMsg,len))
    {  
        memset(rtInfo, 0, sizeof(struct route_info));  
        parseRoutes(nlMsg, rtInfo,gateway);  
    }  
    pf_free(rtInfo);  
    close(sock);  
    return 0;  
}

std::string GetGateway()
{
    std::string gateway;
    char buff[256]={0};  
    if( 0 == get_gateway(buff) )
    {
        gateway = buff;
    }
    return gateway;
}
void cross_sleep( uint32_t millseconds )
{
    usleep( millseconds * 1000 );
}

std::string GetMainIP()
{
    std::string mainIP;
    std::list<InterfaceInformation> intfInformationList;
    GetAllIpInformation( intfInformationList );
    
    std::list<InterfaceInformation>::iterator itor = intfInformationList.begin();
    for( ; itor != intfInformationList.end(); itor++ )
    {
        if( !(*itor).m_ipAddressList.empty() )
        {
            IpPair pair = *(*itor).m_ipAddressList.begin();
            if( pair.m_ip != "127.0.0.1" )
            {
                mainIP = pair.m_ip;
                break;
            }
        }
    }
    
    return mainIP;
}


std::string GetMainIPfromDev(std::string dev)
{
    std::string mainIP;
    std::list<InterfaceInformation> intfInformationList;
    GetAllIpInformation( intfInformationList );

    std::list<InterfaceInformation>::iterator itor = intfInformationList.begin();
    for( ; itor != intfInformationList.end(); itor++ )
    {
        if ((*itor).m_interfaceName == dev)
        {
            if( !(*itor).m_ipAddressList.empty() )
            {
                IpPair pair = *(*itor).m_ipAddressList.begin();
                if( pair.m_ip != "127.0.0.1" )
                {
                    mainIP = pair.m_ip;
                    break;
                }
            }
        }
    }

    return mainIP;
}



bool CreateNameSemaphore(const char* name )
{
    sem_t * sem = sem_open(name, O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH, 0);  
    if(sem == SEM_FAILED)
    {
        PS_CPlus(CM_NES, CMNES_ID_TOOLS_CREATE_SEM_FAIL);
        return false;
    }
    return true;
}