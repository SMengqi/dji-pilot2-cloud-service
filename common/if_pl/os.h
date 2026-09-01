#ifndef  __OS_H__
#define  __OS_H__

#include "pl_type.h"
#include "osport.h"

extern "C" pf_mbox_t msgQArray[];
extern void msg_entry(pf_addrword_t mid);

/*define the reserved log size after thread stack*/
#define LOG_STACK_MAX_SIZE  2097152

#define DECLMODULE(name, stacksize)             \
    S32 name##_init(U32 ulModuleId);            \
    inno_mbox name##_mbox;                      \
    void name##_entry(U32 src, U32 msgID, U32 dst, void * data, U32 length); \
    static int name##stack[(stacksize+LOG_STACK_MAX_SIZE)/4];

#define DECLCOMMONMODULE(name, id, stacksize)   \
    S32 name##_init(U32 ulModuleId);            \
    inno_mbox name##id##_mbox;                  \
    void name##_entry(U32 src, U32 msgID, U32 dst, void * data, U32 length); \
    static int name##id##stack[(stacksize+LOG_STACK_MAX_SIZE)/4];

#define DECLTASK(name, stacksize)           \
    S32 name##_init(U32 ulModuleId);        \
    inno_mbox name##_mbox;                  \
    void name##_entry(pf_addrword_t msgQ);  \
    static int name##stack[(stacksize+LOG_STACK_MAX_SIZE)/4];

#define DECLCOMMONTASK(name, id, stacksize) \
    S32 name##_init(U32 ulModuleId);        \
    inno_mbox name##id##_mbox;              \
    void name##_entry(pf_addrword_t msgQ);  \
    static int name##id##stack[(stacksize+LOG_STACK_MAX_SIZE)/4];

#define COMBINEMODULE(midDes, midSrc, nameSrc)                          \
    {moduleArray[MODULE_##midSrc] = nameSrc##_entry;                    \
    msgQArray[MODULE_##midSrc]         = msgQArray[MODULE_##midDes];    \
    nameSrc##_init(); }

#define CREATEMODULE(pri, mid, name)                                    \
    {moduleArray[MODULE_##mid]        = name##_entry;                   \
    moduleInitArray[MODULE_##mid]     = name##_init; 	                \
    msgQArray[MODULE_##mid] = (void*)pf_mbox_create(&name##_mbox);      \
    pf_thread_create_mid((pf_addrword_t)pri,    msg_entry,    (pf_addrword_t)MODULE_##mid, #name, (void*)name##stack, sizeof(name##stack) - LOG_STACK_MAX_SIZE, MODULE_##mid, LOG_STACK_MAX_SIZE); \
    pf_thread_resume(workerhandles[MODULE_##mid]);}


#define CREATECOMMONMODULE(pri, mid, name, id) \
    {moduleArray[MODULE_##mid##id]        = name##_entry;                   \
    moduleInitArray[MODULE_##mid##id]     = name##_init; 	                \
    msgQArray[MODULE_##mid##id] = (void*)pf_mbox_create(&name##id##_mbox);  \
    pf_thread_create_mid((pf_addrword_t)pri,    msg_entry,    (pf_addrword_t)MODULE_##mid##id, #name, (void*)name##id##stack, sizeof(name##id##stack) - LOG_STACK_MAX_SIZE, MODULE_##mid##id, LOG_STACK_MAX_SIZE); \
    pf_thread_resume(workerhandles[MODULE_##mid##id]);}


#define CREATEMODULE_OWNERENTRY(pri, mid, name)                     \
    {moduleArray[MODULE_##mid] = 0;                                 \
    msgQArray[MODULE_##mid] = (void*)pf_mbox_create(&name##_mbox); 	\
    name##_init(MODULE_##mid);                                      \
    pf_thread_create_mid((pf_addrword_t)pri,    name##_entry,    (pf_addrword_t)MODULE_##mid, #name, (void*)name##stack, sizeof(name##stack) - LOG_STACK_MAX_SIZE, MODULE_##mid, LOG_STACK_MAX_SIZE);}

#define CREATECOMMONMODULE_OWNERENTRY(pri, mid, name, id)                   \
    {moduleArray[MODULE_##mid##id] = 0;                                     \
    msgQArray[MODULE_##mid##id] = (void*)pf_mbox_create(&name##id##_mbox);  \
    name##_init(MODULE_##mid##id);                                          \
    pf_thread_create_mid((pf_addrword_t)pri,    name##_entry,    (pf_addrword_t)MODULE_##mid##id, #name, (void*)name##id##stack, sizeof(name##id##stack) - LOG_STACK_MAX_SIZE, MODULE_##mid##id, LOG_STACK_MAX_SIZE);}


#define STARTMODULE_OWNERENTRY(mid) \
    {pf_thread_resume(workerhandles[MODULE_##mid]);}

#define STARTCOMMONMODULE_OWNERENTRY(mid, id) \
    {pf_thread_resume(workerhandles[MODULE_##mid##id]);}

#define CREATETASK(pri, mid, name)                  \
    {    name##_init(MODULE_##mid);                 \
    pf_thread_create_mid((pf_addrword_t)pri, name##_entry, (pf_addrword_t)0, #name, (void*)name##stack, sizeof(name##stack) - LOG_STACK_MAX_SIZE, MODULE_##mid, LOG_STACK_MAX_SIZE); \
    pf_thread_resume(workerhandles[MODULE_##mid]);}

#define CREATECOMMONTASK(pri, mid, name, id)        \
    {    name##_init(MODULE_##mid##id);             \
    pf_thread_create_mid((pf_addrword_t)pri, name##_entry, (pf_addrword_t)0, #name, (void*)name##id##stack, sizeof(name##id##stack) - LOG_STACK_MAX_SIZE,  MODULE_##mid##id, LOG_STACK_MAX_SIZE); \
    pf_thread_resume(workerhandles[MODULE_##mid##id]);}

#define DECLMODULE_OWNERENTRY(name, stacksize)      \
    void name##_init(U32 ulModuleId);               \
    inno_mbox name##_mbox;                          \
    void name##_entry(pf_addrword_t mid);           \
    static int name##stack[(stacksize+LOG_STACK_MAX_SIZE)/4];

#define DECLCOMMONMODULE_OWNERENTRY(name, id, stacksize)    \
    void name##_init(U32 ulModuleId);                       \
    inno_mbox name##id##_mbox;                              \
    void name##_entry(pf_addrword_t mid);                   \
    static int name##id##stack[(stacksize+LOG_STACK_MAX_SIZE)/4];

#define DECLMODULEGROUP(name, count, stacksize)   \
    S32 name##_init(U32 ulModuleId);            \
    inno_mbox name##_mbox[count];                  \
    void name##_entry(U32 src, U32 msgID, U32 dst, void * data, U32 length); \
    static int name##stack[count][(stacksize+LOG_STACK_MAX_SIZE)/4];

#define CREATEMODULEGROUP(pri, mid, name, count) \
    for(int i=0;i<count;i++)                     \
    {                                        \
        moduleArray[MODULE_##mid##0+i]        = name##_entry;                   \
        moduleInitArray[MODULE_##mid##0+i]     = name##_init; 	                \
        msgQArray[MODULE_##mid##0+i] = (void*)pf_mbox_create(&(name##_mbox[i]));  \
        pf_thread_create_mid((pf_addrword_t)pri,    msg_entry,    (pf_addrword_t)(MODULE_##mid##0+i), #name, (void*)(name##stack[i]), sizeof(name##stack[i]) - LOG_STACK_MAX_SIZE, MODULE_##mid##0+i, LOG_STACK_MAX_SIZE); \
        pf_thread_resume(workerhandles[MODULE_##mid##0+i]);          \
    }

#endif

