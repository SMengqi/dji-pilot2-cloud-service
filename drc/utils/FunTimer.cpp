#define THIS_MODULE MODULE_DRC_VTSM
#include "pl.h"
#include "FunTimer.h"

bool FunTime::debugFlag = true;
FunTime::FunTime(const char * funName, U32 module_name)
    : m_moduleName(module_name)
{
    if (debugFlag)
    {
        this->m_funName = new std::string(funName);
        gettimeofday(&m_startTime, NULL);
    }
}
FunTime::~FunTime()
{
    if (debugFlag)
    {
        gettimeofday(&m_endTime, NULL);
        float time_use = 1000000 * (m_endTime.tv_sec - m_startTime.tv_sec) + (m_endTime.tv_usec - m_startTime.tv_usec); //微妙结果
        // 并不建议在这里执行 std::cout 可采用拷贝方式进行其他线程日志输出 , 在这里我推荐智能指针 shared_ptr;
        pf_log(m_moduleName,UINF,m_funName->c_str(), 0,"cost time %lf us",time_use);
        delete this->m_funName;
    }
}

bool strTimeToTimeStamp(std::string strTime, struct tm &setTimeTm)
{
    //判断格式
    int cNum = 0;
    for(auto c : strTime)
    {
        if(c == ':') {cNum += 1;}
    }

    if(cNum != 2) {return false;}

    //截取字符串，并删掉空格
    std::string::size_type pos = strTime.find(':');
    if(pos == std::string::npos)
    {
        return false;
    }
    std::string firstStr = strTime.substr(0,pos);
    if( !firstStr.empty() )
    {
        //firstStr.erase(0,firstStr.find_first_not_of(" "));
        //firstStr.erase(firstStr.find_last_not_of(" ") + 1);
        int index = 0;
        while( (index = firstStr.find(' ',index)) != string::npos)
        {
            firstStr.erase(index,1);
        }       
    }

    //截取字符串，并删掉空格
    std::string tmpStr = strTime.substr(pos + 1);    
    std::string::size_type pos2 = tmpStr.find(':');
    if(pos2 == std::string::npos)
    {
        return false;
    }
    std::string secondtStr = tmpStr.substr(0,pos2);
    if( !secondtStr.empty() )
    {
        int index = 0;
        while( (index = secondtStr.find(' ',index)) != string::npos)
        {
            secondtStr.erase(index,1);
        } 
    }

    //截取字符串，并删掉空格
    std::string thirdStr = tmpStr.substr(pos2 + 1); 
    if( !thirdStr.empty() )
    {
        int index = 0;
        while( (index = thirdStr.find(' ',index)) != string::npos)
        {
            thirdStr.erase(index,1);
        } 
    }

    //校验时间是否合理
    int hour = std::atoi(firstStr.c_str());
    int min = std::atoi(secondtStr.c_str());
    int sec = std::atoi(thirdStr.c_str());

    if(hour<0 || hour>23){return false;}

    if(min<0 || min>59){return false;}

    if(sec<0 || sec>59){return false;}    

    //生成struct tm格式
    memset(&setTimeTm , 0, sizeof(tm));
    time_t nowTimeT;
    time(&nowTimeT);
    struct tm *nowTm = localtime(&nowTimeT);

    setTimeTm.tm_year = nowTm->tm_year;
    setTimeTm.tm_mon  = nowTm->tm_mon;
    setTimeTm.tm_mday = nowTm->tm_mday;
    setTimeTm.tm_hour = std::atoi(firstStr.c_str());
    setTimeTm.tm_min = std::atoi(secondtStr.c_str());
    setTimeTm.tm_sec = std::atoi(thirdStr.c_str());

    struct tm tmpTm = setTimeTm;
    int iTime = mktime(&setTimeTm);
    if( iTime == -1)
    {
        return false;
    }

    return true;
}

bool compareNowTimeStamp(struct tm startTimeTm , struct tm endTimeTm)
{
    time_t nowTimeT;
    time(&nowTimeT);
    struct tm *nowTm = localtime(&nowTimeT);
    startTimeTm.tm_year = nowTm->tm_year;
    startTimeTm.tm_mon = nowTm->tm_mon;
    startTimeTm.tm_mday = nowTm->tm_mday;
    endTimeTm.tm_year = nowTm->tm_year;
    endTimeTm.tm_mon = nowTm->tm_mon;
    endTimeTm.tm_mday = nowTm->tm_mday;

    int startTimeStamp = mktime(&startTimeTm);
    int endTimeStamp = mktime(&endTimeTm);

    if(nowTimeT>=startTimeStamp && nowTimeT<=endTimeStamp)
    {
        //cout<<"在时间段内"<<endl;
        return true;
    }
    //cout<<"在时间段外"<<endl;
    return false;
}
