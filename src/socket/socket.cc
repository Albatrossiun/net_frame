#include <src/socket/socket.h>
namespace my_net {

Socket::Socket()
    :_Fd(-1)
{}

Socket::~Socket()
{
    Close();
}

int Socket::Read(void* buf, size_t len) const {
    if(_Fd < 0) {
        MY_LOG(ERROR, "socket fd is %d", _Fd);
        return -1;
    }
    int num = 0;
    while(true) {
        num = read(_Fd, buf, len);
        if(num < 0) {
            if(errno == EINTR) {
                // 系统中断导致的错误 这种情况下应该重新读取
                continue;
            }
        }
        // 如果读到了内容 或者错误类型不是系统中断 就要退出
        break;   
    }
    return num;
}

int Socket::Write(const void* buf, size_t len) const {
    if(_Fd < 0) {
        MY_LOG(ERROR, "socket fd is %d", _Fd);
        return -1;
    }

    int num = 0;
    while(true) {
        num = write(_Fd, buf, len);
        if(num < 0) {
            if(errno == EINTR) {
                continue;
            }
        }
        break;
    }
    return num;
}

void Socket::Close() {
    // 仅当文件描述符有效时 才进行关闭
    if(_Fd >= 0) {
        // 关闭失败
        if(close(_Fd) < 0) {
            char BUFF[1024] = {0};
            strerror_r(errno, BUFF, 1024);
            MY_LOG(ERROR, "close fd [%d] failed, error is [%s]",
                _Fd, BUFF);
        }
        // 无论是否成功关闭 都要把fd置为-1
        _Fd = -1;
    }
}

void Socket::ShutdownWrite() {
    if(_Fd >= 0) {
        shutdown(_Fd, SHUT_WR);
    }
}

void Socket::ShutdownRead() {
    if (_Fd >= 0) {
        shutdown(_Fd, SHUT_RD);
    }
}

bool Socket::SetNonBlocking(bool enable) const {
    if(_Fd < 0) {
        MY_LOG(ERROR, "socket fd is %d", _Fd);
        return false;
    }

    int socketFlags = 0;
    // 先获取当前fd上已经设置的属性
    socketFlags = fcntl(_Fd, F_GETFL);
    // 获取失败
    if(socketFlags == -1) {
        char BUFF[1024] = {0};
        strerror_r(errno, BUFF, 1024);
        MY_LOG(ERROR, "set [%d] nblock failed. F_GETFL failed, error is [%s]", 
            _Fd, BUFF);
        return false;
    }

    // 如果是要设置成非阻塞
    if(enable) {
        socketFlags |= O_NONBLOCK;
    } else {
    // 如果要设置成阻塞
        socketFlags &= ~O_NONBLOCK;
    }
    
    int ok = fcntl(_Fd, F_SETFL, socketFlags);
    // 设置失败了
    if(ok == -1) {
        char BUFF[1024] = {0};
        strerror_r(errno, BUFF, 1024);
        MY_LOG(ERROR, "set [%d] nblock failed. F_SETFL failed, error is [%s]", 
            _Fd, BUFF);
        return false;
    }
    return true;
}

bool Socket::SetCloseExec(bool enable) const {
    if(_Fd < 0) {
        MY_LOG(ERROR, "socket fd is %d", _Fd);
        return false;
    }

    int socketFlags = 0;
    // 先获取当前fd上已经设置的属性
    socketFlags = fcntl(_Fd, F_GETFL);
    // 获取失败
    if(socketFlags == -1) {
        char BUFF[1024] = {0};
        strerror_r(errno, BUFF, 1024);
        MY_LOG(ERROR, "set [%d] close on exec failed. F_GETFL failed, error is [%s]", 
            _Fd, BUFF);
        return false;
    }

    // 如果是要设置成close on exec
    if(enable) {
        socketFlags |= FD_CLOEXEC;
    } else {
    // 如果要设置exec时不关闭
        socketFlags &= ~FD_CLOEXEC;
    }
    
    int ok = fcntl(_Fd, F_SETFL, socketFlags);
    // 设置失败了
    if(ok == -1) {
        char BUFF[1024] = {0};
        strerror_r(errno, BUFF, 1024);
        MY_LOG(ERROR, "set [%d] close on exec failed. F_SETFL failed, error is [%s]", 
            _Fd, BUFF);
        return false;
    }
    return true;
}

bool Socket::SetTcpNoDelay(bool enable) const {
    if(_Fd < 0) {
        MY_LOG(ERROR, "socket fd is %d", _Fd);
        return false;
    }
    int noDelay = enable ? 1 : 0;
    int ok = setsockopt(_Fd, IPPROTO_TCP, TCP_NODELAY, &noDelay, sizeof(noDelay));
    // 设置失败了
    if (ok < 0) {
        char BUFF[1024] = {0};
        strerror_r(errno, BUFF, 1024);
        MY_LOG(ERROR, "set [%d] tcp on delay failed, error is [%s]",
            _Fd, BUFF);
        return false;
    }
    return true;
}

bool Socket::SetSoLinger(bool enable, int seconds) const
{
    if(_Fd < 0) {
        MY_LOG(ERROR, "socket fd is %d", _Fd);
        return false;
    }

    // 创建linger配置结构体 并填入配置
    // 在SOCKET关闭时 是否等数据全部发送完 且收到对方确认的ack
    // l_linger是超时时间
    // 参考https://www.cnblogs.com/kuliuheng/p/3670353.html
    linger lingerValue;
    lingerValue.l_onoff = enable ? 1 : 0;
    lingerValue.l_linger = seconds;

    int ok = setsockopt(_Fd, SOL_SOCKET, SO_LINGER, &lingerValue, sizeof(lingerValue));
    // 设置失败了
    if (ok < 0) {
        char BUFF[1024] = {0};
        strerror_r(errno, BUFF, 1024);
        MY_LOG(ERROR, "set [%d] socket Linger failed, error is [%s]",
            _Fd, BUFF);
        return false;
    }
    return true;
}

bool Socket::SetTcpKeepAlive(int idleTime, int keepInterval, int cnt) const
{
    if (_Fd < 0) {
        MY_LOG(ERROR, "socket fd is %d", _Fd);
        return false;
    }
    // TCP_KEEPDILE 设置连接上如果没有数据发送的话，多久后发送keepalive探测分组，单位是秒
    // TCP_KEEPINTVL 前后两次探测之间的时间间隔，单位是秒
    // TCP_KEEPCNT 关闭一个非活跃连接之前的最大重试次数
    
    if (setsockopt(_Fd, IPPROTO_TCP, TCP_KEEPIDLE, &idleTime, 
                   sizeof(idleTime)) < 0) {
        char BUFF[1024] = {0};
        strerror_r(errno, BUFF, 1024);
        MY_LOG(ERROR, "set [%d] TCP_KEEPIDLE failed, error is [%s]",
            _Fd, BUFF);
        return false;
    }

    if (setsockopt(_Fd, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, 
                   sizeof(keepInterval)) < 0) {
        char BUFF[1024] = {0};
        strerror_r(errno, BUFF, 1024);
        MY_LOG(ERROR, "set [%d] TCP_KEEPINTVL failed, error is [%s]",
            _Fd, BUFF);
        return false;
    }

    if (setsockopt(_Fd, IPPROTO_TCP, TCP_KEEPCNT, &cnt, sizeof(cnt)) < 0) {
        char BUFF[1024] = {0};
        strerror_r(errno, BUFF, 1024);
        MY_LOG(ERROR, "set [%d] TCP_KEEPCNT failed, error is [%s]",
            _Fd, BUFF);
        return false;
    }
    return true;
}

bool Socket::SetSoRcvBuf(int buffSize) const
{
    if (_Fd < 0) {
        MY_LOG(ERROR, "socket fd is %d", _Fd);
        return false;
    }

    if (setsockopt(_Fd, SOL_SOCKET, SO_RCVBUF, &buffSize, sizeof(buffSize)) < 0) {
        char BUFF[1024] = {0};
        strerror_r(errno, BUFF, 1024);
        MY_LOG(ERROR, "set [%d] socket recv buff size failed, error is [%s]",
            _Fd, BUFF);
        return false;
    }
    return true;
}

bool Socket::SetSoSndBuf(int buffSize) const
{
    if (_Fd < 0) {
        MY_LOG(ERROR, "socket fd is %d", _Fd);
        return false;
    }

    if (setsockopt(_Fd, SOL_SOCKET, SO_SNDBUF, &buffSize, sizeof(buffSize)) < 0) {
        char BUFF[1024] = {0};
        strerror_r(errno, BUFF, 1024);
        MY_LOG(ERROR, "set [%d] socket send buff size failed, error is [%s]",
            _Fd, BUFF);
        return false;
    }
    return true;
}

bool Socket::SetOption(int option, const void* value, size_t len) const
{
    if (_Fd < 0) {
        MY_LOG(ERROR, "socket fd is %d", _Fd);
        return false;
    }
    if (setsockopt(_Fd, SOL_SOCKET, option, value, len) < 0) {
        char BUFF[1024] = {0};
        strerror_r(errno, BUFF, 1024);
        MY_LOG(ERROR, "set [%d] socket option failed, error is [%s]",
            _Fd, BUFF);
        return false;
    }
    return true;
}

bool Socket::GetSoError(int* code) const
{
    if (_Fd < 0) {
        MY_LOG(ERROR, "socket fd is %d", _Fd);
        return false;
    }
    socklen_t len = sizeof(int);
    if (getsockopt(_Fd, SOL_SOCKET, SO_ERROR, code, &len) < 0) {
        char BUFF[1024] = {0};
        strerror_r(errno, BUFF, 1024);
        MY_LOG(ERROR, "get [%d] socket error failed, error is [%s]",
            _Fd, BUFF);
        return false;
    }
    return true;
}

}
