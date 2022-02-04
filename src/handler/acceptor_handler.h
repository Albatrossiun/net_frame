#ifndef __my_net_src_handler_acceptor_handler_h__
#define __my_net_src_handler_acceptor_handler_h__

#include <src/common/common.h>
#include <src/common/log.h>
#include <src/handler/base_io_handler.h>
#include <src/io_loop/event_loop.h>
#include <src/io_loop/io_handler.h>
#include <src/packet/http_encoder_factory.h>
#include <src/packet/http_parser_factory.h>
#include <src/handler/http_handler.h>
#include <src/my_net/my_net_user_handler.h>
#include <src/handler/http_server_handler.h>
namespace my_net {

// Acceptor是给服务端使用的
// 创建好listen socket后
// 通过accept等待客户端与自己建立连接
class AcceptorHandler : public IOHandler
{
public:
    struct AcceptorConfig {
        bool _CloseOnExec;
        size_t _RecvBuffSize;
        size_t _SendBuffSize;
        
        bool _NoDelay; // tcp的缓存区的数据立即发送
        bool _SoLinger; // tcp关闭时 等待缓存区的数据都发送完

        int _KeepAliveTimeout;
        
        MyNetUserHandlerPtr _PacketHandler;
        HttpEncoderFactoryPtr _PacketEncoderFactoryPtr;
        HttpParserFactoryPtr _PacketParserFactoryPtr;

        AcceptorConfig()
            :_CloseOnExec(true),
             _RecvBuffSize(0),
             _SendBuffSize(0),
             _KeepAliveTimeout(0),
             _NoDelay(true),
             _SoLinger(false)
        {}
    };

public:
    AcceptorHandler(EventLoopPtr eventLoop,
        const std::vector<EventLoopPtr>& eventLoopVec);
    virtual ~AcceptorHandler();

public: 
    bool Start(const AcceptorConfig& config);

    bool Stop();

    bool SetListenSocket(SocketPtr socket);

    SocketPtr GetListenSocket() const;
    
    EventLoopPtr GetNextEventLoop() const;
    EventLoopPtr GetNextEventLoopWithOutLock() const;

public:
    // 实现父类的接口
    // EventLoop里的事件发生时 会调用这两个接口
    // 对于acceptor而言, listen socket上只需要关注"读"事件就行
    // 当socket可读时, 意味着有人在尝试向服务器建立连接
    // 此时调用accept(socket_fd)就可以马上返回, 得到一个新的连接
    virtual bool DoRead();
    // 这个函数里边是空的 因为对于listen socket, 我们不会注册写事件
    virtual bool DoWrite();

    void Destroy();

protected:
    AcceptorConfig _Config;
    mutable std::mutex _Mutex;
    
    volatile bool _Stop;
    // 这个eventLoop用来监控listen socket
    EventLoopPtr _AcceptEventLoop;
    // 这些EventLoop分给各个连接用
    std::vector<EventLoopPtr> _EventLoopVec;
    mutable size_t _NextIndex; // 在分配eventLoop时 用这个下标进行轮询
};
typedef std::shared_ptr<AcceptorHandler> AcceptorPtr;

}

#endif // __my_net_src_handler_acceptor_handler_h__
