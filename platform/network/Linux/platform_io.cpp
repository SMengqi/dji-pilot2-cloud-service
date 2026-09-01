#define THIS_MODULE MODULE_NETWORK
/* C standard header file */
#include <stdint.h>

/* Linux system header file */
#include <unistd.h>
#include <sys/epoll.h>

#include "net_handle.h"
#include "platform_io.h"
#include "platform_socket.h"
#include "net_manager.h"
#include "platform.h"

#include "pf_thread_mon.h"


PlatformIO* PlatformIO::GetInstance()
{
    static PlatformIO __instance;

    return &__instance;
}

PlatformIO::PlatformIO()
{
    this->m_EpollFd = -1;
}

PlatformIO::~PlatformIO()
{

}

bool PlatformIO::initIO()
{
    /* Since Linux 2.6.8, the size argument is ignored, but must be greater than zero */
    this->m_EpollFd = epoll_create(1);

    //printf("\r\n PlatformIO::Init  m_EpollFd= %d",this->m_EpollFd);

    /*  On success, these system calls return a nonnegative file descriptor. 
    On error, -1 is returned, and errno is set to indicate the error */
    if ( -1 == this->m_EpollFd )
    {
        PS_CPlus(CM_NES, CMNES_ID_PLATFORMIO_INITIO_FAIL);
        return false;
    }
    else
    {
        return true;
    }
}

bool PlatformIO::BindSocket( void* handle , void* overlapped)
{
    NAS_PrintLog( LOG_FATAL," BindSocket   m_socketFd= %d,  m_EpollFd= %d ", *((int*)handle) , this->m_EpollFd);

    if( this->m_EpollFd >= 0 )
    {
        epoll_event epollEventToAdd;
        

        int* fdToAdd             = (int*)handle;
        epollEventToAdd.data.ptr = overlapped;

        PlatformSocket* pPlatformSocket = ( PlatformSocket* )overlapped;
        if( pPlatformSocket->IsListenSocket() )
        {
            epollEventToAdd.events  = EPOLLIN;
        }
        else
        {
            if (pPlatformSocket->GetProtocol() == NETPROTOCOL_ASFPMAL)
                epollEventToAdd.events  = EPOLLIN;
            else
                epollEventToAdd.events  = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLERR | EPOLLHUP;
                
        }

        /* When successful, epoll_ctl() returns zero.  When an error occurs,
        epoll_ctl() returns -1 and errno is set appropriately             */
        if (0 == epoll_ctl(this->m_EpollFd,EPOLL_CTL_ADD,*fdToAdd,&epollEventToAdd))
        {
            return true;
        } 
        else
        {
            PS_CPlus(CM_NES, CMNES_ID_PLATFORMIO_BIND_EPOLLCTL_FAIL);
            return false;
        }
    }
    else
    {
        PS_CPlus(CM_NES, CMNES_ID_PLATFORMIO_BIND_EPOLLFD_FAIL);
        return false;
    }
}

void PlatformIO::UnBindSocket( void* handle)
{
    if( this->m_EpollFd >= 0 )
    {
        epoll_event epollEventToAdd;

        NAS_PrintLog( LOG_FATAL," UnBindSocket   m_socketFd= %d,  m_EpollFd= %d ", *((int*)handle) , this->m_EpollFd);


        int* fdToAdd             = (int*)handle;
        
        /* When successful, epoll_ctl() returns zero.  When an error occurs,
        epoll_ctl() returns -1 and errno is set appropriately             */
        if (0 == epoll_ctl(this->m_EpollFd,EPOLL_CTL_DEL,*fdToAdd,&epollEventToAdd))
        {
            return ;
        } 
        else
        {
            PS_CPlus(CM_NES, CMNES_ID_PLATFORMIO_UNBIND_EPOLLCTL_FAIL);
            return;
        }
    }
    else
    {
        return;
    }
}


void PlatformIO::runIO()
{  
    RunIocp();
}

void PlatformIO::RunSelect()
{
    while( true )
    {
        pf_usleep( 500 );
    }
}

#define MAX_NUMBER_OF_PROCESSED_EPOLL_EVENT (8)


void PlatformIO::RunIocp()
{

    epoll_event eventResults[MAX_NUMBER_OF_PROCESSED_EPOLL_EVENT];

    while (true)
    {
        int waitedEventNumber = epoll_wait(this->m_EpollFd,eventResults,MAX_NUMBER_OF_PROCESSED_EPOLL_EVENT,-1);
        pf_thread_mon_update_count(MODULE_NETWORK);


        /* When successful, epoll_wait() returns the number of file descriptors ready
        for the requested I/O, or zero if no file descriptor became ready during
        the requested timeout milliseconds.  When an error occurs, epoll_wait()
        returns -1 and errno is set appropriately. */
        if (-1 == waitedEventNumber)
        {
            if( errno != EINTR )
            {
                NAS_PrintLog( LOG_WARNING," epoll wait error:%d " , errno );
            }
            pf_thread_mon_update_count(MODULE_NETWORK);
            continue;
        }

//printf("\r\n RunIocp_2: waitedEventNumber=%d",waitedEventNumber);

        PlatformSocket* socketToHandle = NULL;
        for (int eventIndex = 0 ;eventIndex < waitedEventNumber; ++eventIndex)
        {
            epoll_event& currentEvent = eventResults[eventIndex];
            socketToHandle = (PlatformSocket*)currentEvent.data.ptr;

            if ( true == socketToHandle->IsInvalid() )
            {
                SessionData tmpsession = socketToHandle->GetSessionData();
                
                NAS_PrintLog( LOG_WARNING," eventIndex_%d , socketToHandle_0x%08x is invalid , loacal_%s:%d , peer_%s:%d " ,
                    eventIndex,socketToHandle,tmpsession.m_LocalIP.c_str(), tmpsession.m_LocalPort, tmpsession.m_PeerIP.c_str(), tmpsession.m_PeerPort );

                continue;
            }

            if ( true == socketToHandle->IsListenSocket() )
            {

//printf("\r\n RunIocp_3: OnAccept");

                socketToHandle->OnAccept();
            } 
            else
            {



                /* EPOLLIN   The associated file is available for read(2) operations.
                   EPOLLPRI  There is urgent data available for read(2) operations.*/
                if ( 
                     (currentEvent.events & EPOLLIN) ||
                     (currentEvent.events & EPOLLPRI)
                   )
                {

//printf("\r\n RunIocp_4: EPOLLIN || EPOLLPRI");
                    if(socketToHandle->IsEncrypted() == true)
                    {
                        if( false == socketToHandle->RecvSsl() )
                        {
                            const ConnHandle& handle = socketToHandle->GetNetHandle();;

                            socketToHandle->OnClose();
                        
                            NetManager::GetInstance()->Close( handle );

                            //delete socketToHandle;
                            continue;
                        }
                    }
                    else 
                    {
                        if( false == socketToHandle->Recv() )
                        {
                            const ConnHandle& handle = socketToHandle->GetNetHandle();;

                            socketToHandle->OnClose();
                        
                            NetManager::GetInstance()->Close( handle );

                            //delete socketToHandle;
                            continue;
                        }
                     }
                    //ev.data.fd = socketToHandle->GetSocket();
                    //ev.data.ptr = socketToHandle;
                    //ev.events = EPOLLOUT|EPOLLET;
                    //epoll_ctl( this->m_EpollFd , EPOLL_CTL_MOD , ev.data.fd , &ev );
                }

                /*  EPOLLOUT The associated file is available for write(2) operations. */
                if ( currentEvent.events & EPOLLOUT)
                {

//printf("\r\n RunIocp_5: EPOLLOUT");

                    if( false == socketToHandle->Send() )
                    {
                        const ConnHandle& handle = socketToHandle->GetNetHandle();;
                    
                        socketToHandle->OnClose();

                        NetManager::GetInstance()->Close( handle );

                        //delete socketToHandle;
                        continue;
                    }
                    
                    
                    //ev.data.fd = socketToHandle->GetSocket();
                    //ev.events = EPOLLIN|EPOLLET;
                    //ev.data.ptr = socketToHandle;
                    //epoll_ctl( this->m_EpollFd , EPOLL_CTL_MOD , ev.data.fd , &ev );
                }

                /*  EPOLLHUP Hang up happened on the associated file descriptor.  epoll_wait(2)
                    will always wait for this event; it is not necessary to set it in
                    events.*/
                if ( ( currentEvent.events & EPOLLERR ) || (currentEvent.events & EPOLLHUP) )
                {


//printf("\r\n RunIocp_5: EPOLLERR || EPOLLHUP");

                    const ConnHandle& handle = socketToHandle->GetNetHandle();;

                    socketToHandle->OnClose();
                    
                    NetManager::GetInstance()->Close( handle );

                    //delete socketToHandle;
                    continue;
                }
            }
        }
        pf_thread_mon_update_count(MODULE_NETWORK);
    }
}
