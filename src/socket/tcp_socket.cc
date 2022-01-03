#include <src/socket/tcp_socket.h>

namespace my_net {

TcpSocket::TcpSocket(in_addr_t localIp, uint16_t localPort)
    :_LocalIp(localIp),
     _LocalPort(localPort)
{}

TcpSocket::~TcpSocket()
{}

bool TcpSocket::Bind() {
    if(_Fd < 0) {
        MY_LOG(ERROR, "bind failed, socket fd is [%d]", _Fd);
        return false;
    }

    // 开始进行bind操作
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = _LocalIp;
    addr.sin_port = htons(_LocalPort);

    // bind失败
    int ok = bind(_Fd, (sockaddr*)(&addr), sizeof(addr));
    if(ok < 0) {
        char BUFF[1024] = {0};
        strerror_r(errno, BUFF, 1024);
        MY_LOG(ERROR, "bind failed, error is [%d][%d][%s][%u][%u]", _Fd, errno, BUFF, _LocalIp, _LocalPort);
        return false;
    }

    // 如果bind的时候没有指定固定的端口
    // 那么bind函数会随机选一个端口bind 我们需要把这个随机选出的端口记录下来
    if(_LocalPort == 0) {
        socklen_t addrLen = sizeof(addr);
        if(getsockname(_Fd, (sockaddr*)(&addr), &addrLen) < 0) {
            MY_LOG(ERROR, "get port of socket [%d] failed", _Fd);
            return false;
        }
        _LocalPort = ntohs(addr.sin_port);
    }
    return true;
}

TcpListenSocket::TcpListenSocket(in_addr_t localIp, uint16_t localPort)
    :TcpSocket(localIp, localPort)
{}

TcpListenSocket::~TcpListenSocket()
{}

bool TcpListenSocket::Listen() const {
    if(_Fd < 0) {
        MY_LOG(ERROR, "listen failed, fd is [%d]", _Fd);
        return false;
    }
    // 系统调用 listen 函数.
    // 参数1 : 要listen的socketfd
    // 参数2 : backlog 未完成队列和已完成队列中连接数目之和的最大值
    if(listen(_Fd, 1000) < 0) {
        MY_LOG(ERROR, "listen [%d] failed", _Fd);
        return false;
    }
    return true;
}

bool TcpListenSocket::Accept(TcpServerSocketPtr& tcpServerSocketPrt) const {
    tcpServerSocketPrt.reset();
    if(_Fd < 0) {
        MY_LOG(ERROR, "accept failed, fd is [%d]", _Fd);
        return false;
    }
    sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    int newFd = accept(_Fd, (sockaddr*)(&addr), &addrlen);
    if(newFd < 0) {
        if(errno != EAGAIN) {
            char BUFF[1024] = {0};
            strerror_r(errno, BUFF, 1024);
            MY_LOG(ERROR, "accept on [%d] failed, error is [%s]",
                _Fd,
                BUFF);
            return false;
        }
        return false;
    }

    // 用tcpServerSocket记录这个新的fd
    TcpServerSocket* tcpServerSocket = new TcpServerSocket(
        _LocalIp,
        _LocalPort,
        addr.sin_addr.s_addr,
        ntohs(addr.sin_port)
    );
    if(tcpServerSocket == NULL) {
        MY_LOG(ERROR, "%s", "create tcpServerSocket failed");
        return false;
    }
    tcpServerSocket->SetFd(newFd);
    tcpServerSocketPrt.reset(tcpServerSocket);
    return true;
}


TcpClientSocket::TcpClientSocket(in_addr_t localIp,
                                 uint16_t localPort,
                                 in_addr_t remoteIp,
                                 uint16_t remotePort)
    :TcpSocket(localIp, localPort)
{
     TcpSocket::_RemoteIp = remoteIp;
     TcpSocket::_RemotePort = remotePort;
}

TcpClientSocket::~TcpClientSocket()
{}

TcpClientSocket::TcpSocketConnectType TcpClientSocket::Connect() const {
    if(_Fd < 0) {
        MY_LOG(ERROR, "connect failed, fd is [%d]", _Fd);
        return TcpSocketConnectType::TSCT_ERROR;
    }

    // MY_LOG(DEBUG, "remote ip is [%d], port is [%d]", _RemoteIp, _RemotePort);

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = _RemoteIp;
    addr.sin_port = htons(_RemotePort);

    if(connect(_Fd, (sockaddr*)(&addr), sizeof(addr)) < 0) {
        // 如果fd是非阻塞的
        // 出现这个错误 表示这次连接还在进行中
        // 可以将这个fd 放到epoll, poll, select等io复用中, 如果触发了可写事件, 代表连接完成了
        if(errno != EINPROGRESS) {
            char BUFF[1024] = {0};
            strerror_r(errno, BUFF, 1024);
            MY_LOG(ERROR, "connect failed, error is [%s]", BUFF);
            return TcpSocketConnectType::TSCT_ERROR;
        }
        return TcpSocketConnectType::TSCT_CONNECTING;
    }
    return TcpSocketConnectType::TSCT_CONNECTED;
}

TcpServerSocket::TcpServerSocket(in_addr_t localIp,
                                 uint16_t localPort,
                                 in_addr_t remoteIp,
                                 uint16_t remotePort)
    :TcpSocket(localIp, localPort)
{
     _RemoteIp = remoteIp;
     _RemotePort = remotePort;
}

TcpServerSocket::~TcpServerSocket()
{}
}
