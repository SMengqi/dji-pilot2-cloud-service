//本文件按照运行平台组织编译开关。
//同一个开关在不同的SECTION可能会有重复
#ifndef __OPTION__H__
#define __OPTION__H__


#    define MEMORY_STATICS

//使用mempool获取动态内存
#define MEMORY_IN_MEMPOOL       

//打开PS中的授权功能
//#define PS_AUTHORIZATION_ENABLE 

//打开pf_log中的函数名和行号功能
#define PF_LOG_WITH_FUNCTION    

#endif /*__OPTION__H__*/
