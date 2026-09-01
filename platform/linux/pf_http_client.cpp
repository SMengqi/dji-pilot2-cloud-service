#define THIS_MODULE PLATFORM_EX
#include "pl.h"

#include <pistache/client.h>
#include <pistache/http.h>
#include <pistache/net.h>
#include <pistache/http_headers.h>

#include "pf_http_client.h"

using std::cout;
using std::endl;
using std::cerr;
using std::string;
using namespace Pistache;

pl_http_client::pl_http_client()
{
	m_threads = PISTACHE_CLIENT_THREAD;
	m_max_connects = PISTACHE_CLIENT_MAX_CONNECTS;
	m_async_time_sec = ASYNC_TIME_OUT;
}

pl_http_client::~pl_http_client()
{
	m_client.shutdown();
}

void
pl_http_client::init()
{
	auto opts = Http::Experimental::Client::options().threads(m_threads).maxConnectionsPerHost(m_max_connects);
	m_client.init(opts);
	pl_log(INF, "Http client init done");	
}

void
pl_http_client::init(S32 threads, S32 max_connects, S32 async_time_sec)
{
    m_async_time_sec = async_time_sec;
	auto opts = Http::Experimental::Client::options().threads(threads).maxConnectionsPerHost(async_time_sec);
	m_client.init(opts);
	pl_log(INF, "Http client init done");
}


S32 build_headers(Pistache::Http::Experimental::RequestBuilder &rb, std::initializer_list<std::pair<std::string, std::string>> &headers_list)
{
		/* process header list */
    for (auto iter = headers_list.begin(); iter != headers_list.end(); iter++) 
    {
        //cout << iter->first << ": " << iter->second << endl;
        if (iter->first == "Access-Token")
        {
            rb.header<Http::Header::AccessToken>(iter->second);
		}        

        if (iter->first == "User-Agent")
        {
            rb.header<Http::Header::UserAgent>(iter->second);
	    }
		
	    if (iter->first == "Server")
        {
            rb.header<Http::Header::Server>(iter->second);
	    }
		
	    if (iter->first == "Location")
        {
            rb.header<Http::Header::Location>(iter->second);
	    }
		
	    if (iter->first == "Host")
        {
            rb.header<Http::Header::Host>(iter->second);
	    }
			
	    if (iter->first == "Authorization")
        {
            rb.header<Http::Header::Authorization>(iter->second);
	    }
		
	    if (iter->first == "Access-Control-ExposeHeaders")
        {
            rb.header<Http::Header::AccessControlExposeHeaders>(iter->second);
	    }
		
	
        if (iter->first == "Access-Control-Allow-Headers")
        {
            rb.header<Http::Header::AccessControlAllowHeaders>(iter->second);
	    }
		
	    if (iter->first == "Content-Type")
        {
            rb.header<Http::Header::ContentType>(iter->second);
	    }

		
	    if (iter->first == "Access-Control-Allow-Methods")
        {
            rb.header<Http::Header::AccessControlAllowMethods>(iter->second);
	    }
				
	    if (iter->first == "AccessControlAllowOrigin")
        {
            rb.header<Http::Header::AccessControlAllowOrigin>(iter->second);
	    }
		
    }
	
    return 0;	
}

S32 
pl_http_client::get(string url, HTTP_CMD_RES &res)
{	
    std::vector<Async::Promise<Http::Response>> responses;
		
    auto resp = m_client.get(url).send();
     
    resp.then(
		[&](Http::Response response) {
			
			res.body = response.body();
			res.code = static_cast <pl_http_status_code> (response.code());
		},
		
		[&](std::exception_ptr exc) {
			PrintException excPrinter;
			excPrinter(exc);
			return -1;
		});
		
    responses.push_back(std::move(resp));
    auto sync = Async::whenAll(responses.begin(), responses.end());
    Async::Barrier<std::vector<Http::Response>> barrier(sync);
    barrier.wait_for(std::chrono::seconds(m_async_time_sec));
    
    return 0;
}


S32 
pl_http_client::get(std::string url, std::initializer_list<std::pair<std::string, std::string>> &headers_list, HTTP_CMD_RES &res)
{	
    std::vector<Async::Promise<Http::Response>> responses;
	
    Pistache::Http::Experimental::RequestBuilder rb = m_client.get(url);
	
    build_headers(rb, headers_list);
    
    auto resp = rb.send();
     
    resp.then(
		[&](Http::Response response) {
			auto raw_headers_list = response.headers().rawList();
		    for ( auto iter = raw_headers_list.begin(); iter != raw_headers_list.end(); iter++) 
                    {			
		        res.headers.insert(std::pair(iter->second.name(), iter->second.value()));
		    }
	    
		    res.body = response.body();
		    res.code = static_cast <pl_http_status_code> (response.code());
		},
		
		[&](std::exception_ptr exc) {
			PrintException excPrinter;
			excPrinter(exc);
			return -1;
		});
		
    responses.push_back(std::move(resp));
    auto sync = Async::whenAll(responses.begin(), responses.end());
    Async::Barrier<std::vector<Http::Response>> barrier(sync);
    barrier.wait_for(std::chrono::seconds(m_async_time_sec));
    
    return 0;	
}

S32
pl_http_client::post(std::string url, std::string content, HTTP_CMD_RES &res)
{	
    std::vector<Async::Promise<Http::Response>> responses;
		
    auto resp = m_client.post(url).body(content).send();
     
    resp.then(
		[&](Http::Response response) {
			
			res.body = response.body();
			res.code = static_cast <pl_http_status_code> (response.code());
		},
		
		[&](std::exception_ptr exc) {
			PrintException excPrinter;
			excPrinter(exc);
			return -1;
		});
		
    responses.push_back(std::move(resp));
    auto sync = Async::whenAll(responses.begin(), responses.end());
    Async::Barrier<std::vector<Http::Response>> barrier(sync);
    barrier.wait_for(std::chrono::seconds(m_async_time_sec));
    
    return 0;	
}

S32 
pl_http_client:: post(std::string url, std::string content, std::initializer_list<std::pair<std::string, std::string>> &heads_list, HTTP_CMD_RES &res)
{	
    std::vector<Async::Promise<Http::Response>> responses;
	
    Pistache::Http::Experimental::RequestBuilder rb = m_client.post(url).body(content);
	
    build_headers(rb, heads_list);
    	

	printf("rb.send start\n");
    auto resp = rb.send();
	printf("rb.send end\n");
     
    resp.then(
		[&](Http::Response response) {
			
			res.body = response.body();
			res.code = static_cast <pl_http_status_code> (response.code());
		},
		
		[&](std::exception_ptr exc) {
			PrintException excPrinter;
			excPrinter(exc);
			return -1;
		});
		
    responses.push_back(std::move(resp));
    auto sync = Async::whenAll(responses.begin(), responses.end());
    Async::Barrier<std::vector<Http::Response>> barrier(sync);
    barrier.wait_for(std::chrono::seconds(m_async_time_sec));
    
    return 0;	    	
}
