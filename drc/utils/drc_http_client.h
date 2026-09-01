#ifndef DRC_HTTP_CLIENT_H_
#define DRC_HTTP_CLIENT_H_
#include <iostream>
#include <string>
#include <functional>
#include <cstdio>
#include <limits>
#include <unordered_map>
#include <atomic>
#include <curl/curl.h>

namespace DRC_HTTP_CLIENT
{
    typedef size_t (*POST_CALLBACK_FUNC)(void *contents, size_t size, size_t nmemb ,void *pCurl);

    namespace Default
    {
        constexpr int TimeOut               = 5;
        constexpr int MaxConnectionsPerHost = 8;
        constexpr bool KeepAlive            = true;
        constexpr size_t MaxResponseSize    = std::numeric_limits<uint32_t>::max();
    } 

    typedef struct _HTTP_CMD_RES{
        std::string body;
        long code;
        std::unordered_map<std::string, std::string> headers;
    }HTTP_CMD_RES;

    class drc_http_client 
    {
        struct Options
        {
            friend class drc_http_client;

            Options()
                : timeout_(Default::TimeOut)
                , maxConnectionsPerHost_(Default::MaxConnectionsPerHost)
                , keepAlive_(Default::KeepAlive)
                , maxResponseSize_(Default::MaxResponseSize)
            { }

            Options& timeOut(int val);
            Options& keepAlive(bool val);
            Options& maxConnectionsPerHost(int val);
            Options& maxResponseSize(size_t val);

        private:
            int timeout_;
            int maxConnectionsPerHost_;
            bool keepAlive_;
            size_t maxResponseSize_;
        };


    private:
        Options options_;

    public:
        drc_http_client();

        ~drc_http_client();

        static Options options();

        void init(const Options& options);

        int  post(std::string &url, std::string &content, POST_CALLBACK_FUNC postCallbackFunc ,HTTP_CMD_RES &httpRes);

        std::string  getLocalTime();
    };
}

#endif