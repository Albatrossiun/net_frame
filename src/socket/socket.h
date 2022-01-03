#ifndef __my_net_src_socket_socket__
#define __my_net_src_socket_socket__

#include <src/common/common.h>
#include <src/common/log.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

namespace my_net {
class Socket
{
public:
    Socket();
    virtual ~Socket();

    void SetFd(int fd) { _Fd = fd; }
    int GetFd() const { return _Fd; }
    // 从socket中读数据
    int Read(void* buf, size_t len) const;
    // 往socket中写数据
    int Write(const void* buf, size_t len) const;
    // 关闭读端和写端 释放文件描述符
    void Close();
    // 关闭写端
    void ShutdownWrite();
    // 关闭读端
    void ShutdownRead();
    // 设置非阻塞
    bool SetNonBlocking(bool enable) const;
    // 设置exec时 关闭此文件描述符
    bool SetCloseExec(bool enable) const;
    // 设置TCP包马上发送 不缓存
    bool SetTcpNoDelay(bool enable) const;
    // 设置SOCKET在关闭时 是否需要等待缓存区的数据发送完 
    bool SetSoLinger(bool enable, int seconds) const;
    // 设置TCP长连接属性
    bool SetTcpKeepAlive(int idleTime, int keepInterval, int cnt) const;
    // 设置缓存大小(接收时缓存)
    bool SetSoRcvBuf(int buffSize) const;
    // 设置缓存大小(发送时缓存)
    bool SetSoSndBuf(int buffSize) const;
    // 自由设置socket属性
    bool SetOption(int option, const void* value, size_t len) const;
    // 获取发生在这个socket上的错误码
    bool GetSoError(int* code) const;
protected:
    int _Fd;
private:
    // 设置为不可拷贝
    Socket(Socket& s) = delete;
    Socket& operator=(Socket& s) = delete;
};
typedef std::shared_ptr<Socket> SocketPtr;

}

#endif 