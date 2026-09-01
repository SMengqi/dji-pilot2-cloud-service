#ifndef _PL_HTTP_SERVER_H_
#define _PL_HTTP_SERVER_H_

#include <pistache/common.h>
#include <pistache/endpoint.h>
#include <pistache/http.h>
#include <pistache/http_headers.h>
#include "../common/pl_type.h"
#include "pl_http_defs.h"

#define PISTACHE_SERVER_THREAD 2



/*
Server 端的代码设计是，让使用者完成一个回调函数，再通过 set get/post handler 的办法将函数指针传入，
Server 在接收到 client 来的数据后，再通过调用这个回调函数，把数据传给使用者
*/

/*
GET 回调函数定义
Input:
      response_headers: 类型 unordered_map 引用，供使用者填写头信息，使用者可以通过 insert map 的方式，对想要修改的头字段进行编辑
      response_body: 类型 string 引用，供使用者填写 body 信息

Output:
      headers: 类型 unordered_map 引用，从 client 端报文中提取出的报文头
      resource: 类型 string 引用，从 client 端报文提取出的 resource，如 /resource/abc
      params: 类型 string 引用， 从 client 端报文提取出的参数列表，如 param1=10&parms=20
      request_body: 类型 string 引用，从 client 端提出的报文体

返回值:
      pl_http_status_code 类型，如 pl_http_status_code::oK, 可参考 pl_http_defs.h

*/
typedef pl_http_status_code (* FUNC_GET)(std::unordered_map <std::string, std::string> &headers, 
                                         std::string &resource,
                                         std::string &params,
                                         std::string &request_body,
                                         std::unordered_map <std::string, std::string> &response_headers,
                                         std::string &response_body);

/*
POST 回调函数定义
Input:
      response_headers: 类型 unordered_map 引用，供使用者填写头信息，使用者可以通过 insert map 的方式，对想要修改的头字段进行编辑
      response_body: 类型 string 引用，供使用者填写 body 信息

Output:
      headers: 类型 unordered_map 引用，从 client 端报文中提取出的报文头
      resource: 类型 string 引用，从 client 端报文提取出的 resource，如 /resource/abc
      request_body: 类型 string 引用，从 client 端提出的报文体

返回值:
      pl_http_status_code 类型，如 pl_http_status_code::oK, 可参考 pl_http_defs.h

*/
typedef pl_http_status_code (* FUNC_POST)(std::unordered_map <std::string, std::string> &headers,
                                          std::string &resource,
                                          std::string &request_body,
                                          std::unordered_map <std::string, std::string> &response_headers,
                                          std::string &response_body);

class pl_http_server {
    public:
        pl_http_server(S32);
        ~pl_http_server();
        
        void init(S32 threads);
        void run();
        
        class pl_http_server_handler;
/*
void set_get_handler(FUNC_GET func)
Input:
      func: 类型 FUNC_GET, 用户需填写自己编写的回调函数名

返回值：
       无
*/        
        void set_get_handler(FUNC_GET func);
/*
void set_post_handler(FUNC_GET func)
Input:
      func: 类型 FUNC_POST, 用户需填写自己编写的回调函数名

返回值：
       无
*/
        
        void set_post_handler(FUNC_POST func);
               
    private:
        Pistache::Http::Endpoint m_server;
        S32 m_threads;        
        S32 m_port;
        FUNC_GET get_handler;
        FUNC_POST post_handler;
};

#endif
