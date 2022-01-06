#ifndef __my_net_src_handler_http_handler_h__
#define __my_net_src_handler_http_handler_h__
#include <src/common/common.h>
#include <src/common/log.h>
#include <src/packet/http_packet.h>
#include <src/handler/base_io_handler.h>

namespace my_net {


class BaseIoHandler;

class HttpHandler
{
public:
    HttpHandler();
    virtual ~HttpHandler();

    void SetBaseIOHandler(BaseIoHandler* baseIoHandler) { _BaseIOHandler = baseIoHandler; } 
    BaseIoHandler* GetBaseIOHandler() { return _BaseIOHandler; }

    bool GetKeepAlive() { return _KeepAlive; }
    void SetKeepAlive(bool keepAlive) { _KeepAlive = keepAlive; }

    bool IsClosed() const;

    void Close();

    void Destroy();

    virtual bool Process(BaseIoHandler* baseIoHander, HttpPacketPtr packet, int error) = 0;
protected:
    BaseIoHandler* _BaseIOHandler;
    bool _KeepAlive;
    // 接受的序号
    size_t _RecvNo;
    // 发送的序号
    size_t _SendNo;
};
typedef std::shared_ptr<HttpHandler> HttpHandlerPtr;

}

#endif // __my_net_src_handler_http_handler_h__