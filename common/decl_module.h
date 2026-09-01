/************************************************************************
* Copyright (C), 2006~2015, ASTRI&Innofidei Inc
* File name:        decl_module.h
* Author:     
* Version:          1.0
* Date:             2012-07-09
* Description:      This file defines  all modules 
* Others:
* Revision History:
*   1.  Date:       2012-07-09
*       Author:    lifengqing@innofidei.com
*       Version:    1.0
*       Content:    Draft
*   2.  ...
**************************************************************************/

/////////////////////////////PLATFORM/////////////////////
DECL_MODULE(GTEST),
DECL_MODULE(DAILYREC),
DECL_MODULE(FTP),
DECL_MODULE(LOG),


#ifdef UNIT_TEST
DECL_MODULE(MC),
DECL_MODULE(MAC),
DECL_MODULE(RXRLC),
DECL_MODULE(TXRLC),
DECL_MODULE(TXPDCP),
DECL_MODULE(RXPDCP),
DECL_MODULE(S1AP),
DECL_MODULE(IPGW),
DECL_MODULE(RRC),
DECL_MODULE(RRM),
#endif




