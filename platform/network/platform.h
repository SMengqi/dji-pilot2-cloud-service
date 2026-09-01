#ifndef S1_COMMON_H
#define S1_COMMON_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdint.h>
#include <string>

#include "option.h"
#include "module.h"
#include "event.h"
#include "pl.h"
#include "ipv4address.h"
#include "pf_timer.h"

enum LogLevel
{
    LOG_LEVEL_DO_NOT_USE,
    LOG_FATAL,
    LOG_ERROR,
    LOG_WARNING,
    LOG_INFO,
    LOG_TRACE,
    LOG_UINFO,
    LOG_TEST_INFO,
    LOG_TEST_ERROR
};

#define NAS_CopyMessage(sourceId,messageId,destinationId,pMessage,messageSize) pf_copy_msg(sourceId,messageId,destinationId,pMessage,messageSize)

#define NAS_PrintLog(level, format, ...) pl_log(level,format, ## __VA_ARGS__)
//#define NAS_PrintLog(level, format, ...)   printf("\r\n");printf(format, ## __VA_ARGS__);pl_log(level,format, ## __VA_ARGS__)

//#define NAS_PrintModuleLog(level,moduleID, format, ...) ltePlatform.PrintModuleLog(level,moduleID, format,## __VA_ARGS__)
#define NAS_MemoryCopy(pDestionationData, pSourceData, dataSize) memcpy(pDestionationData, pSourceData, dataSize)
#define NAS_MemorySet(pDestionationData, dataValue, dataSize) pl_memset(pDestionationData, dataValue, dataSize)
#define NAS_MemoryMalloc(memorySize) pl_malloc(memorySize)
#define NAS_MemoryFree(pMemory) pl_free(pMemory)
#define NAS_GetMessageName(eventid) pf_get_event_name(eventid)
#define NAS_TimerStart(SrcModuleId, Duration, tmpTypeId, Param,tmpTimerId) pf_timer_start(SrcModuleId, Duration, tmpTypeId, Param,tmpTimerId)
#define NAS_TimerStop(tmpTimerId, SrcModuleId) pf_timer_stop(tmpTimerId, SrcModuleId) 




#endif
