#define THIS_MODULE MODULE_NETWORK
#include "net_lib.h"
#include "platform_io.h"
#include "platform_socket.h"
#include "scoped_lock.h"
#include "platform_socket_factory.h"
#include "net_manager.h"
#include "platform.h"
#include "pf_crypt.h"


#define THIS_MODULE MODULE_NETWORK
using namespace std;
static bool g_netlib_init = false;



void NetlibInit( void )
{
    if( false == g_netlib_init )
    {
        CHAR acPathSrc[256];
        CHAR acPathDst[256];

        sprintf(acPathSrc, "%s%s", pf_get_root_path(), "/config/bootup/dr_auto_ssl");
        if(pf_is_file_exist((S8*)acPathSrc))
		{
            sprintf(acPathSrc, "%s%s", pf_get_root_path(), "/DR_APP/drssl.cfg");
            sprintf(acPathDst, "%s%s", pf_get_root_path(), "/DR_APP/inf/");
	    	pf_cipher_decrypt_mul_file(acPathSrc, acPathDst);
		}

		
        NetFactory::GetInstance();
        NetManager::GetInstance();
        PlatformSocketFactory::GetInstance();

        if( false == PlatformSocket::PlatformSocketInit() )
        {
            NAS_PrintLog( LOG_ERROR , "Failed to init network library. " ); 
            PS_CPlus(CM_NES, CMNES_ID_NETLIB_SOCKET_INIT_FAIL);
            return;
        }
        if( false == PlatformIO::GetInstance()->initIO() )
        {
            NAS_PrintLog( LOG_ERROR , "Failed to init network library. " );
            PS_CPlus(CM_NES, CMNES_ID_NETLIB_PLATFORMIO_INITIO_FAIL);
            return;
        }

        g_netlib_init = true;

    }
}

S32 network_init(U32 ulModuleId)
{
    return PF_RET_SUCCESS;
}

void network_entry( unsigned long long  )
{


    PlatformIO::GetInstance()->runIO();
}

