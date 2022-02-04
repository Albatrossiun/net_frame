#ifndef __my_net_src_my_net_my_net_h__
#define __my_net_src_my_net_my_net_h__

#include <src/common/common.h>
#include <src/common/log.h>
#include <src/handler/acceptor_handler.h>
#include <src/handler/base_io_handler.h>
#include <src/handler/http_handler.h>
#include <src/handler/http_client_handler.h>
#include <src/handler/http_server_handler.h>
#include <src/packet/http_parser_factory.h>
#include <src/packet/http_encoder_factory.h>
#include <src/my_net/my_net_user_handler.h>
#include <src/io_loop/event_loop.h>
#include <src/thread/thread_pool.h>
#include <src/util/url_tools/addr_parser.h>
#include <src/handler/acceptor_handler.h>

namespace my_net {

// 网络传输的配置
struct MyNetConfig {
    bool _CloseOnExec; // 是否在exec的时候关闭socket
    uint32_t _RecvBuffSize; // socket描述符的接受缓存区大小
    uint32_t _SendBuffSize; // socket描述符的发送缓存区大小
    uint32_t _ThreadNum; // 线程池大小

    bool _NoDelay; // tcp数据立即发送
    bool _SoLinger;  // 连接关闭时 是否等待tcp写缓存区的数据都发送完

    MyNetConfig()
        :_CloseOnExec(true),
         _RecvBuffSize(65536),
         _SendBuffSize(65536),
         _ThreadNum(16),
         _NoDelay(true),
         _SoLinger(false)
    {}
};

// 管理网络传输的类 所有的Connection都用这个类进行管理
//
// 基本的处理流程是:
// 1.每当有新的网络连接建立时 产生一个新的Connection
// 2.每个Connection都分配给一个EventLoop 去管理这个Connection上发生的网络事件
class MyNet {
public:
    MyNet();
    virtual ~MyNet();

    bool Init(const MyNetConfig& config);

    // 启动Transport
    // 主要是启动线程池 让线程池执行所有的EventLoop
    bool Start();
    // 停止Transport 
    // 停止每个EventLoop 然后停止线程池
    bool Stop();

    // 获取所有的EventLoop
    const std::vector<EventLoopPtr>& GetEventLoopPtrVec()
    {
        return _EventLoopPtrVec;
    }

    // 获取一个EventLoop
    // 这个函数会按照轮询的方式返回EventLoop
    EventLoopPtr GetOneEventLoop();

public:
    // 创建一个TCP连接
    // remoteSpec 远程地址, 可以是ip:port 也可以是一个url
    // localSpec 本地地址 
    HttpClientHandlerPtr CreateConnection(const std::string& remoteSpec,
                                 const std::string& localSpec,
                                 size_t timeout);

    AcceptorHandler* CreateAcceptor();
    
    bool StartServer(std::string ipAndPort,MyNetUserHandlerPtr handler);

protected:
    MyNetConfig _Config;
    std::vector<EventLoopPtr> _EventLoopPtrVec;
    uint32_t _ThreadNum;
    ThreadPool _ThreadPool;
    // 采用轮询的方式分配eventLoop, 所以需要一个下标
    std::atomic<std::uint64_t> _EventLoopIndex;
    volatile bool _Running;
    HttpEncoderFactoryPtr _PacketEncoderFactoryPtr;
    HttpParserFactoryPtr _PacketParserFactoryPtr;
};
typedef std::shared_ptr<MyNet> MyNetPtr;

}

#endif // __my_net_src_my_net_my_net_h__
