#define THIS_MODULE MODULE_DRC_COMMON
#include <iostream>
#include <string>
#include <thread>
#include "pl_type.h"
#include "pl.h"
#include "drc_http_client.h"

using namespace DRC_HTTP_CLIENT;

drc_http_client::drc_http_client()
{
    curl_global_init(CURL_GLOBAL_ALL); 
}

drc_http_client::~drc_http_client()
{
    curl_global_cleanup();
}

drc_http_client::Options& drc_http_client::Options::timeOut(int val)
{
    timeout_ = val;
    return *this;
}

drc_http_client::Options& drc_http_client::Options::keepAlive(bool val)
{
    keepAlive_ = val;
    return *this;
}

drc_http_client::Options& drc_http_client::Options::maxConnectionsPerHost(int val)
{
    maxConnectionsPerHost_ = val;
    return *this;
}

drc_http_client::Options drc_http_client::options() 
{ 
    return drc_http_client::Options(); 
}

void drc_http_client::init(const Options& options)
{
    this->options_ = options;

    //std::cout<<"timeout_::"<<this->options_.timeout_<<std::endl;
    //std::cout<<"keepAlive_::"<<this->options_.keepAlive_<<std::endl;
    //std::cout<<"maxResponseSize_::"<<this->options_.maxResponseSize_<<std::endl;
}

std::string  drc_http_client::getLocalTime()
{
    struct tm pstTmInfo;
    time_t pulTime = time(NULL);
    pstTmInfo = *localtime(&pulTime); 

    char local_timestamp[256] = {0};
    int ulSpLen = sprintf((char*)local_timestamp, "%04d-%02d-%02d %02d:%02d:%02d",
        pstTmInfo.tm_year + 1900, pstTmInfo.tm_mon + 1, pstTmInfo.tm_mday,
        pstTmInfo.tm_hour, pstTmInfo.tm_min, pstTmInfo.tm_sec);    

    std::string strTime;
    strTime.assign(local_timestamp, ulSpLen);

    return strTime;
}


int  drc_http_client::post(std::string &url, std::string &content, POST_CALLBACK_FUNC postCallbackFunc ,HTTP_CMD_RES &httpRes)
{
    int ret = 0;
    CURL *pCurl = curl_easy_init();
    if (!pCurl)
    {
        std::string strErr = " curl_easy_init error";
        httpRes.code = -1;  
        httpRes.body = strErr;
        std::cout<<getLocalTime()<<strErr<<std::endl;
        pl_log(FATAL,strErr.c_str());
        curl_easy_cleanup(pCurl);
        curl_global_cleanup();
        return -1;
    }

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers,"Content-Type:application/json");
    if(headers == NULL)
    {
        std::string strErr = " curl_slist_append error";
        httpRes.code = -1;  
        httpRes.body = strErr;        
        std::cout<<getLocalTime()<<strErr<<std::endl;
        pl_log(FATAL,strErr.c_str());
        curl_slist_free_all(headers);
        return -1;
    }        

    curl_easy_setopt(pCurl, CURLOPT_URL, url.c_str());   // 指定url
    curl_easy_setopt(pCurl, CURLOPT_TIMEOUT, this->options_.timeout_);
    curl_easy_setopt(pCurl, CURLOPT_POST, 1L);
    curl_easy_setopt(pCurl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(pCurl, CURLOPT_POSTFIELDS, content.c_str());
    curl_easy_setopt(pCurl, CURLOPT_WRITEFUNCTION, postCallbackFunc);    

    long response_code;
    CURLcode resC = curl_easy_perform(pCurl);
    if(resC != CURLE_OK)
    {
        ret = -1;

        std::string strErr = " curl_easy_perform error:" + std::to_string(resC);
        httpRes.code = resC;  
        httpRes.body = strErr;        
        std::cout<<getLocalTime()<<strErr<<std::endl;
        pl_log(FATAL,strErr.c_str());
    }
    else
    {
        //std::cout<<"curl_easy_perform success:"<<resC<<std::endl;
        curl_easy_getinfo(pCurl, CURLINFO_RESPONSE_CODE ,&response_code);
        if(response_code != 200)
        {
            std::cout<<"Server return code status abnormal:"<<response_code<<std::endl;
            pl_log(FATAL,"Server return code status abnormal: %d",response_code);
        }   
        httpRes.code = response_code;    
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(pCurl);

    return ret;
}
