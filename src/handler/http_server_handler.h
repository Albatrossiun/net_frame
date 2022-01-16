#ifndef __my_net_src_handler_http_server_handler_h__
#define __my_net_src_handler_http_server_handler_h__

#include <src/common/common.h>
#include <src/common/log.h>
#include <src/handler/base_io_handler.h>
#include <src/handler/http_handler.h>
#include <src/io_loop/timer.h>
#include <src/packet/http_packet.h>
#include <src/my_net/my_net_user_handler.h>

namespace my_net {

// keepAlive 长连接 如果超过一定时间没有收到数据
// 就需要关闭这个长连接
class HttpServerHandler;
class HttpKeepAliveTimeItem : public TimeItem 
{
public:
    HttpKeepAliveTimeItem(HttpServerHandler& handler);
    ~HttpKeepAliveTimeItem();
    virtual void Process();
private:
    HttpServerHandler& _Handler;
};
typedef std::shared_ptr<HttpKeepAliveTimeItem> HttpKeepAliveTimeItemPtr;

// IdleTimeOut 空闲超时
// 空闲指的是 从连接建立开始 到收到第一个包之间的间隔
// 如果建立连接后 很久都没有收到数据 那么服务端会关闭连接
class HttpIdleTimeItem : public TimeItem
{
public:
    HttpIdleTimeItem(HttpServerHandler& handler);
    ~HttpIdleTimeItem();
    virtual void Process();
private:
    HttpServerHandler& _Handler;
};
typedef std::shared_ptr<HttpIdleTimeItem> HttpIdleTimeItemPtr;

class HttpServerHandler : public HttpHandler
{
    friend class HttpKeepAliveTimeItem;
public:
    HttpServerHandler(MyNetUserHandlerPtr myNetUserHandler);
    virtual ~HttpServerHandler();
    
    // 服务端收到客户端请求后
    // 用这个接口进行回复
    bool Reply(HttpResponsePacketPtr httpResponsePacket);

    // HttpKeepAliveTimeItem会调用这个接口
    void OnKeepAliveTimeout();
    // HttpIdleTimeItem 会调用这个接口
    void OnIdleTimeout();

    virtual bool Process(HttpPacketPtr packet, int error);

    void SetKeepAliveTimeout(size_t time) { _KeepAliveTimeout = time; }
    bool AddIdleTimeout(size_t timeout);
private:
    MyNetUserHandlerPtr _MyNetUserHandlerPtr;
    // 因为每次收到packet后 就需要重新设置一次keepAlive的定时器
    // 所以这里把超时时间保存下来
    size_t _KeepAliveTimeout;
    HttpKeepAliveTimeItemPtr _HttpKeepAliveTimeItemPtr;
    HttpIdleTimeItemPtr _HttpIdleTimeItemPtr;
    // 开启keepAlive后, 服务端在一个连接上会收到多个顺序的请求
    // 服务器相应这些请求时  需要按照接受顺序进行回复
    // 由于程序是多线程的 各个ResponsePacket准备好的顺序是随机的
    // 所以这里需要一个Map, 把Response按照顺序保存下, 然后依次发送出去
    std::map<size_t, HttpResponsePacketPtr> _ResponsePacketPtrMap;
};
typedef std::shared_ptr<HttpServerHandler> HttpServerHandlerPtr;

}
#endif // __my_net_src_handler_http_server_handler_h__
