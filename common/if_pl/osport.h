#ifndef _OSPORT_H_
#define _OSPORT_H_

#include "pl.h"

#ifdef __cplusplus
extern "C" {
#endif
#include <pthread.h>
#ifdef __cplusplus
}
#endif
typedef unsigned long long pf_addrword_t;
typedef pthread_t pf_handle_t;
typedef unsigned int pf_mutex_t ;
typedef unsigned int pf_sem_t ;
typedef unsigned int pf_thread_t;
typedef void pf_thread_entry_t(pf_addrword_t);
typedef void* pf_mbox_t;
 void pf_thread_create(
     pf_addrword_t      sched_info,                 /* scheduling info (eg pri)  */
     pf_thread_entry_t  *entry,                     /* entry point function      */
     pf_addrword_t      entry_data,                 /* entry data                */
     char                *name,                     /* optional thread name      */
     void                *stack_base,               /* stack base, NULL = alloc  */
     unsigned int        stack_size,                /* stack size, 0 = default   */
     pf_handle_t        *handle,                    /* returned thread handle    */
     pf_thread_t        *thread                     /* put thread here           */
 ) ;

void pf_thread_create_mid(
        pf_addrword_t       sched_info,             /* scheduling info (eg pri)      */
        pf_thread_entry_t   *entry,                 /* entry point function          */
        pf_addrword_t       entry_data,             /* entry data                    */
        char                *name,                  /* optional thread name          */
        void                *stack_base,            /* stack base, NULL = alloc      */
        U32                 stack_size,             /* stack size, 0 = default       */
        U32                 mid,                    /* module thread handle id       */
        U32                 log_size                /* log size,default is LOGMAXSIZE*/
        );

void pf_thread_delete(pf_handle_t thread);

void pf_create_module_group(
        pf_addrword_t pri,                          /* scheduling info (eg pri)  */
        CHAR* module_name,                          /* module thread name        */
        U32 module_id,                              /* module thread handle id   */
        MODULE_ENTRY module_entry,                  /* entry point function      */
        MODULE_INIT module_init,                    /* module init function      */
        void* module_mbox,                          /* module msg box            */
        void* modulestack,                          /* stack base                */
        U32 stack_size,                             /* stack size                */
        U32 log_size                                /* log size                  */
    );


                              //
#define pf_mutex_lock(...)    //todo: implementation
#define pf_mutex_unlock(...)  //todo: implementation
#define pf_thread_resume(...) //todo: implementation



#include <stdio.h>
#include <unistd.h>    //chdir, write, close
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h> 
#include <sys/types.h>




#define dbgline //printf("%s,%d\n",__FUNCTION__,__LINE__)
#endif
