/*FtpGetRun*/  
#define THIS_MODULE MODULE_FTP

#include <sys/types.h>  
#include <sys/socket.h>  
  
#include <netinet/in.h>  
  
#include <arpa/inet.h>  
  
#include <fcntl.h>  
  
#include <unistd.h>  
  
#include <stdarg.h>  
  
#include <stdio.h>  
  
#include <netdb.h>  

#include "pl.h"
#include "pf_upgrade.h"
#include "event.h"
#include "errid.h"
#include "pf_crypt.h"
#include "pf_ftp.h"


  
FILE *pFtpIOFile = NULL;  
  
char aFtpBuffer[4096];  

typedef struct
{
    S8 aucFtpIpAddr[64];                   // the size of the buff
    S8 aucFtpUserName[64];                  // the total num of buff
    S8 aucFtpPassword[64];
    U32 usFtpPortNum;                  // the num which is available for malloc
}DESCRIPTION_HEADER_S;

 
DESCRIPTION_HEADER_S m_stHeaderInfo;

/**********************************************************************
Function:
    void ftp_init(void)
Description: 
    该函数在系统启动时由平台调用，
    用于ftp线程的初始
Input:
    void
Output:
    void
Return: 
   MC_SUCC
Others:        
************************************************************************/
S32 ftp_init(U32 ulModuleId)
{

    pl_log(INF, "ftp init success %d!", ulModuleId);

    return PF_RET_SUCCESS;
}


/**********************************************************************
Function:
    int ftp_entry(U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, 
                          void* pcvMsg, U32 ulLength)
Description: 
    FTP线程消息入口参数，由平台调用
Input:
    ulSrcModuleId: source module identification
    ulMsgId: message identification
    ulDstModuleId: 目的模块标识号
    pcvMsg: 消息体地址 
    ulLength: 消息体长度 
Output:
    void
Return: 
    0-success
    other-failure
Others:        
************************************************************************/
void ftp_entry(U32 ulSrcModuleId,
                U32 ulMsgId,
                U32 ulDstModuleId, 
                void* pcvMsg,
                U32 ulLength)
{
    switch(ulMsgId)
    {
        case OAM_FTP_GET_NEW_FILE_REQ:
        {
            OAM_FTP_GET_NEW_FILE_REQ_MSG* pstGetFileReq = (OAM_FTP_GET_NEW_FILE_REQ_MSG*)pcvMsg;
            FTP_OAM_GET_NEW_FILE_RSP_MSG stGetFileRsp;

            S8 ascFtpFilePath[FILE_PATH_LEN];
            S8 ascLocalFilePath[FILE_PATH_LEN];
            sprintf((CHAR*)ascFtpFilePath, "%s%s", pstGetFileReq->stFtpAddress.ascPath, pstGetFileReq->ascVersionNo);
            sprintf((CHAR*)ascLocalFilePath, "%s%s%s",              \
                    pf_get_root_path(),                             \
                    FTP_ROOT_PATH, pstGetFileReq->ascVersionNo);

            /*check the IP address*/
            if(!pf_get_inet_aton((char *)pstGetFileReq->stFtpAddress.ascDomainName))
            {
                stGetFileRsp.slRlt = pf_get_ftp_file_pasv_info(         \
                        ascFtpFilePath,                             \
                        ascLocalFilePath,                           \
                        pstGetFileReq->ascLoginName,                \
                        pstGetFileReq->ascPassword,                 \
                        pstGetFileReq->stFtpAddress.ascDomainName,  \ 
                        pstGetFileReq->stFtpAddress.ulPortId);

                /*check the result of ftp download*/
                if(!stGetFileRsp.slRlt)
                {
                    if(FALSE == pf_is_file_exist(ascLocalFilePath))
                    {
                        stGetFileRsp.slRlt = ERRID_FTP_LOCAL_FILE_OPEN_FAIL;
                    }
                }
            }
            else
            {
                stGetFileRsp.slRlt = ERRID_FTP_SERVER_IP_INPUT_FAIL;
            }

            stGetFileRsp.ulProceType = pstGetFileReq->ulProceType;
            pf_memcpy(stGetFileRsp.ascPath, FTP_ROOT_PATH, FTP_PATH_LEN);
            pf_memcpy(stGetFileRsp.ascVersionNo, pstGetFileReq->ascVersionNo, FTP_VERSION_NUMBER_LEN);

            pl_log(INF, "ftp get file %s result is %d!", ascFtpFilePath, stGetFileRsp.slRlt);

            pf_copy_msg(ulDstModuleId,                              \
                        FTP_OAM_GET_NEW_FILE_RSP,                   \
                        ulSrcModuleId,                              \
                        &stGetFileRsp,                              \
                        sizeof(stGetFileRsp));
            
            break;
        }

        case OAM_FTP_PUT_NEW_FILE_REQ:
        {
            OAM_FTP_PUT_NEW_FILE_REQ_MSG* pstPutFileReq = (OAM_FTP_PUT_NEW_FILE_REQ_MSG*)pcvMsg;
            FTP_OAM_PUT_NEW_FILE_RSP_MSG stPutFileRsp;

            S8 ascFtpFilePath[FILE_PATH_LEN];
            S8 ascLocalFilePath[FILE_PATH_LEN];
            sprintf((CHAR*)ascFtpFilePath, "%s%s", pstPutFileReq->stFtpAddress.ascPath, pstPutFileReq->ascVersionNo);
            sprintf((CHAR*)ascLocalFilePath, "%s%s%s",              \
                    pf_get_root_path(),                             \
                    FTP_ROOT_PATH, pstPutFileReq->ascVersionNo);

            /*check the IP address*/
            if(!pf_get_inet_aton((char *)pstPutFileReq->stFtpAddress.ascDomainName))
            {
                stPutFileRsp.slRlt = pf_put_ftp_file_pasv_info(         \
                        ascFtpFilePath,                             \
                        ascLocalFilePath,                           \
                        pstPutFileReq->ascLoginName,                \
                        pstPutFileReq->ascPassword,                 \
                        pstPutFileReq->stFtpAddress.ascDomainName,  \ 
                        pstPutFileReq->stFtpAddress.ulPortId);
            }
            else
            {
                stPutFileRsp.slRlt = ERRID_FTP_SERVER_IP_INPUT_FAIL;
            }

            stPutFileRsp.ulProceType = pstPutFileReq->ulProceType;
            pf_memcpy(stPutFileRsp.ascPath, FTP_ROOT_PATH, FTP_PATH_LEN);
            pf_memcpy(stPutFileRsp.ascVersionNo, pstPutFileReq->ascVersionNo, FTP_VERSION_NUMBER_LEN);

            pl_log(INF, "ftp get file %s result is %d!", ascFtpFilePath, stPutFileRsp.slRlt);

            pf_copy_msg(ulDstModuleId,                              \
                        FTP_OAM_PUT_NEW_FILE_RSP,                   \
                        ulSrcModuleId,                              \
                        &stPutFileRsp,                              \
                        sizeof(stPutFileRsp));
            
            break;
        }
        
        case OAM_FTP_GET_NEW_VERSION_REQ:
        {
            OAM_FTP_GET_NEW_FILE_REQ_MSG* pstGetFileReq = (OAM_FTP_GET_NEW_FILE_REQ_MSG*)pcvMsg;
            FTP_OAM_GET_NEW_FILE_RSP_MSG stGetFileRsp;

            S8 ascFtpFilePath[FILE_PATH_LEN];
            S8 ascLocalFilePath[FILE_PATH_LEN];
            S8 ascCheckVer[FILE_PATH_LEN];
            sprintf((CHAR*)ascFtpFilePath, "%s%s", pstGetFileReq->stFtpAddress.ascPath, pstGetFileReq->ascVersionNo);
            sprintf((CHAR*)ascLocalFilePath, "%s%s%s",              \
                    pf_get_root_path(),                             \
                    FTP_ROOT_PATH, pstGetFileReq->ascVersionNo);

            /*check the IP address*/
            if(!pf_get_inet_aton((char *)pstGetFileReq->stFtpAddress.ascDomainName))
            {
                stGetFileRsp.slRlt = pf_get_ftp_file_pasv_info(         \
                            ascFtpFilePath,                             \
                            ascLocalFilePath,                           \
                            pstGetFileReq->ascLoginName,                \
                            pstGetFileReq->ascPassword,                 \
                            pstGetFileReq->stFtpAddress.ascDomainName,  \ 
                            pstGetFileReq->stFtpAddress.ulPortId);

                /*check the result of ftp download*/
                if(!stGetFileRsp.slRlt)
                {
                    if(FALSE == pf_is_file_exist(ascLocalFilePath))
                    {
                        stGetFileRsp.slRlt = ERRID_FTP_LOCAL_FILE_OPEN_FAIL;
                    }
                    else
                    {
                        sprintf((CHAR*)ascCheckVer, "%s/DR_APP/check_dr_ver.sh", pf_get_root_path());

                        if(pf_is_file_exist((S8*)ascCheckVer))
                        {
                            sprintf((CHAR*)ascCheckVer, "%s %s%s %s %s%s",                          \
                                    ascCheckVer, pf_get_root_path(), FTP_ROOT_PATH,                 \
                                    pstGetFileReq->ascVersionNo,                                    \
                                    pf_get_root_path(), BIN_ROOT_PATH);

                            stGetFileRsp.slRlt = pf_set_system_call((const S8 *)ascCheckVer);

                            if(PF_RET_FAILURE == stGetFileRsp.slRlt)
                            {
                                stGetFileRsp.slRlt = ERRID_FTP_CHECK_VERSION_BASE_FAIL;
                            }
                            else if(stGetFileRsp.slRlt)
                            {
                                stGetFileRsp.slRlt = (stGetFileRsp.slRlt>>8)    \
                                        + ERRID_FTP_CHECK_VERSION_BASE_FAIL;
                            }
                        }
                        /*keep the version forward compatibility*/
                        else if(pf_is_file_exist((S8*)"/dr/script/check_dr_ver.sh"))
                        {
                            sprintf((CHAR*)ascCheckVer, "/dr/script/check_dr_ver.sh %s%s %s %s%s",  \
                                    pf_get_root_path(), FTP_ROOT_PATH,                              \
                                    pstGetFileReq->ascVersionNo,                                    \
                                    pf_get_root_path(), BIN_ROOT_PATH);

                            stGetFileRsp.slRlt = pf_set_system_call((const S8 *)ascCheckVer);

                            if(PF_RET_FAILURE == stGetFileRsp.slRlt)
                            {
                                stGetFileRsp.slRlt = ERRID_FTP_CHECK_VERSION_BASE_FAIL;
                            }
                            else if(stGetFileRsp.slRlt)
                            {
                                stGetFileRsp.slRlt = (stGetFileRsp.slRlt>>8)    \
                                        + ERRID_FTP_CHECK_VERSION_BASE_FAIL;
                            }
                        }
                        else
                        {
                            stGetFileRsp.slRlt = ERRID_FTP_CHECK_VERSION_BASE_FAIL;
                        }
                        
                    }
                }
            }
            else
            {
                stGetFileRsp.slRlt = ERRID_FTP_SERVER_IP_INPUT_FAIL;
            }

            stGetFileRsp.ulProceType = pstGetFileReq->ulProceType;
            pf_memcpy(stGetFileRsp.ascPath, FTP_ROOT_PATH, FTP_PATH_LEN);
            pf_memcpy(stGetFileRsp.ascVersionNo, pstGetFileReq->ascVersionNo, FTP_VERSION_NUMBER_LEN);

            pf_copy_msg(ulDstModuleId,                              \
                        (ulMsgId + 1),                              \
                        ulSrcModuleId,                              \
                        &stGetFileRsp,                              \
                        sizeof(stGetFileRsp));

            pl_log(INF, "ftp get file %s result is %d!", ascFtpFilePath, stGetFileRsp.slRlt);

            break;
        }

        case OAM_FTP_PUT_LOG_FILE_REQ:
        {
            OAM_FTP_PUT_NEW_FILE_REQ_MSG* pstPutFileReq = (OAM_FTP_PUT_NEW_FILE_REQ_MSG*)pcvMsg;
            S32 slRlt;

            S8 ascFtpFilePath[FILE_PATH_LEN];
            S8 ascLocalFilePath[FILE_PATH_LEN];
            sprintf((CHAR*)ascFtpFilePath, "%s%s", pstPutFileReq->stFtpAddress.ascPath, pstPutFileReq->ascVersionNo);
            sprintf((CHAR*)ascLocalFilePath, "%s%s%s",              \
                    pf_get_root_path(),                             \
                    FTP_ROOT_PATH, pstPutFileReq->ascVersionNo);

            /*check the IP address*/
            if(!pf_get_inet_aton((char *)pstPutFileReq->stFtpAddress.ascDomainName))
            {
                slRlt = pf_put_ftp_file_pasv_info(         \
                        ascFtpFilePath,                             \
                        ascLocalFilePath,                           \
                        pstPutFileReq->ascLoginName,                \
                        pstPutFileReq->ascPassword,                 \
                        pstPutFileReq->stFtpAddress.ascDomainName,  \ 
                        pstPutFileReq->stFtpAddress.ulPortId);
            }
            else
            {
                slRlt = ERRID_FTP_SERVER_IP_INPUT_FAIL;
            }

            pl_log(INF, "ftp get file %s result is %d!", ascFtpFilePath, slRlt);

            if(slRlt == PF_RET_SUCCESS)
            {
                PLATFORM_OAM_LOG_UPLOAD_IND_MSG MsgInd;
                S8 ascStartTime[MAX_DATE_AND_TIME_LEN];
                CHAR* pcData = strstr((char*)pstPutFileReq->ascVersionNo, "run");
                if(pcData ==NULL)
                {
                    pcData = strstr((char*)pstPutFileReq->ascVersionNo, "ext");
                    if(pcData == NULL)
                    {
                        pcData = strstr((char*)pstPutFileReq->ascVersionNo, "war");
                        if(pcData == NULL)
                        {
                            pcData = strstr((char*)pstPutFileReq->ascVersionNo, "evt");
                        }
                    }
                }
             
                pf_memset(&MsgInd, 0, sizeof(PLATFORM_OAM_LOG_UPLOAD_IND_MSG));

                pf_memcpy(MsgInd.stUploadInd.strFilePath, pstPutFileReq->stFtpAddress.ascPath, FTP_PATH_LEN);
                pf_memcpy(MsgInd.stUploadInd.strStartTime, &pcData[4], 12);               
                pf_memcpy(MsgInd.stUploadInd.strEndTime, &pcData[17], 12);
                pf_memcpy(MsgInd.stUploadInd.strLogFileName, pstPutFileReq->ascVersionNo, FTP_VERSION_NUMBER_LEN);
                MsgInd.stUploadInd.eFileType = LOG_TYPE_DEBUG;
                
                pf_copy_msg(ulDstModuleId,                              \
                        PLATFORM_OAM_LOG_UPLOAD_IND,                   \
                        pstPutFileReq->ulCfgSrcMid,                              \
                        &MsgInd,                              \
                        sizeof(MsgInd));
            }
            
            break;
        }

        case OAM_FTP_PUT_NEW_FILE_NOW_REQ:
        {

            FTP_UPLOAD_LOG_NOW_REQ_MSG* pstPutFileReq = (FTP_UPLOAD_LOG_NOW_REQ_MSG*)pcvMsg;
            PLATFORM_OAM_UPLOAD_LOG_NOW_RSP_MSG stPutFileRsp;

            S32 slRlt;

            S8 ascFtpFilePath[FILE_PATH_LEN];
            S8 ascLocalFilePath[FILE_PATH_LEN];
            sprintf((CHAR*)ascFtpFilePath, "%s%s", pstPutFileReq->stFtpAddress.ascPath, pstPutFileReq->ascVersionNo);
            sprintf((CHAR*)ascLocalFilePath, "%s%s%s",              \
                    pf_get_root_path(),                             \
                    FTP_ROOT_PATH, pstPutFileReq->ascVersionNo);


            /*check the IP address*/
            if(!pf_get_inet_aton((char *)pstPutFileReq->stFtpAddress.ascDomainName))
            {
                slRlt = pf_put_ftp_file_pasv_info(         \
                        ascFtpFilePath,                             \
                        ascLocalFilePath,                           \
                        pstPutFileReq->ascLoginName,                \
                        pstPutFileReq->ascPassword,                 \
                        pstPutFileReq->stFtpAddress.ascDomainName,  \ 
                        pstPutFileReq->stFtpAddress.ulPortId);
            }
            else
            {
                slRlt = ERRID_FTP_SERVER_IP_INPUT_FAIL;
            }

            stPutFileRsp.ulResult = slRlt;
            stPutFileRsp.ulTransId = pstPutFileReq->ulTransId;
            
            pf_copy_msg(ulDstModuleId,                              \
                        FTP_OAM_PUT_NEW_FILE_NOW_RSP,                   \
                        pstPutFileReq->ulCfgSrcMid,                              \
                        &stPutFileRsp,                              \
                        sizeof(stPutFileRsp));
            
            break;
        }
        
        default:
            pl_log(ERR, "UNKNOWN ID %d", ulMsgId);
            break;
    }

    return ;
}
  
int ftp_cmd_port(int iSockftp_cmd_port,char *cFmt,...)    
{     
    va_list vVaStartUse;     
    int iftp_cmd_portReturn;    
    int iFtpLength;      
  
    if (pFtpIOFile == NULL)     
    {     
        pFtpIOFile = fdopen(iSockftp_cmd_port,"r");    
        if (pFtpIOFile == NULL)    
        {    
            pl_log(ERR, "The ERROR of pointer of pFtpIOFile");    
            PS_CPlus(CM_PES, CMPES_ID_FTP_CMD_OPEN_NULL);
            return PF_RET_FAILURE;    
        }     
    }      
    if (cFmt)     
    {     
        va_start(vVaStartUse,cFmt);     
        iFtpLength = vsprintf(aFtpBuffer,cFmt,vVaStartUse);     
        aFtpBuffer[iFtpLength++] = '\r';     
        aFtpBuffer[iFtpLength++]='\n';     
        write(iSockftp_cmd_port,aFtpBuffer,iFtpLength); //如同send     
    }      
    do     
    {     
        if (fgets(aFtpBuffer,sizeof(aFtpBuffer),pFtpIOFile) == NULL)     
        {    
            PS_CPlus(CM_PES, CMPES_ID_FTP_CMD_FGETS_NULL);
            return PF_RET_FAILURE;    
        }      
    } while(aFtpBuffer[3] == '-');      
  
    sscanf(aFtpBuffer,"%d",&iftp_cmd_portReturn);      
    return iftp_cmd_portReturn;    
}    


/**********************************************************************************************
 * @API function  pf_get_ftp_file_port
 * @brief         主动模式从FTP服务器下载文件的接口
 * @input         filename       下载 文件在FTP服务器的路径名称
                  pcSaveFile     下载 文件存储路径名称
 * @output        void
 * @return        >=0 - success
                  -1   - failure
 *********************************************************************************************/
extern "C" S32 pf_get_ftp_file_port(S8 *filename, S8 *pcSaveFile)    
{     
    int iSockftp_cmd_port = PF_RET_FAILURE;//用来socket接受调用后返回的套接口描述符号     
    int iSockFtpData = PF_RET_FAILURE;//datasocket建立后返回的套接口描述符号     
    int iSockAccept = PF_RET_FAILURE;     
    struct sockaddr_in addr;//定义socket结构      
    unsigned long hostip;//存放主机地址的变量      
    int iFtpLength;    
    int tmp;    
    int iftp_cmd_portReturn;     
    int retval = PF_RET_FAILURE;     
    int iOpenReturn; //接收open函数的返回值     
    unsigned char *c;//用来指向data连接时候的主机地址     
    unsigned char *p;//用来指向data连接时候的端口     
    struct hostent *he;     
    
    pl_log(INF, "get ftp:%s and save %s", filename, pcSaveFile);

    hostip = inet_addr((char*)m_stHeaderInfo.aucFtpIpAddr); //转换主机地址为网络排序模式     
    if (PF_RET_FAILURE == hostip)     
    {     
        pl_log(ERR, "HostIP is ERROR!!");    
        PS_CPlus(CM_PES, CMPES_ID_FTP_GET_PORT_INET_FAIL);
        return PF_RET_FAILURE;
    }     
        
    //建立socket     
    //设定相应的socket协议和地址     
    /**********************************************************/    
    iSockftp_cmd_port = socket(AF_INET,SOCK_STREAM,0);      
  
    if (PF_RET_FAILURE == iSockftp_cmd_port)         
    {     
        pl_log(ERR, "equal -1 failed!");    
        PS_CPlus(CM_PES, CMPES_ID_FTP_GET_PORT_CMD_SOCKET_FAIL);
        goto out;      
    }     
  
    addr.sin_family = PF_INET;     
    addr.sin_port = htons(m_stHeaderInfo.usFtpPortNum);     
    addr.sin_addr.s_addr = hostip;      
  
    /**********************************************************/     
    /*connect*/    
    if (PF_RET_FAILURE == connect(iSockftp_cmd_port,(struct sockaddr *)&addr,sizeof(addr)))     
    {     
        pl_log(ERR, "connect -1 failed!");    
        PS_CPlus(CM_PES, CMPES_ID_FTP_GET_PORT_CONNECT_FAIL);
        goto out;      
    }     

    iftp_cmd_portReturn = ftp_cmd_port(iSockftp_cmd_port,NULL);     
    if (iftp_cmd_portReturn != 220)     
    {     
        pl_log(ERR, "not 220 failed!");    
        PS_CPlus(CM_PES, CMPES_ID_FTP_GET_PORT_CMD_NULL);
        goto out;      
    }     
  
    iftp_cmd_portReturn = ftp_cmd_port(iSockftp_cmd_port,"USER %s",m_stHeaderInfo.aucFtpUserName);     
    if (iftp_cmd_portReturn != 331)     
    {     
        pl_log(ERR, "not 331 failed!");    
        PS_CPlus(CM_PES, CMPES_ID_FTP_GET_PORT_CMD_USER_FAIL);
        goto out;      
    }     
        
    iftp_cmd_portReturn = ftp_cmd_port(iSockftp_cmd_port,"PASS %s",m_stHeaderInfo.aucFtpPassword);     
    if (iftp_cmd_portReturn != 230)     
    {     
        pl_log(ERR, "iftp_cmd_portReturn not 230 failed!");    
        PS_CPlus(CM_PES, CMPES_ID_FTP_GET_PORT_CMD_PASS_FAIL);
        goto out;      
    }     
        
    iftp_cmd_portReturn = ftp_cmd_port(iSockftp_cmd_port,"TYPE I");     
    if (iftp_cmd_portReturn != 200)     
    {     
        pl_log(ERR, "iftp_cmd_portReturn not 200 failed!");    
        PS_CPlus(CM_PES, CMPES_ID_FTP_GET_PORT_CMD_TYPEI_FAIL);
        goto out;      
    }     
        
    /*建立data socket*/    
    iSockFtpData = socket(AF_INET,SOCK_STREAM,0);     
    if (PF_RET_FAILURE == iSockFtpData)     
    {     
        pl_log(ERR, "iSockFtpData equal -1 failed!");    
        PS_CPlus(CM_PES, CMPES_ID_FTP_GET_PORT_DATA_SOCKET_FAIL);
        goto out;      
    }     
        
    tmp = sizeof(addr);     
        
    getsockname(iSockftp_cmd_port, (struct sockaddr *)&addr, (socklen_t*)&tmp);     
    addr.sin_port = 0;     
        
    /*绑定*/    
    if (PF_RET_FAILURE == bind(iSockFtpData,(struct sockaddr *)&addr,sizeof(addr)))     
    {     
        pl_log(ERR, "bind iSockFtpData failed!");    
        PS_CPlus(CM_PES, CMPES_ID_FTP_GET_PORT_BIND_FAIL);
        goto out;      
    }     
        
    if (PF_RET_FAILURE == listen(iSockFtpData,1))     
    {     
        pl_log(ERR, "listen ftpdata failed!");    
        PS_CPlus(CM_PES, CMPES_ID_FTP_GET_PORT_LISTEN_FAIL);
        goto out;      
    }     
        
    tmp = sizeof(addr);     
    getsockname(iSockFtpData, (struct sockaddr *)&addr, (socklen_t*)&tmp);     
    c = (unsigned char *)&addr.sin_addr;     
    p = (unsigned char *)&addr.sin_port;     
#if 1        
    iftp_cmd_portReturn = ftp_cmd_port(iSockftp_cmd_port,"PORT %d,%d,%d,%d,%d,%d", c[0],c[1],c[2],c[3],p[0],p[1]);         
    if (iftp_cmd_portReturn != 200)     
    {     
        pl_log(ERR, "ftp return not 200! faild");    
        PS_CPlus(CM_PES, CMPES_ID_FTP_GET_PORT_CMD_PORT_FAIL);
        goto out;      
    }     
#endif        
    iftp_cmd_portReturn = ftp_cmd_port(iSockftp_cmd_port,"RETR %s", (char*)filename);     
    if (iftp_cmd_portReturn != 150)     
    {     
        pl_log(ERR, "ftp not 150 failed!");    
        PS_CPlus(CM_PES, CMPES_ID_FTP_GET_PORT_CMD_RETR_FAIL);
        goto out;      
    }     
        
    tmp = sizeof(addr);     
    iSockAccept = accept(iSockFtpData, (struct sockaddr *)&addr, (socklen_t*)&tmp);     
        
    if (PF_RET_FAILURE == iSockAccept)     
    {     
        pl_log(ERR, "ftp get iSockAccept -1 failed!");    
        PS_CPlus(CM_PES, CMPES_ID_FTP_GET_PORT_ACCEPT_FAIL);
        goto out;      
    }     

    //     
    iOpenReturn = open((char*)pcSaveFile, O_WRONLY|O_CREAT, 0644);     
    if (PF_RET_FAILURE == iOpenReturn)     
    {     
        pl_log(ERR, "ftp open failed!");    
        PS_CPlus(CM_PES, CMPES_ID_FTP_GET_PORT_OPEN_FAIL);
        goto out;      
    }     
        
    retval = 0;     
    while ((iFtpLength=read(iSockAccept, aFtpBuffer, sizeof(aFtpBuffer)))>0)     
    {     
        write(iOpenReturn, aFtpBuffer, iFtpLength);     
        retval += iFtpLength;     
    };     
        
    close(iOpenReturn);    
    
out:     
    close(iSockAccept);     
    close(iSockFtpData);     
    close(iSockftp_cmd_port);     
    if (pFtpIOFile)     
    {     
        fclose(pFtpIOFile);     
        pFtpIOFile = NULL;     
    }      

    if(PF_RET_FAILURE == retval)
    {
        pl_log(ERR, "failed iftp_cmd_portReturn %d!", iftp_cmd_portReturn);    
        PS_CPlus(CM_PES, CMPES_ID_FTP_GET_PORT_RETURN_FAIL);
    }

    return retval;    
}  
  
/**********************************************************************************************
 * @API function  pf_put_ftp_file_port
 * @brief         主动模式上传文件到FTP服务器的接口
 * @input         filename       上传 文件到FTP服务器的目标路径名称
                  pcSaveFile     上传 文件的路径名称
 * @output        void
 * @return        >=0 - success
                  -1   - failure
 *********************************************************************************************/
extern "C" int pf_put_ftp_file_port(S8 *filename, S8 *pcSaveFile)    
{     
    int iSockftp_cmd_port = PF_RET_FAILURE;//用来socket接受调用后返回的套接口描述符号     
    int iSockFtpData = PF_RET_FAILURE;//datasocket建立后返回的套接口描述符号     
    int iSockAccept = PF_RET_FAILURE;     
    struct sockaddr_in addr;//定义socket结构      
    unsigned long hostip;//存放主机地址的变量      
    int iFtpLength;    
    int tmp;    
    int iftp_cmd_portReturn;     
    int retval = PF_RET_FAILURE;     
    int iOpenReturn; //接收open函数的返回值     
    unsigned char *c;//用来指向data连接时候的主机地址     
    unsigned char *p;//用来指向data连接时候的端口     
    struct hostent *he;     

    pl_log(INF, "put ftp:%s and save %s", filename, pcSaveFile);

    hostip = inet_addr((char*)m_stHeaderInfo.aucFtpIpAddr); //转换主机地址为网络排序模式     
    if (PF_RET_FAILURE == hostip)     
    {     
        pl_log(ERR, "HostIP is ERROR!");    
        PS_CPlus(CM_PES, CMPES_ID_FTP_PUT_PORT_INET_FAIL);
        goto out;
    }     
        
    //建立socket     
    //设定相应的socket协议和地址     
    /**********************************************************/    
    iSockftp_cmd_port = socket(AF_INET,SOCK_STREAM,0);     
        
    if (PF_RET_FAILURE == iSockftp_cmd_port)     
    {
        PS_CPlus(CM_PES, CMPES_ID_FTP_PUT_PORT_CMD_SOCKET_FAIL);
        goto out;     
    }
    
    addr.sin_family = PF_INET;     
    addr.sin_port = htons(m_stHeaderInfo.usFtpPortNum);     
    addr.sin_addr.s_addr = hostip;     
    
    /**********************************************************/     
    /*connect*/    
    if (PF_RET_FAILURE == connect(iSockftp_cmd_port,(struct sockaddr *)&addr,sizeof(addr)))     
    {     
        pl_log(ERR, "put connect failed!");    
        PS_CPlus(CM_PES, CMPES_ID_FTP_PUT_PORT_CONNECT_FAIL);
        goto out;      
    }     
  
    iftp_cmd_portReturn = ftp_cmd_port(iSockftp_cmd_port,NULL);     
    if (iftp_cmd_portReturn != 220)     
    {     
        pl_log(ERR, "put not 220 failed!");    
        PS_CPlus(CM_PES, CMPES_ID_FTP_PUT_PORT_CMD_NULL);
        goto out;      
    }     
        
    iftp_cmd_portReturn = ftp_cmd_port(iSockftp_cmd_port,"USER %s", m_stHeaderInfo.aucFtpUserName);     
    if (iftp_cmd_portReturn != 331)     
    {     
        pl_log(ERR, "put not 331 failed!");    
        PS_CPlus(CM_PES, CMPES_ID_FTP_PUT_PORT_CMD_USER_FAIL);
        goto out;      
    }     
        
    iftp_cmd_portReturn = ftp_cmd_port(iSockftp_cmd_port,"PASS %s", m_stHeaderInfo.aucFtpPassword);     
    if (iftp_cmd_portReturn != 230)     
    {     
        pl_log(ERR, "put not 230 failed!");    
        PS_CPlus(CM_PES, CMPES_ID_FTP_PUT_PORT_CMD_PASS_FAIL);
        goto out;      
    }     
        
    iftp_cmd_portReturn = ftp_cmd_port(iSockftp_cmd_port,"TYPE I");     
    if (iftp_cmd_portReturn != 200)     
    {     
        pl_log(ERR, "put not 200 failed!");    
        PS_CPlus(CM_PES, CMPES_ID_FTP_PUT_PORT_CMD_TYPEI_FAIL);
        goto out;      
    }     
        
    /*建立data socket*/    
    iSockFtpData = socket(AF_INET,SOCK_STREAM,0);     
        
    if (PF_RET_FAILURE == iSockFtpData)     
    {     
        pl_log(ERR, "put eq -1 failed!");    
        PS_CPlus(CM_PES, CMPES_ID_FTP_PUT_PORT_DATA_SOCKET_FAIL);
        goto out;      
    }     
        
    tmp = sizeof(addr);     
        
    getsockname(iSockftp_cmd_port, (struct sockaddr *)&addr, (socklen_t*)&tmp);     
    addr.sin_port = 0;     
        
    /*bind socket*/    
    if (PF_RET_FAILURE == bind(iSockFtpData,(struct sockaddr *)&addr,sizeof(addr)))     
    {     
        pl_log(ERR, "ftp put bind failed!");    
        PS_CPlus(CM_PES, CMPES_ID_FTP_PUT_PORT_BIND_FAIL);
        goto out;      
    }     
        
    if (PF_RET_FAILURE == listen(iSockFtpData,1))     
    {     
        pl_log(ERR, "ftp put listen failed!");    
        PS_CPlus(CM_PES, CMPES_ID_FTP_PUT_PORT_LISTEN_FAIL);
        goto out;      
    }     
        
    tmp = sizeof(addr);     
    getsockname(iSockFtpData, (struct sockaddr *)&addr, (socklen_t*)&tmp);     
    c = (unsigned char *)&addr.sin_addr;     
    p = (unsigned char *)&addr.sin_port;     
#if 1        
    iftp_cmd_portReturn = ftp_cmd_port(iSockftp_cmd_port,"PORT %d,%d,%d,%d,%d,%d", c[0],c[1],c[2],c[3],p[0],p[1]);     
    
    if (iftp_cmd_portReturn != 200)     
    {     
        pl_log(ERR, "ftp put not 200 failed!");    
        PS_CPlus(CM_PES, CMPES_ID_FTP_PUT_PORT_CMD_PORT_FAIL);
        goto out;      
    }     
#endif        
    iftp_cmd_portReturn = ftp_cmd_port(iSockftp_cmd_port,"STOR %s", (char*)filename);     
    if (iftp_cmd_portReturn != 150)     
    {     
        pl_log(ERR, "ftp put not 150 failed!");    
        PS_CPlus(CM_PES, CMPES_ID_FTP_PUT_PORT_CMD_STOR_FAIL);
        goto out;      
    }     
        
    tmp = sizeof(addr);     
    iSockAccept = accept(iSockFtpData, (struct sockaddr *)&addr, (socklen_t*)&tmp);     
        
    if (PF_RET_FAILURE == iSockAccept)     
    {     
        pl_log(ERR, "ftp put accept -1 failed!");    
        PS_CPlus(CM_PES, CMPES_ID_FTP_PUT_PORT_ACCEPT_FAIL);
        goto out;      
    }     
    //     
    iOpenReturn = open((char*)pcSaveFile, O_RDONLY, 0644);     
    if (PF_RET_FAILURE == iOpenReturn)     
    {     
        pl_log(ERR, "ftp put open failed!");    
        PS_CPlus(CM_PES, CMPES_ID_FTP_PUT_PORT_OPEN_FAIL);
        goto out;      
    }     
        
    retval = 0;     
    
    retval=read(iOpenReturn, aFtpBuffer, 4096);    
    
    while(retval != 0)    
    {    
        write(iSockAccept,aFtpBuffer,retval);    
        retval=read(iOpenReturn,aFtpBuffer,4096);    
    }     
  
    close(iOpenReturn);    
  
out:     
    close(iSockAccept);     
    close(iSockFtpData);     
    close(iSockftp_cmd_port);     
    if (pFtpIOFile)     
    {     
        fclose(pFtpIOFile);     
        pFtpIOFile = NULL;     
    }  

    if(PF_RET_FAILURE == retval)
    {
        pl_log(ERR, "ftp put failed iftp_cmd_portReturn %d!", iftp_cmd_portReturn);    
        PS_CPlus(CM_PES, CMPES_ID_FTP_PUT_PORT_RETURN_FAIL);
    }
    return retval;  
}

/**********************************************************************************************
 * @API function  pf_modify_filename_ftp
 * @brief         修改ftp服务器上文件名接口
 * @input         pstRenameReq        ftp服务器参数结构体
                       pscFilePath              ftp服务器上目标路径文件
                       pscRnFilePath           重命名后的文件
 * @output        void
 * @return        0 success   
                        -1 failture
 *********************************************************************************************/
extern "C" S32 pf_modify_filename_ftp(FILE_NAME_MODIFY_FTP_REQ *pstRenameReq, const S8 *pscFilePath, const S8 *pscRnFilePath)
{
    if((!pscFilePath) ||(!pscRnFilePath))
    {
        pl_log(ERR, "filepath is NULL");  
        return PF_RET_FAILURE;
    }

    FILE *control_stream;
    ftp_host_info_t stFtpServer;
    ftp_host_info_t *server = &stFtpServer;
    S32 slRet = PF_RET_SUCCESS;
    int response;
    char buf[512];
    
    server->user = (const char *)pstRenameReq->ascLoginName;
    server->password = (const char *)pstRenameReq->ascPassword;
    server->lsa = xhost2sockaddr((const char *)pstRenameReq->stFtpAddress.ascDomainName, pstRenameReq->stFtpAddress.ulPortId);

    control_stream = ftp_login(server, &slRet);
    if(NULL == control_stream)
    {
        pl_log(ERR, "FTP rename info login %d error", slRet);
        return slRet;
    }    
    
    if(ftpcmd("RNFR", (const char *)pscFilePath, control_stream, buf) != 350)
    {
        pl_log(ERR, "FTP rename RNFR error rsp is %s", buf);
        return PF_RET_FAILURE;
    }
    
    response = ftpcmd("RNTO", (const char *)pscRnFilePath, control_stream, buf);
    switch(response)
    {
            case 250:
                break;

            default:
                ftp_die("ALLO", buf);
                pl_log(ERR, "FTP rename RNTO error rsp is %s", buf);
                return PF_RET_FAILURE;
    }
     ftp_quit(control_stream);
     return PF_RET_SUCCESS;
}

int xatou(char *buf)
{
    char c;
    int i;
    int j = 0;
    int retval = 0;
    char mod[3] = {1, 10, 100};
    int len = strlen(buf);
    if ( (!len)||(len > 3) )
    {
        PS_CPlus(CM_PES, CMPES_ID_FTP_XATOU_FAIL);
        return PF_RET_FAILURE;
    }    

    for (i = len - 1; i >= 0; i--)
    {
        c = buf[i];
        retval += atoi(&c)*mod[j++];
    }
    return retval;
}

int xatoul_range(char *buf, int low, int top)
{
    int retval = xatou(buf);
    if (retval < low)
    {
        retval = low;
    }    

    if (retval > top)
    {
        retval = top;
    }

    return retval;
}

len_and_sockaddr *xhost2sockaddr(const char *ip_addr, int port)
{
    int rc;
    len_and_sockaddr *r = NULL;
    struct addrinfo *result = NULL;
    struct addrinfo hint;
    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;
    rc = getaddrinfo(ip_addr, NULL, &hint, &result);
    if (rc||!result)
    {
        PS_CPlus(CM_PES, CMPES_ID_FTP_XATOU2SOCK_FAIL);
        return NULL;
    }
    
    r = (len_and_sockaddr *)pf_malloc(4 + result->ai_addrlen);
    if (r == NULL)
    {
        PS_CPlus(CM_PES, CMPES_ID_FTP_XATOU2SOCK_MALLOC_FAIL);
        return NULL;
    }
    
    r->len = result->ai_addrlen;
    memcpy(&r->sa, result->ai_addr, result->ai_addrlen);
    r->sin.sin_port = htons(port);
    freeaddrinfo(result);    
    return r;    
}

/******************************************************************/
#define IGNORE_PORT NI_NUMERICSERV

union {
struct sockaddr sa;
struct sockaddr_in sin;
}sockaddr_info;



char* sockaddr2str(const struct sockaddr *sa, int flags)
{
    char host[128];
    char serv[16];
    int rc;
    socklen_t salen;

    salen = sizeof(sockaddr_info);
    rc = getnameinfo(sa, salen,
            host, sizeof(host),
            /* can do ((flags & IGNORE_PORT) ? NULL : serv) but why bother? */
            serv, sizeof(serv),
            /* do not resolve port# into service _name_ */
            flags | NI_NUMERICSERV
            );
    if (rc)
    {
        PS_CPlus(CM_PES, CMPES_ID_FTP_SOCKADDR2STR_GETNAME_FAIL);
        return NULL;
    }
    
    if (flags & IGNORE_PORT)
    {
        PS_CPlus(CM_PES, CMPES_ID_FTP_SOCKADDR2STR_FLAGS_FAIL);
        return strdup(host);
    }

    /* For now we don't support anything else, so it has to be INET */
    /*if (sa->sa_family == AF_INET)*/
    char* retmsg;
    retmsg = (char*)pf_malloc(2048);
    memset(retmsg, 0, 2048);
    sprintf(retmsg, "%s:%s", host, serv);
    return retmsg;
}

char* xmalloc_sockaddr2dotted(const struct sockaddr *sa)
{
    return sockaddr2str(sa, NI_NUMERICHOST);
}

int xopen3(const char *pathname, int flags, int mode)
{
    int ret;

    ret = open(pathname, flags, mode);
    if (ret < 0) 
    {
        PS_CPlus(CM_PES, CMPES_ID_FTP_XOPEN3_FAIL);
        pl_log(ERR, "can't open '%s'", pathname);
    }
    return ret;
}

int xopen(const char *pathname, int flags)
{
    return xopen3(pathname, flags, 0666);
}

ssize_t safe_read(int fd, void *buf, size_t count)
{
    ssize_t n;

    do 
    {
        n = read(fd, buf, count);
    } 
    while (n < 0 && errno == EINTR);

    return n;
}

ssize_t safe_write(int fd, const void *buf, size_t count)
{
    ssize_t n;

    do 
    {
        n = write(fd, buf, count);
    } 
    while (n < 0 && errno == EINTR);

    return n;
}

size_t full_write(int fd, const void *buf, size_t len)
{
    ssize_t cc;
    ssize_t total;

    total = 0;

    while (len) 
    {
        cc = safe_write(fd, buf, len);

        if (cc < 0)
        {
            PS_CPlus(CM_PES, CMPES_ID_FTP_FULL_WRITE_FAIL);
            return cc;    /* write() returns -1 on failure. */
        }

        total += cc;
        buf = ((const char *)buf) + cc;
        len -= cc;
    }

    return total;
}



off_t bb_full_fd_action(int src_fd, int dst_fd, off_t size)
{
    int status = PF_RET_FAILURE;
    off_t total = 0;
    char buffer[BUFSIZ];
    if (src_fd < 0)
    {
        PS_CPlus(CM_PES, CMPES_ID_FTP_FULL_ACTION_FD_FAIL);
        goto out;
    }
    
    if (!size) 
    {
        PS_CPlus(CM_PES, CMPES_ID_FTP_FULL_ACTION_SIZE_FAIL);
        size = BUFSIZ;
        status = 1; /* copy until eof */
    }

    while (1) 
    {
        ssize_t rd;
        rd = safe_read(src_fd, buffer, size > BUFSIZ ? BUFSIZ : size);

        if (!rd) 
        { /* eof - all done */
            status = 0;
            break;
        }
        
        if (rd < 0) 
        {
            PS_CPlus(CM_PES, CMPES_ID_FTP_FULL_ACTION_RD_FAIL);
            pl_log(ERR, "bb_msg_read_error");
            break;
        }
        
        /* dst_fd == -1 is a fake, else... */
        if (dst_fd >= 0) 
        {
            ssize_t wr = full_write(dst_fd, buffer, rd);
            if (wr < rd) 
            {
                PS_CPlus(CM_PES, CMPES_ID_FTP_FULL_ACTION_WRITE_FAIL);
				pl_log(ERR, "bb_msg_write_error");
                break;
            }
        }
        
        total += rd;
        if (status < 0) 
        { /* if we aren't copying till EOF... */
            size -= rd;
            if (!size) 
            {
                /* 'size' bytes copied - all done */
                status = 0;
                break;
            }
        }
    }
    
out:
    return status ? PF_RET_FAILURE : total;
}

off_t bb_copyfd_eof(int fd1, int fd2)
{
    return bb_full_fd_action(fd1, fd2, 0);
}


off_t bb_copyfd_size(int fd1, int fd2, off_t size)
{
    if (size) 
    {
        return bb_full_fd_action(fd1, fd2, size);
    }
    return 0;
}


/******************************************************************/
void ftp_die(const char *msg, const char *remote)
{
    /* Guard against garbage from remote server */
    const char *cp = remote;
    while (*cp >= ' ' && *cp < '\x7f') 
    {
        cp++;
    }
    pl_log(ERR, "unexpected server response %s %s", msg, remote);
}

int ftpcmd(const char *s1, const char *s2, FILE *stream, char *buf)
{
    unsigned n;
    if (s1) 
    {
        if (s2) 
        {
            fprintf(stream, "%s %s\r\n", s1, s2);
        } 
        else 
        {
            fprintf(stream, "%s\r\n", s1);
        }
    }
    
    do 
    {
        char *buf_ptr;

        if (fgets(buf, 510, stream) == NULL) 
        {
            pl_log(ERR, "fgets");
            PS_CPlus(CM_PES, CMPES_ID_FTP_CMD_FGETS_FAIL);
            return ERRID_FTP_CMD_FGETS_FAIL;
        }
        buf_ptr = strstr(buf, "\r\n");
        if (buf_ptr) 
        {
            *buf_ptr = '\0';
        }
    } 
    while (!isdigit(buf[0]) || buf[3] != ' ');

    buf[3] = '\0';
    n = xatou(buf);
    buf[3] = ' ';
    return n;
}

void set_nport(len_and_sockaddr *lsa, unsigned port)
{
    if (lsa->sa.sa_family == AF_INET) 
    {
        lsa->sin.sin_port = port;
        return;
    }
}

int xconnect_ftpdata(ftp_host_info_t *server, char *buf)
{
    char *buf_ptr;
    unsigned short port_num;

    /* Response is "NNN garbageN1,N2,N3,N4,P1,P2[)garbage]
     * Server's IP is N1.N2.N3.N4 (we ignore it)
     * Server's port for data connection is P1*256+P2 */
    buf_ptr = strrchr(buf, ')');
    if (buf_ptr) *buf_ptr = '\0';

    buf_ptr = strrchr(buf, ',');
    if (buf_ptr) *buf_ptr = '\0';
    port_num = xatoul_range(buf_ptr + 1, 0, 255);

    buf_ptr = strrchr(buf, ',');
    if (buf_ptr) *buf_ptr = '\0';
    port_num += xatoul_range(buf_ptr + 1, 0, 255) * 256;
    
    pl_log(UINF, "#### port_num = %d\n", port_num);
    set_nport(server->lsa, htons(port_num));
    return xconnect_stream(server->lsa);
}

void xconnect(int s, const struct sockaddr *s_addr, socklen_t addrlen)
{
    if (connect(s, s_addr, addrlen) < 0) 
    {
        if (s_addr->sa_family == AF_INET)
            pl_log(ERR, "cannot connect to remote host (%s)", inet_ntoa(((struct sockaddr_in *)s_addr)->sin_addr));
    }
}

// Die with an error message if we can't open a new socket.
int xsocket(int domain, int type, int protocol)
{
    int r = socket(domain, type, protocol);

    if (r < 0) 
    {
        /* Hijack vaguely related config option */
        pl_log(ERR, "socket");
    }

    return r;
}

int xconnect_stream(const len_and_sockaddr *lsa)
{
    int fd = xsocket(lsa->sa.sa_family, SOCK_STREAM, 0);
    xconnect(fd, &lsa->sa, lsa->len);
    return fd;
}

FILE *ftp_login(ftp_host_info_t *server, S32* pslRes)
{
    FILE *control_stream;
    char buf[512];
    int login_fd;
    /* Connect to the command socket */
    login_fd = xconnect_stream(server->lsa);
    if(PF_RET_FAILURE == login_fd)
    {
        PS_CPlus(CM_PES, CMPES_ID_FTP_LOGIN_CONNECT_FAIL);
        *pslRes = ERRID_FTP_LOGIN_CONNECT_FAIL;
        return NULL;
    }
    
    control_stream = fdopen(login_fd, "r+");
    if (control_stream == NULL) 
    {
        /* fdopen failed - extremely unlikely */
        PS_CPlus(CM_PES, CMPES_ID_FTP_LOGIN_FDOPEN_FAIL);
        *pslRes = ERRID_FTP_LOGIN_FDOPEN_FAIL;
        return NULL;
    }
    
    if (ftpcmd(NULL, NULL, control_stream, buf) != 220) 
    {
        ftp_die(NULL, buf);
        PS_CPlus(CM_PES, CMPES_ID_FTP_LOGIN_FTPCMD_FAIL);
        *pslRes = ERRID_FTP_LOGIN_FTPCMD_FAIL;
        fclose(control_stream);
        return NULL;
    }
    
    /*  Login to the server */
    switch (ftpcmd("USER", server->user, control_stream, buf)) 
    {
        case 230:
            break;

        case 331:
            if (ftpcmd("PASS", server->password, control_stream, buf) != 230) 
            {
                ftp_die("PASS", buf);
                PS_CPlus(CM_PES, CMPES_ID_FTP_LOGIN_PASS_FAIL);
                *pslRes = ERRID_FTP_LOGIN_PASS_FAIL;
                 fclose(control_stream);
                return NULL;
            }
            break;

        default:
            ftp_die("USER", buf);
            PS_CPlus(CM_PES, CMPES_ID_FTP_LOGIN_USER_FAIL);
            *pslRes = ERRID_FTP_LOGIN_USER_FAIL;
            fclose(control_stream);
            return NULL;
    }
    
    if(PF_RET_FAILURE == ftpcmd("TYPE I", NULL, control_stream, buf))
    {
        PS_CPlus(CM_PES, CMPES_ID_FTP_LOGIN_TYPEI_FAIL);
        *pslRes = ERRID_FTP_LOGIN_TYPEI_FAIL;
        fclose(control_stream);
        return NULL;
    }
    
    return control_stream;
}


int ftp_send(ftp_host_info_t *server, FILE *control_stream,
        const char *server_path, char *local_path)
{
    struct stat sbuf;
    char buf[512];
    int fd_data;
    int fd_local;
    int response;

    /*  Connect to the data socket */
    if (ftpcmd("PASV", NULL, control_stream, buf) != 227) 
    {
        ftp_die("PASV", buf);
        PS_CPlus(CM_PES, CMPES_ID_FTP_SEND_PASV_FAIL);
        return ERRID_FTP_SEND_PASV_FAIL;
    }
    
    fd_data = xconnect_ftpdata(server, buf);
    if(PF_RET_FAILURE == fd_data)
    {
        PS_CPlus(CM_PES, CMPES_ID_FTP_SEND_FTPDATA_FAIL);
        return ERRID_FTP_SEND_FTPDATA_FAIL;
    }
    
    /* get the local file */
    fd_local = STDIN_FILENO;
    if (NOT_LONE_DASH(local_path)) 
    {
        fd_local = xopen(local_path, O_RDONLY);
        if(PF_RET_FAILURE == fd_local)
        {
            fd_local = xopen(local_path, O_RDONLY);
            if(PF_RET_FAILURE == fd_local)
            {
                PS_CPlus(CM_PES, CMPES_ID_FTP_SEND_XOPEN_FAIL);
                return ERRID_FTP_SEND_XOPEN_FAIL;
            }
        }
        fstat(fd_local, &sbuf);

        sprintf(buf, "ALLO %"OFF_FMT"u", sbuf.st_size);
        response = ftpcmd(buf, NULL, control_stream, buf);
        switch (response) 
        {
            case 200:
            case 202:
                break;

            default:
                close(fd_local);
                ftp_die("ALLO", buf);
                PS_CPlus(CM_PES, CMPES_ID_FTP_SEND_ALLO_FAIL);
                return ERRID_FTP_SEND_ALLO_FAIL;
        }
    }

    response = ftpcmd("STOR", server_path, control_stream, buf);
    switch (response) 
    {
        case 125:
        case 150:
            break;
            
        default:
            close(fd_local);
            ftp_die("STOR", buf);
            PS_CPlus(CM_PES, CMPES_ID_FTP_SEND_STOR_FAIL);
            return ERRID_FTP_SEND_STOR_FAIL;
    }
        
    /* transfer the file  */
    ftpmissions_t missionlist={0,0};
    ftpbackupmission_info_t filelist={0,0};
    do
    {
        do
        {
            if (PF_RET_FAILURE == bb_copyfd_eof(fd_local, fd_data)) 
            {
                close(fd_data);
                close(fd_local);
                PS_CPlus(CM_PES, CMPES_ID_FTP_SEND_BBEOF_FAIL);
                return ERRID_FTP_SEND_BBEOF_FAIL;
            }
        }
        while(filelist.currfilelist_id++<=filelist.len_filelist);
    }
    while (missionlist.currmission_id++<=missionlist.len_missionlist);

    /* close it all down */
    close(fd_data);
    close(fd_local);
    if (ftpcmd(NULL, NULL, control_stream, buf) != 226) 
    {
        ftp_die("close", buf);
        PS_CPlus(CM_PES, CMPES_ID_FTP_SEND_CLOSE_FAIL);
        return ERRID_FTP_SEND_CLOSE_FAIL;
    }
    
    return RET_SUCCESS;
}



static int ftp_recieve(ftp_host_info_t *server, FILE *control_stream, const char *local_path, char *server_path)
{
    char buf[512];
    off_t filesize = 0;
    int fd_data;
    int fd_local = PF_RET_FAILURE;
    off_t beg_range = 0;

    /* Connect to the data socket */
    if (ftpcmd("PASV", NULL, control_stream, buf) != 227) 
    {
        ftp_die("PASV error", buf + 4);
        PS_CPlus(CM_PES, CMPES_ID_FTP_RECIEVE_PASV_FAIL);
        return ERRID_FTP_RECIEVE_PASV_FAIL;
    }

    fd_data = xconnect_ftpdata(server, buf);
    if (ftpcmd("SIZE", server_path, control_stream, buf) == 213) 
    {
        filesize = strtoul(buf + 4, NULL, 10);
    }

    pl_log(UINF, "filesize=%d\n", filesize);

    if ((local_path[0] == '-') && (local_path[1] == '\0')) 
    {
        fd_local = STDOUT_FILENO;
    }

    if (ftpcmd("RETR", server_path, control_stream, buf) > 150) 
    {
        ftp_die("RETR error", buf + 4);
        PS_CPlus(CM_PES, CMPES_ID_FTP_RECIEVE_RETR_FAIL);
        return ERRID_FTP_RECIEVE_COPYFD_FAIL;
    }

    /* only make a local file if we know that one exists on the remote server */
    if (PF_RET_FAILURE == fd_local) 
    {
        fd_local = xopen(local_path, O_CREAT | O_TRUNC | O_WRONLY);
        if (PF_RET_FAILURE == fd_local) 
        {
            PS_CPlus(CM_PES, CMPES_ID_FTP_RECIEVE_XOPEN_FAIL);
            return ERRID_FTP_RECIEVE_XOPEN_FAIL;
        }
    }

    /* Copy the file */
    if (PF_RET_FAILURE == bb_copyfd_size(fd_data, fd_local, filesize)) 
    {
        close(fd_data);
        PS_CPlus(CM_PES, CMPES_ID_FTP_RECIEVE_COPYFD_FAIL);
        return ERRID_FTP_RECIEVE_COPYFD_FAIL;
    }

    /* close it all down */
    close(fd_data);

    if (ftpcmd(NULL, NULL, control_stream, buf) != 226) 
    {
        ftp_die("ftp error", buf + 4);
        PS_CPlus(CM_PES, CMPES_ID_FTP_RECIEVE_CLOSE_FAIL);
        return ERRID_FTP_RECIEVE_CLOSE_FAIL;
    }

    ftpcmd("QUIT", NULL, control_stream, buf);

    return(RET_SUCCESS);
}


int ftp_quit(FILE *control_stream)
{
    char buf[512];
    ftpcmd("QUIT", NULL, control_stream, buf);
    return 0;
}


//改变目录函数chdir
void  ftp_changdir(char *dir,int control_sockfd)
{
    char sendline[1024];
    char recvline[1024];
    int recvbytes,sendbytes;
    memset(sendline, 0, 1024);
    memset(recvline, 0,1024);
    sprintf(sendline,"CWD %s", dir);
    pl_log(UINF, "%s\n",sendline);
    sendbytes=send(control_sockfd,sendline,strlen(sendline),0);
    if(sendbytes<0)
    {
        pl_log(ERR, "cwd send is error!\n");
    }
    recvbytes=recv(control_sockfd,recvline,sizeof(recvline),0);
    if(recvbytes<0)
    {
        pl_log(ERR, "cwd recv is error!/n");
    }
    if(0 == strncmp(recvline,"250",3))
    {
        char buf[55];
        snprintf(buf,39,">>> %s/n",recvline);
        pl_log(UINF, "%s/n",buf);
    }
    else
    {
        pl_log(ERR, "cwd chdir is error!/n");
        return;
    }
}


/**********************************************************************************************
 * @API function  pf_get_ftp_file_pasv
 * @brief         被动模式从FTP服务器下载文件的接口
 * @input         filename       下载 文件在FTP服务器的路径名称
                  pcSaveFile     下载 文件存储路径名称
 * @output        void
 * @return        0    - success
                  other  - failure
 *********************************************************************************************/
extern "C" S32 pf_get_ftp_file_pasv(S8 *filename, S8 *pcSaveFile)    
{
    FILE *control_stream;
    ftp_host_info_t stFtpServer;
    ftp_host_info_t *server = &stFtpServer;

    S32 slRet = PF_RET_SUCCESS;

    server->user = (const char *)m_stHeaderInfo.aucFtpUserName;
    server->password = (const char *)m_stHeaderInfo.aucFtpPassword;
    server->lsa = xhost2sockaddr((const char *)m_stHeaderInfo.aucFtpIpAddr, m_stHeaderInfo.usFtpPortNum);
    pl_log(INF, "Connecting to %s (%s) get %s %s", m_stHeaderInfo.aucFtpIpAddr,
            xmalloc_sockaddr2dotted(&server->lsa->sa), filename, pcSaveFile);
    control_stream = ftp_login(server, &slRet);
    if(NULL == control_stream)
    {
        pl_log(ERR, "FTP get ftp pasv login %d error", slRet);
        return slRet;
    }    

    slRet = ftp_recieve(server, control_stream, (const char *)pcSaveFile, (char *)filename);
    if(slRet < 0)
    {
        pl_log(ERR, "FTP get ftp:%s %s error", filename, pcSaveFile);
        PS_CPlus(CM_PES, CMPES_ID_FTP_GET_PASV_RECIEVE_FAIL);
    }

    ftp_quit(control_stream);

    return slRet;
}


/**********************************************************************************************
 * @API function  pf_get_ftp_file_pasv_info
 * @brief         被动模式从FTP服务器下载文件的接口
 * @input         filename       下载文件在FTP服务器的路径名称
                  pcSaveFile     下载文件存储路径名称
                  pscFtpUserName 下载文件的用户名
                  pscFtpPassword 下载文件的密码
                  pscFtpIpAddr   下载文件的ftp服务器的路径或者IP address
                  ulFtpPort      下载文件的端口号
 * @output        void
 * @return        0    - success
                  other  - failure
 *********************************************************************************************/
extern "C" S32 pf_get_ftp_file_pasv_info(
        S8 *filename, 
        S8 *pcSaveFile, 
        S8* pscFtpUserName, 
        S8* pscFtpPassword,
        S8* pscFtpIpAddr,
        U32 ulFtpPort)    
{
    FILE *control_stream;
    ftp_host_info_t stFtpServer;
    ftp_host_info_t *server = &stFtpServer;

    S32 slRet = PF_RET_SUCCESS;

    pl_log(UINF, "FTP get :%s %s1 %d", pscFtpUserName, pscFtpPassword, ulFtpPort);

    server->user = (const char *)pscFtpUserName;
    server->password = (const char *)pscFtpPassword;
    server->lsa = xhost2sockaddr((const char *)pscFtpIpAddr, ulFtpPort);
    pl_log(INF, "Connecting to %s (%s) get %s %s", pscFtpIpAddr,
            xmalloc_sockaddr2dotted(&server->lsa->sa), filename, pcSaveFile);
    control_stream = ftp_login(server, &slRet);
    if(NULL == control_stream)
    {
        pl_log(ERR, "FTP get ftp pasv info login %d error", slRet);
        return slRet;
    }    

    slRet = ftp_recieve(server, control_stream, (const char *)pcSaveFile, (char *)filename);
    if(slRet < 0)
    {
        pl_log(ERR, "FTP get ftp:%s %s error", filename, pcSaveFile);
        PS_CPlus(CM_PES, CMPES_ID_FTP_GET_PASV_INFO_RECIEVE_FAIL);
    }

    ftp_quit(control_stream);

    return slRet;
}


/**********************************************************************************************
 * @API function  pf_get_ftp_file_pasv_info_encrypt
 * @brief         被动模式从FTP服务器下载文件的加密接口
 * @input         filename       下载文件在FTP服务器的路径名称
                  pcSaveFile     下载文件存储路径名称
                  pscFtpUserName 下载文件的用户名
                  pscFtpPassword 下载文件的密码
                  pscFtpIpAddr   下载文件的ftp服务器的路径或者IP address
                  ulFtpPort      下载文件的端口号
 * @output        void
 * @return        0    - success
                  other  - failure
 *********************************************************************************************/
extern "C" S32 pf_get_ftp_file_pasv_info_encrypt(
        S8 *filename, 
        S8 *pcSaveFile, 
        S8* pscFtpUserName, 
        S8* pscFtpPassword,
        S8* pscFtpIpAddr,
        U32 ulFtpPort)    
{
    FILE *control_stream;
    ftp_host_info_t stFtpServer;
    ftp_host_info_t *server = &stFtpServer;
    U8 FtpIpAddr[32];
    U8 FtpUserName[32];
    U8 FtpPassword[32];
    U8 i;

    S32 slRet = PF_RET_SUCCESS;
    pl_log(UINF, "FTP get :%s %s1 %d", pscFtpUserName, pscFtpPassword, ulFtpPort);

    pf_memset(FtpIpAddr, 0, 32);
    pf_memset(FtpUserName, 0, 32);
    pf_memset(FtpPassword, 0, 32);

    for(i=0; i<32; i++)
    {
        if(pscFtpUserName[i])
        {
            FtpUserName[i] = pscFtpUserName[i] - 1;
        }

        if(pscFtpPassword[i])
        {
            FtpPassword[i] = pscFtpPassword[i] - i;
        }

        if(pscFtpIpAddr[i])
        {
            FtpIpAddr[i] = pscFtpIpAddr[i] + 1;
        }
    }

    pl_log(UINF, "FTP get :%s %s1 %s %d", FtpUserName, FtpPassword, FtpIpAddr, ulFtpPort);

    server->user = (const char *)FtpUserName;
    server->password = (const char *)FtpPassword;
    server->lsa = xhost2sockaddr((const char *)FtpIpAddr, ulFtpPort);
    pl_log(INF, "Connecting to %s (%s) get %s %s", FtpIpAddr,
            xmalloc_sockaddr2dotted(&server->lsa->sa), filename, pcSaveFile);
    control_stream = ftp_login(server, &slRet);
    if(NULL == control_stream)
    {
        pl_log(ERR, "FTP get ftp pasv info encrypt login %d error", slRet);
        return slRet;
    }    

    slRet = ftp_recieve(server, control_stream, (const char *)pcSaveFile, (char *)filename);
    if(slRet < 0)
    {
        pl_log(ERR, "FTP get ftp:%s %s error", filename, pcSaveFile);
        PS_CPlus(CM_PES, CMPES_ID_FTP_GET_PASV_ENCINFO_RECIEVE_FAIL);
    }

    ftp_quit(control_stream);
    return slRet;
}


/**********************************************************************************************
 * @API function  pf_put_ftp_file_pasv
 * @brief         被动模式上传文件到FTP服务器的接口
 * @input         filename       上传 文件到FTP服务器的目标路径名称
                  pcSaveFile     上传 文件的路径名称
 * @output        void
 * @return        0    - success
                  other  - failure
 *********************************************************************************************/
extern "C" int pf_put_ftp_file_pasv(S8 *filename, S8 *pcSaveFile)    
{
    FILE *control_stream;
    ftp_host_info_t stFtpServer;
    ftp_host_info_t *server = &stFtpServer;

    S32 slRet = PF_RET_SUCCESS;

    server->user = (const char *)m_stHeaderInfo.aucFtpUserName;
    server->password = (const char *)m_stHeaderInfo.aucFtpPassword;
    server->lsa = xhost2sockaddr((const char *)m_stHeaderInfo.aucFtpIpAddr, m_stHeaderInfo.usFtpPortNum);
    pl_log(INF, "Connecting to %s (%s) put %s %s", m_stHeaderInfo.aucFtpIpAddr,
            xmalloc_sockaddr2dotted(&server->lsa->sa), filename, pcSaveFile);
    control_stream = ftp_login(server, &slRet);
    if(NULL == control_stream)
    {
        pl_log(ERR, "FTP put ftp pasv login %d error", slRet);
        return slRet;
    }    

    slRet = ftp_send(server, control_stream, (const char *)filename, (char *)pcSaveFile);
    if(slRet < 0)
    {
        pl_log(ERR, "FTP put ftp:%s %s error", filename, pcSaveFile);
        PS_CPlus(CM_PES, CMPES_ID_FTP_PUT_PASV_SEND_FAIL);
    }

    ftp_quit(control_stream);

    return slRet;
}


/**********************************************************************************************
 * @API function  pf_put_ftp_file_pasv_info
 * @brief         被动模式上传文件到FTP服务器的接口
 * @input         filename       上传文件到FTP服务器的目标路径名称
                  pcSaveFile     上传文件的路径名称
                  pscFtpUserName 上传文件的用户名
                  pscFtpPassword 上传文件的密码
                  pscFtpIpAddr   上传文件的ftp服务器的路径或者IP address
                  ulFtpPort      上传文件的端口号
 * @output        void
 * @return        0    - success
                  other  - failure
 *********************************************************************************************/
extern "C" S32 pf_put_ftp_file_pasv_info(
        S8 *filename, 
        S8 *pcSaveFile,
        S8* pscFtpUserName, 
        S8* pscFtpPassword,
        S8* pscFtpIpAddr,
        U32 ulFtpPort)    
{
    FILE *control_stream;
    ftp_host_info_t stFtpServer;
    ftp_host_info_t *server = &stFtpServer;

    S32 slRet = PF_RET_SUCCESS;

    pl_log(UINF, "FTP get :%s %s1 %d", pscFtpUserName, pscFtpPassword, ulFtpPort);

    server->user = (const char *)pscFtpUserName;
    server->password = (const char *)pscFtpPassword;
    server->lsa = xhost2sockaddr((const char *)pscFtpIpAddr, ulFtpPort);
    pl_log(INF, "Connecting to %s (%s) put %s %s", pscFtpIpAddr,
            xmalloc_sockaddr2dotted(&server->lsa->sa), filename, pcSaveFile);
    control_stream = ftp_login(server, &slRet);
    if(NULL == control_stream)
    {
        pl_log(ERR, "FTP put ftp pasv info login %d error", slRet);
        return slRet;
    }    

    slRet = ftp_send(server, control_stream, (const char *)filename, (char *)pcSaveFile);
    if(slRet < 0)
    {
        pl_log(ERR, "FTP put ftp:%s %s error", filename, pcSaveFile);
        PS_CPlus(CM_PES, CMPES_ID_FTP_PUT_PASV_INFO_SEND_FAIL);
    }

    ftp_quit(control_stream);

    return slRet;
}



