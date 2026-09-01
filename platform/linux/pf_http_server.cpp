#define THIS_MODULE PLATFORM_EX
#include "pl.h"
#include <pistache/client.h>
#include <pistache/http.h>
#include <pistache/net.h>
#include <pistache/http_headers.h>

#include "pf_http_server.h"

using std::cout;
using std::endl;
using std::cerr;
using std::string;
using namespace Pistache;

pl_http_server::pl_http_server(S32 port)
    : m_server(Address(Ipv4::any(), Port(port)))
{
    m_threads = PISTACHE_SERVER_THREAD;	
}

pl_http_server::~pl_http_server()
{
    m_server.shutdown();
}

S32 build_headers(Http::ResponseWriter &rb, std::unordered_map<std::string, std::string> &headers_list)
{
		/* process header list */
    for (auto iter = headers_list.begin(); iter != headers_list.end(); iter++) 
    {

        if (iter->first == "User-Agent")
        {
            rb.headers().add<Http::Header::UserAgent>(iter->second);
	}
		
	if (iter->first == "Server")
        {
            rb.headers().add<Http::Header::Server>(iter->second);
	}
		
	if (iter->first == "Location")
        {
            rb.headers().add<Http::Header::Location>(iter->second);
	}
		
	if (iter->first == "Host")
        {
            rb.headers().add<Http::Header::Host>(iter->second);
	}
			
	if (iter->first == "Authorization")
        {
            rb.headers().add<Http::Header::Authorization>(iter->second);
	}
		
	if (iter->first == "Access-Control-ExposeHeaders")
        {
            rb.headers().add<Http::Header::AccessControlExposeHeaders>(iter->second);
	}
		
	
        if (iter->first == "Access-Control-Allow-Headers")
        {
            rb.headers().add<Http::Header::AccessControlAllowHeaders>(iter->second);
	}
		
	    if (iter->first == "Content-Type")
        {
            rb.headers().add<Http::Header::ContentType>(iter->second);
	}

		
	if (iter->first == "Access-Control-Allow-Methods")
        {
            rb.headers().add<Http::Header::AccessControlAllowMethods>(iter->second);
	}
				
	if (iter->first == "AccessControlAllowOrigin")
        {
            rb.headers().add<Http::Header::AccessControlAllowOrigin>(iter->second);
	}
		
    }
	
    return 0;	
}

class 
pl_http_server::pl_http_server_handler  : public Http::Handler
{	
    HTTP_PROTOTYPE(pl_http_server_handler)
    
    string resource;
    string params;
    string body;
    string response_body; // input by APP user
    pl_http_status_code response_code;
        
    void onRequest(const Http::Request& req, Http::ResponseWriter response) override 
    {
		auto headerCollection = req.headers();
	    auto raw_headers_list = headerCollection.rawList();
    	            
        std::unordered_map <string,string> headers;
        std::unordered_map <string,string> response_headers;
        
        for (auto iter = raw_headers_list.begin(); iter != raw_headers_list.end(); iter++) 
        {			
		    headers.insert(std::pair(iter->second.name(), iter->second.value()));
		}
				
	    resource = req.resource();
	    params = req.query().as_str();
	    body = req.body();
				   
	    if (req.method() == Http::Method::Get) 
	    {		
			response_code = get_fun(headers, resource, params, body, response_headers, response_body);
			build_headers(response, response_headers);
	    }
	    else if (req.method() == Http::Method::Post) {
	        response_code = post_fun(headers,resource, body, response_headers, response_body);
	    }
	    
	    response.send(static_cast <Http::Code> (response_code), response_body);
    }

    void onTimeout(
        const Http::Request& /*req*/,
        Http::ResponseWriter response) override
    {
        response
            .send(Http::Code::Request_Timeout, "Timeout")
            .then([=](ssize_t) {}, PrintException());
    }

    public:
        FUNC_GET get_fun;
        FUNC_POST post_fun;
};

void 
pl_http_server::init(S32 thread_num)
{
//	auto flags = Tcp::Options::ReuseAddr;
//	auto server_opts = Http::Endpoint::options().flags(flags);
	
	auto opts = Http::Endpoint::options().threads(thread_num);
	m_server.init(opts);	
}

void
pl_http_server::run()
{
	auto handler = Http::make_handler<pl_http_server_handler>();
	
	handler->get_fun = get_handler;
	handler->post_fun = post_handler;
	
	m_server.setHandler(handler);
		
	m_server.serve();
        pl_log(INF, "Http server is running!")
}

void
pl_http_server::set_get_handler(FUNC_GET func)
{
	get_handler = func;
}

void
pl_http_server::set_post_handler(FUNC_POST func)
{
	post_handler = func;
}
