
#ifndef DRC_FUN_TIMER_H_
#define DRC_FUN_TIMER_H_
#include <iostream>
#include <string>
#include <sys/time.h>

class FunTime
{
public:
    FunTime(const char * funName, U32 module_name);
    ~FunTime();

private:
    static bool debugFlag;
    std::string* m_funName;
    U32 m_moduleName;
    timeval m_startTime;
    timeval m_endTime;
};


bool strTimeToTimeStamp(std::string strTime, struct tm &setTimeTm);

bool compareNowTimeStamp(struct tm startTimeTm , struct tm endTimeTm);

#endif