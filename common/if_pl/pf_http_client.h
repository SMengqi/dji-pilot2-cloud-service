#ifndef _PL_HTTP_CLIENT_H_
#define _PL_HTTP_CLIENT_H_

#include <pistache/client.h>
#include <pistache/http.h>
#include <pistache/net.h>
#include <pistache/http_headers.h>
#include "pl_http_defs.h"
#include "../common/pl_type.h"
#define PISTACHE_CLIENT_THREAD 1
#define PISTACHE_CLIENT_MAX_CONNECTS 8
#define ASYNC_TIME_OUT 5
//typedef signed int S32;
/*
 HTTP_CMD_RES 结构体，用于承载从 HTTP server 端返回的信息
 body: std::string 类型，用于存放 http 报文的 body 信息
 code: pl_http_status_code 类型（enum class），具体请参考 pl_http_defs.h 头文件，用于存放报文的状态码，如 pl_http_status_code::oK (值为200）
 headers:  std::unordered_map 类型，MAP 中的 key 和 value 都是 string 类型，用于存放报文的头信息 （简单使用时，一般用不到它）
*/

typedef struct _HTTP_CMD_RES{
	std::string body;
	pl_http_status_code code;
	std::unordered_map<std::string, std::string> headers;
}HTTP_CMD_RES;

class pl_http_client {
public:
	pl_http_client();
	~pl_http_client();
	
/*
void init()
说明：
    http client 初始化函数，在建立 client 对象后，要调用它（或下面带参数的重载函数）        
*/
	void init();
/*
void init(S32 threads, S32 max_connects, S32 async_time_sec)
说明：
    http client 初始化函数，在建立 client 对象后，要调用它（或上面无参数的 init 函数
    Input: 
           threads: 类型 S32, 使用 thread 的个数，缺省为 1
           max_connects: 类型 S32, 最大连接个数，缺省为 8
           async_time_sec: 类型 S32, 命令等待最长时间（秒），缺省为 5秒
*/
	void init(S32 threads, S32 max_connects, S32 async_time_sec);

/*
S32 get(std::string url, HTTP_CMD_RES &res):
说明: http 协议中的 get 命令
Input: 
      url:  类型 string，GET 命令中的 URL，
            如 http://127.0.0.1:8090/resource/abc?param=1&param=2
            其中: http 为协议名; 127.0.0.1 为 http 服务器的 IP 地址; 8090 为通信端口号; 
            /resource/abc 为 resource 段;
            “?” 是输入参数的开始标志; param=1 是第一个参数及值; “&” 是第一参数和第二参数的分割符; param=2 是第二参数及值

Output：
      res: 类型 HTTP_CMD_RES 引用，在得到 server 传回的消息后，get 会填写好这个结构，使用者可利用结构中的数据进行处理
返回值：
      0 为 ok
      其它：异常
            
*/
	S32 get(std::string url, HTTP_CMD_RES &res);
/*
S32 get(std::string url, std::initializer_list<std::pair<std::string, std::string>> &headers_list, HTTP_CMD_RES &res)

说明: http 协议中的 get 命令 (可带用户自己修改的头信息） 
Input: 
      url:  类型 string，GET 命令中的 URL，
            如 http://127.0.0.1:8090/resource/abc?param=1&param=2
            其中: http 为协议名; 127.0.0.1 为 http 服务器的 IP 地址; 8090 为通信端口号; 
                /resource/abc 为 resource 段;
                “?” 是输入参数的开始标志; param=1 是第一个参数及值; “&” 是第一参数和第二参数的分割符; param=2 是第二参数及值

    headers_list: 类型 std::initializer_list 引用，用户可以加上自己修改的头信息
            例如: {{"Server", "bxt-server", "Host", "bxt-host"}}
    注意：  目前只支持以下的头信息修改：
          "User-Agent"
          "Server" 
          "Location"
          "Host" 
          "Authorization"                
          "Access-Control-ExposeHeaders" 
          "Access-Control-Allow-Headers"         
          "Content-Type"         
          "Access-Control-Allow-Methods"         
          "AccessControlAllowOrigin"
Output:
       res: 类型 HTTP_CMD_RES 引用，用户先创建一个 HTTP_CMD_RES 结构变量，使用者调用此成员函数后，成员函数会填写好这个结构体
           返回信息请参考 HTTP_CMD_RES 的定义
返回值：
       0 为成功
       其它为异常          
*/

	S32 get(std::string url, std::initializer_list<std::pair<std::string, std::string>> &headers_list, HTTP_CMD_RES &res);
/*
S32 post(std::string url, std::string content, HTTP_CMD_RES &res)

说明: http 协议中的 POST 命令
Input:
      url: 类型 string， POST 命令中的 URL，参考前面的相关描述
      content： 类型 string， POST 命令要发的内容，参数设定之类的都可以写在 content 中，也可以说是 body
Output:
       res: 类型 HTTP_CMD_RES 引用，用户先创建一个 HTTP_CMD_RES 结构变量，使用者调用此成员函数后，成员函数会填写好这个结构体
           返回信息请参考 HTTP_CMD_RES 的定义
返回值：
       0 为成功
       其它为异常     
*/

	S32 post(std::string url, std::string content, HTTP_CMD_RES &res);

/*
S32 post(std::string url, std::string content, std::initializer_list<std::pair<std::string, std::string>> &headers_list, HTTP_CMD_RES &res)

说明: http 协议中的 POST 命令 （可带头字段）
Input:
      url: 类型 string， POST 命令中的 URL，参考前面的相关描述
      content: 类型 string， POST 命令要发的内容，参数设定之类的都可以写在 content 中，也可以说是 body
      headers_list: 类型 std::initializer_list 引用，用户可以加上自己修改的头信息
            例如: {{"Server", "bxt-server", "Host", "bxt-host"}}
    注意：  目前只支持以下的头信息修改：
          "User-Agent"
          "Server" 
          "Location"
          "Host" 
          "Authorization"                
          "Access-Control-ExposeHeaders" 
          "Access-Control-Allow-Headers"         
          "Content-Type"         
          "Access-Control-Allow-Methods"         
          "AccessControlAllowOrigin"
      

Output:
       res: 类型 HTTP_CMD_RES 引用，用户先创建一个 HTTP_CMD_RES 结构变量，使用者调用此成员函数后，成员函数会填写好这个结构体
           返回信息请参考 HTTP_CMD_RES 的定义
返回值：
       0 为成功
       其它为异常   
*/

	S32 post(std::string url, std::string content, std::initializer_list<std::pair<std::string, std::string>> &headers_list, HTTP_CMD_RES &res);
	
private:
	Pistache::Http::Experimental::Client m_client;
	S32 m_threads;
	S32 m_max_connects;
	S32 m_async_time_sec;
};
/*
Available HTTP header settings:
------------------------
 "User-Agent"
 "Server" 
 "Location"
 "Host"	
 "Authorization"		
 "Access-Control-ExposeHeaders"	
 "Access-Control-Allow-Headers"		
 "Content-Type"		
 "Access-Control-Allow-Methods"		
 "AccessControlAllowOrigin"
---------------------------
Demo code of HTTP GET/POST  without/with header/body
    cout << "---------------------------------------------" << endl;
	if (!client.get("http://127.0.0.1:9080/ping/res?param1=10&param2=20", st_result))
	{
		cout << " GET: send message ok " << endl;
		
		if (st_result.code == pl_http_status_code::Ok)
		{
			cout << "[ Server return code is ] OK " << endl;
			cout << "[ Body is ] " << st_result.body << endl;
		}
		else
		{
			cout << "!!! GET: Server return  NO-OK" << endl;
		}
	}
	else
	{
		cout << " GET: send message failed" << endl;
	}
	
	cout << "---------------------------------------------" << endl;
	
	std::initializer_list<std::pair<std::string, std::string>> headers_list({{"Server", "bxt-server"}, {"Host", "bxt-host"}});
	
	if (!client.get("http://127.0.0.1:9080/pong/res?param1=30&param2=40", headers_list, st_result))
	{
		cout << " GET: send message ok " << endl;

		if (st_result.code == pl_http_status_code::Ok)
		{
			cout << "[ Server return code is ] OK " << endl;
			cout << "[ Server return headers are: ] " << endl;
			for (auto iter = st_result.headers.begin(); iter != st_result.headers.end(); iter++)
			{
				cout << iter->first << " : "<< iter->second << endl;
			}
			cout << "Get body is " << st_result.body << endl;
		}
		else
		{
		    cout << "!!! GET: Server return  NO-OK" << endl;	
		}
	}
	else
	{
		cout << " GET: send message failed" << endl;
	}
	
	cout << "---------------------------------------------" << endl;
	
	if (!client.post("http://127.0.0.1:9080/ping/res", "param1=10 param2=20 ", st_result))
	{
		cout << " POST: send message ok " << endl;		
		if (st_result.code == pl_http_status_code::Ok)
		{
			cout << "[ Server return code is ] OK " << endl;
			cout << "[ Body is ] " << st_result.body << endl;
		}
		else
		{
		    cout << "!!! POST: Server return  NO-OK" << endl;	
		}
	}
	else
	{
		cout << " POST: send message failed" << endl;
	}
	
	cout << "---------------------------------------------" << endl;
	
	if (!client.post("http://127.0.0.1:9080/ping/res", "param1=10 param2=20 ", headers_list, st_result))
	{
		cout << " POST: send message ok " << endl;		
		if (st_result.code == pl_http_status_code::Ok)
		{
			cout << "[ Server return code is ] OK " << endl;
			cout << "[ Body is ] " << st_result.body << endl;
		}
		else
		{
		    cout << "!!! POST: Server return  NO-OK" << endl;	
		}
	}
	else
	{
		cout << " POST: send message failed" << endl;
	}
	
 */
#endif
