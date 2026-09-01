#ifndef LINUXPORT_H
#define LINUXPORT_H


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

/* platform-independent definitions */
#include <pthread.h>
#include <semaphore.h>
#include <netinet/in.h>




///////////////////////////////////////////////////////////////////////////////////
#define PF_THREAD_SELF                          pthread_self

#define PF_MUTEX_T                                pthread_mutex_t
//#define PF_MUTEX_INIT(pMutex)                   {*(pMutex) = PTHREAD_MUTEX_INITIALIZER;}
#define PF_MUTEX_INIT(pMutex)                   {    \
        pthread_mutexattr_t inherit_attr;    \
        pthread_mutexattr_init(&inherit_attr);            \
        pthread_mutexattr_setprotocol(&inherit_attr, PTHREAD_PRIO_INHERIT);    \
        pthread_mutex_init(pMutex, &inherit_attr);}

#define PF_MUTEX_LOCK(pMutex)                   pthread_mutex_lock(pMutex)
#define PF_MUTEX_UNLOCK(pMutex)                 pthread_mutex_unlock(pMutex)
#define PF_MUTEX_DESTROY(pMutex)                pthread_mutex_destroy(pMutex)


#define PF_SEMAPHORE_T                          sem_t
#define PF_SEMAPHORE_INIT(pSemaphore, value)    sem_init((pSemaphore), 0, (value))
#define PF_SEMAPHORE_WAIT(pSemaphore)           sem_wait(pSemaphore)
#define PF_SEMAPHORE_POST(pSemaphore)           sem_post(pSemaphore)
#define PF_SEMAPHORE_DESTROY(pSemaphore)        sem_destroy(pSemaphore)


//#define htons(x) ((((x)&0xff00)>>8) + (((x)&0xff)<<8))
//#define ntohs(x) htons(x)
//#define htonl(x) ((((x)&0xff000000)>>24) + (((x)&0x00ff0000)>>8)+(((x)&0x0000ff00)<<8)+(((x)&0x000000ff)<<24))
//#define ntohl(x) htonl(x)

#endif

