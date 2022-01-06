#ifndef __my_net_src_handler_base_io_handler_h__
#define __my_net_src_handler_base_io_handler_h__
#include <src/io_loop/io_handler.h>
#include <src/io_loop/timer.h>
#include <src/io_loop/epoll.h>
#include <src/io_loop/event_loop.h>
#include <src/socket/tcp_socket.h>
#include <src/packet/http_packet.h>
#include <src/packet/http_encoder.h>
#include <src/packet/http_parser.h>
#include <src/handler/http_handler.h>

namespace my_net {

class HttpHandler;
typedef std::shared_ptr<HttpHandler> HttpHandlerPtr;

class BaseIoHandler : public IOHandler {
    friend class HttpHandler;
public:
    class ConnectionTimer : public TimeItem {
    public:
        ConnectionTimer(BaseIoHandler* handler)
            :_handler(handler)
        {}
        void Process() {
            _handler->TimeOutProcess();
        }
    private:
        BaseIoHandler* _handler;    
    };

    friend class ConnectionTimer;

    enum CONNECT_STATE {
        CS_NONE,
        CS_CONNECTING, // 正在建立连接
        CS_CONNECTED,  // 成功建立的连接
        CS_CLOSING,    // 正在关闭
        CS_CLOSED      // 已经关闭
    };

    // 处理连接过程中各种可能的错误类型
    enum PROCESS_ERROR {
        PE_NONE, // 没有错误
        PE_CONNECT_FAILED, // 连接失败
        PE_CONNECT_CLOSE, // 连接被关闭
        PE_READ_FAILED, // 读取错误
        PE_WRITE_FAILED, // 写入失败
        PE_DECODE_FAILED, // 解析数据包失败
        PE_CONNECT_TIMEOUT, // 超时
        PE_PACKET_TIMEOUT, // 等待数据包超时
        PE_HTTP_KEEPALIVE_TIMEOUT, // Http长连接超时
        PE_HTTP_IDLE_TIMEOUT, // Http空闲超时
    };
protected:
    BaseIoHandler(EventLoopPtr eventLoopPtr);
    virtual ~BaseIoHandler();

    EventLoopPtr GetEventLoop() const { return _EventLoopPtr; }

    SocketPtr GetSocket() const { return _SocketPtr; }
    void SetSocket(SocketPtr socketPtr) { _SocketPtr = socketPtr; }

    void SetPacketParser(HttpParserPtr packetParserPtr) { _PacketParserPtr = packetParserPtr; }
    HttpParserPtr GetPacketParser() const { return _PacketParserPtr; }

    void SetPacketEncoder(HttpEncoderPtr packetEncoderPtr) { _PacketEncoderPtr = packetEncoderPtr; }
    HttpEncoderPtr GetPacketEncoder() const { return _PacketEncoderPtr; }

    std::mutex& GetMutex() { return _Mutex; }

    // 设置处理packet的句柄函数
    void SetPacketHandler(HttpHandlerPtr handler) { _HttpHandler = handler; }

    // 发送数据的函数
    virtual bool SendPacket(HttpPacketPtr& packet);
    virtual bool SendPacketWithoutLock(HttpPacketPtr packet);

    // 建立连接
    virtual bool Connect(size_t timeout);
    // 关闭连接
    virtual void Close();
    virtual void CloseWithoutLock();

    // 获取当前是否处于连接状态
    virtual bool IsConnected();    
    virtual bool IsClosed();

    virtual bool DoRead();
    virtual bool DoWrite();

    // 向_EventLoop中加入超时事件
    bool AddTimeItem(TimeItemPtr timeItem, int timeout);
    // 从_EventLoop中删除超时事件
    bool DeleteTimeItem(TimeItemPtr timeItem);

    // 处理超时事件
    void TimeOutProcess();
  
    // 检查正在连接的socket 是否完成了连接
    bool CheckConnectingState();

    // 是否在所有的包发送完成后 关闭连接
    void SetCloseAfterAllPacketSent(bool close) { _DoCloseAfterSend = close; }
    
    void ReportErrorAndClose(PROCESS_ERROR pe);

    // 释放链接 清理内存
    void Destroy();
    bool ConnectWithoutLock(size_t timeout);
protected:
    volatile CONNECT_STATE _State;
    EventLoopPtr _EventLoopPtr;
    SocketPtr _SocketPtr;
    HttpParserPtr _PacketParserPtr;
    HttpEncoderPtr _PacketEncoderPtr;
    std::shared_ptr<ConnectionTimer> _ConnectionTimerPtr;

    std::list<DataBlobPtr> _DataBlobPtrList;

    // 读取数据后 交个这个handler处理
    HttpHandlerPtr _HttpHandler; 

    std::mutex _Mutex;
    bool _DoCloseAfterSend;
};
}
#endif