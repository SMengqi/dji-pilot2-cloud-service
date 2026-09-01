#ifndef _PL_HTTP_DEFS_H_
#define _PL_HTTP_DEFS_H_


#define STATUS_CODES                                                          \
    CODE(100, Continue, "Continue")                                           \
    CODE(101, Switching_Protocols, "Switching Protocols")                     \
    CODE(102, Processing, "Processing")                                       \
    CODE(103, Early_Hints, "Early Hints")                                     \
    CODE(200, Ok, "OK")                                                       \
    CODE(201, Created, "Created")                                             \
    CODE(202, Accepted, "Accepted")                                           \
    CODE(203, NonAuthoritative_Information, "Non-Authoritative Information")  \
    CODE(204, No_Content, "No Content")                                       \
    CODE(205, Reset_Content, "Reset Content")                                 \
    CODE(206, Partial_Content, "Partial Content")                             \
    CODE(207, MultiStatus, "Multi-Status")                                    \
    CODE(208, Already_Reported, "Already Reported")                           \
    CODE(226, IM_Used, "IM Used")                                             \
    CODE(300, Multiple_Choices, "Multiple Choices")                           \
    CODE(301, Moved_Permanently, "Moved Permanently")                         \
    CODE(302, Found, "Found")                                                 \
    CODE(303, See_Other, "See Other")                                         \
    CODE(304, Not_Modified, "Not Modified")                                   \
    CODE(305, Use_Proxy, "Use Proxy")                                         \
    CODE(307, Temporary_Redirect, "Temporary Redirect")                       \
    CODE(308, Permanent_Redirect, "Permanent Redirect")                       \
    CODE(400, Bad_Request, "Bad Request")                                     \
    CODE(401, Unauthorized, "Unauthorized")                                   \
    CODE(402, Payment_Required, "Payment Required")                           \
    CODE(403, Forbidden, "Forbidden")                                         \
    CODE(404, Not_Found, "Not Found")                                         \
    CODE(405, Method_Not_Allowed, "Method Not Allowed")                       \
    CODE(406, Not_Acceptable, "Not Acceptable")                               \
    CODE(407, Proxy_Authentication_Required, "Proxy Authentication Required") \
    CODE(408, Request_Timeout, "Request Timeout")                             \
    CODE(409, Conflict, "Conflict")                                           \
    CODE(410, Gone, "Gone")                                                   \
    CODE(411, Length_Required, "Length Required")                             \
    CODE(412, Precondition_Failed, "Precondition Failed")                     \
    CODE(413, Request_Entity_Too_Large, "Request Entity Too Large")           \
    CODE(414, RequestURI_Too_Long, "Request-URI Too Long")                    \
    CODE(415, Unsupported_Media_Type, "Unsupported Media Type")               \
    CODE(416, Requested_Range_Not_Satisfiable,                                \
         "Requested Range Not Satisfiable")                                   \
    CODE(417, Expectation_Failed, "Expectation Failed")                       \
    CODE(418, I_m_a_teapot, "I'm a teapot")                                   \
    CODE(421, Misdirected_Request, "Misdirected Request")                     \
    CODE(422, Unprocessable_Entity, "Unprocessable Entity")                   \
    CODE(423, Locked, "Locked")                                               \
    CODE(424, Failed_Dependency, "Failed Dependency")                         \
    CODE(426, Upgrade_Required, "Upgrade Required")                           \
    CODE(428, Precondition_Required, "Precondition Required")                 \
    CODE(429, Too_Many_Requests, "Too Many Requests")                         \
    CODE(431, Request_Header_Fields_Too_Large,                                \
         "Request Header Fields Too Large")                                   \
    CODE(444, Connection_Closed_Without_Response,                             \
         "Connection Closed Without Response")                                \
    CODE(451, Unavailable_For_Legal_Reasons, "Unavailable For Legal Reasons") \
    CODE(499, Client_Closed_Request, "Client Closed Request")                 \
    CODE(500, Internal_Server_Error, "Internal Server Error")                 \
    CODE(501, Not_Implemented, "Not Implemented")                             \
    CODE(502, Bad_Gateway, "Bad Gateway")                                     \
    CODE(503, Service_Unavailable, "Service Unavailable")                     \
    CODE(504, Gateway_Timeout, "Gateway Timeout")                             \
    CODE(505, HTTP_Version_Not_Supported, "HTTP Version Not Supported")       \
    CODE(506, Variant_Also_Negotiates, "Variant Also Negotiates")             \
    CODE(507, Insufficient_Storage, "Insufficient Storage")                   \
    CODE(508, Loop_Detected, "Loop Detected")                                 \
    CODE(510, Not_Extended, "Not Extended")                                   \
    CODE(511, Network_Authentication_Required,                                \
         "Network Authentication Required")                                   \
    CODE(599, Network_Connect_Timeout_Error, "Network Connect Timeout Error")
    
    enum class pl_http_status_code {
#define CODE(value, name, _) name = value,
        STATUS_CODES
#undef CODE
    };    
#endif
