#ifndef __my_net_src_socet_tcp_socket__
#define __my_net_src_socet_tcp_socket__

#include <src/common/common.h>
#include <src/common/log.h>
#include <src/socket/socket.h>
namespace my_net {

// 对于tcp而言 
// socket需要记录的数据有 
// 本地ip 本地端口
// 远程ip 远程端口
class TcpSocket : public Socket {
public:
    TcpSocket(in_addr_t localIp, uint16_t localPort);
    virtual ~TcpSocket();

    bool Bind();

    in_addr_t GetLocalIp() const { return _LocalIp; }
    uint16_t GetLocalPort() const { return _LocalPort; }
    in_addr_t GetRemoteIp() const { return _RemoteIp; }
    uint16_t GetRemotePort() const { return _RemotePort; }

protected:
    in_addr_t _LocalIp;
    uint16_t _LocalPort;
    in_addr_t _RemoteIp;
    uint16_t _RemotePort;
};
typedef std::shared_ptr<TcpSocket> TcpSocketPtr;

// 用于监听的tcp socket
// 需要实现两个函数
// 一个是开始监听 listen
// 一个是接受远程连接accept

class TcpServerSocket;
class TcpListenSocket : public TcpSocket {
public:
    TcpListenSocket(in_addr_t localIp, uint16_t localPort);
    ~TcpListenSocket();
    
    bool Listen() const;
    bool Accept(std::shared_ptr<TcpServerSocket>& tcpServerSocketPtr) const;
};
typedef std::shared_ptr<TcpListenSocket> TcpListenSocketPtr;


// 作为客户端时 需要向远程建立连接
// 需要实现 connect函数 用于向远程建立连接
class TcpClientSocket : public TcpSocket {
public:
    enum TcpSocketConnectType {
        TSCT_ERROR,   // 发生错误
        TSCT_CONNECTING, // 连接建立中
        TSCT_CONNECTED, // 连接建立完成
    };
public:
    TcpClientSocket(in_addr_t localIp,
                    uint16_t localPort,
                    in_addr_t remoteIp,
                    uint16_t remotePort);
    ~TcpClientSocket();

    // 向远程建立连接
    TcpSocketConnectType Connect() const;
};
typedef std::shared_ptr<TcpClientSocket> TcpClientSocketPtr;

// 作为服务端时
// 没有特殊要求
class TcpServerSocket : public TcpSocket {
public:
    TcpServerSocket(in_addr_t localIp,
                    uint16_t localPort,
                    in_addr_t remoteIp,
                    uint16_t remotePort);
    ~TcpServerSocket();
};
typedef std::shared_ptr<TcpServerSocket> TcpServerSocketPtr;

}
#endif
