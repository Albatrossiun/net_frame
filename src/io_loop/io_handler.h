#ifndef __my_net_src_io_loop_io_handler__
#define __my_net_src_io_loop_io_handler__

#include <src/common/common.h>
#include <src/socket/socket.h>
#include <atomic>
namespace my_net {
class Epoll;
class IOHandler {
    friend class Epoll;
public:
    virtual ~IOHandler() {}

    virtual bool DoRead() = 0;
    virtual bool DoWrite() = 0;

    IOHandler* CopyPtr() {
        ++_ReferenceCount;
        return this;
    }

    void Release() {
        --_ReferenceCount;
        if(_ReferenceCount == 0) {
            delete this;
        }
    }

    int GetSocketFd() {
        return _Socket->GetFd();
    }

    void SetEvent(int type) { _EopllEventType = type; }
    int GetEvent() { return _EopllEventType;}

    void SetCurrentEvent(int event) { _CurrentEvent = event; }
    int GetCurrentEvent() { return _CurrentEvent; }

    void SetSocketPtr(SocketPtr socket) { _Socket = socket; }
    SocketPtr GetSocketPtr() { return _Socket; }
protected:// 不允许创建在栈上 必须用指针进行访问
    IOHandler()
        :_EopllEventType(0),
         _ReferenceCount(1)
    {}

    IOHandler(const IOHandler&) = delete;
    IOHandler(const IOHandler&&) = delete;
    IOHandler& operator=(const IOHandler&) = delete;
    IOHandler& operator=(const IOHandler&&) = delete;
protected: 
    SocketPtr _Socket;
    int _EopllEventType; // 注册的事件
    int _CurrentEvent; // epoll_wait 得到的事件
    volatile std::atomic<int> _ReferenceCount;
};
}
#endif