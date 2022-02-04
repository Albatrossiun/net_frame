#ifndef __my_net_src_handler_http_client_handler_h__
#define __my_net_src_handler_http_client_handler_h__

#include <src/common/common.h>
#include <src/common/log.h>
#include <src/handler/http_handler.h>
#include <src/handler/base_io_handler.h>
#include <src/io_loop/timer.h>
#include <src/packet/http_packet.h>
#include <src/my_net/my_net_user_handler.h>

namespace my_net {

class HttpClientHandler;
class HttpClientRequestTimeItem : public TimeItem {
public:
    HttpClientRequestTimeItem(HttpClientHandler& handler);
    virtual ~HttpClientRequestTimeItem();
    virtual void Process();
private:
    HttpClientHandler& _handler;
};
typedef std::shared_ptr<HttpClientRequestTimeItem> HttpClientRequestTimeItemPtr;

class HttpClientHandler : public HttpHandler{
public:
    friend class MyNet;
    friend class HttpClientRequestTimeItem;
public:
    HttpClientHandler();
    virtual ~HttpClientHandler();
   
    virtual bool Process(HttpPacketPtr packet, int error);

    bool Request(HttpRequestPacketPtr requestPacket,
                 int timeout,
                 HttpPacketPtr& responsePacketPtr, int& error);
    // 异步请求 
    // 调用后立刻返回
    // 有结果后 MyNetUserHandlerPtr
    bool RequestAsync(HttpRequestPacketPtr requestPacket,
                 int timeout,
                 MyNetUserHandlerPtr& handler);
private:
    void OnTimeOut();
private:
 
    struct OneRequest {
        MyNetUserHandlerPtr _Handler;
        HttpClientRequestTimeItemPtr _Timer;

        OneRequest(HttpClientHandler& h1, MyNetUserHandlerPtr h2);
        ~OneRequest();
    };    
    typedef std::shared_ptr<OneRequest> OneRequestPtr;
    std::list<OneRequestPtr> _RequestLists;
};
typedef std::shared_ptr<HttpClientHandler> HttpClientHandlerPtr;

}

#endif // __my_net_src_handler_http_client_handler_h__
