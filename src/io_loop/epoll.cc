#include <src/io_loop/epoll.h>
namespace my_net{

Epoll::Epoll()
    :_Inited(false),
     _EpollFd(-1)
{}

Epoll::~Epoll()
{
    if(_EpollFd >= 0) {
        close(_EpollFd);
    }
}

bool Epoll::Init()
{
    _EpollFd = epoll_create(999);
    if(_EpollFd < 0) {
        MY_LOG(ERROR, "create epoll failed [%d]", _EpollFd);
        return false;
    }
    _Inited = true;
    return true;
}

bool Epoll::AddEvent(IOHandler* handler) {
    if(_Inited == false) {
        MY_LOG(ERROR, "%s", "Add event failed, Epoll has not been inited");
        return false;
    }
    int fd = handler->GetSocketFd();
    if(fd <= 0) {
        MY_LOG(ERROR, "Add event into Epoll failed. fd is [%d]", fd);
        return false;
    }

    struct epoll_event event = {0};
    event.data.ptr = handler->CopyPtr();
    event.events = handler->GetEvent() | EPOLLET;
    int stat = epoll_ctl(_EpollFd, EPOLL_CTL_ADD, fd, &event);

    if(stat < 0) {
        char BUFF[1024] = {0};
        strerror_r(errno, BUFF, 1024);
        MY_LOG(ERROR, "Add event into Epoll failed, error is [%s]", BUFF);
        handler->Release();
        return false;
    }
    return true;
}

bool Epoll::ModifyEvent(IOHandler* handler) {
    // 要么删除 要么修改
    if(_Inited == false) {
        MY_LOG(ERROR, "%s", "Add event failed, Epoll has not been inited");
        return false;
    }
    int fd = handler->GetSocketFd();
    if(fd <= 0) {
        MY_LOG(ERROR, "Add event into Epoll failed. fd is [%d]", fd);
        return false;
    }
    int eventType = handler->GetEvent();
    int stat = 0;

    struct epoll_event event = {0};
    event.data.ptr = handler;
    event.events = eventType | EPOLLET;

    if(eventType == 0) {
        stat = epoll_ctl(_EpollFd, EPOLL_CTL_DEL, fd, &event);
        if(stat >= 0) {
            handler->Release();
        }
    } else {
        stat = epoll_ctl(_EpollFd, EPOLL_CTL_MOD, fd, &event);
    }
    if(stat < 0) {
        char BUFF[1024] = {0};
        strerror_r(errno, BUFF, 1024);
        MY_LOG(ERROR, "modify Epoll event failed. error is [%s]", BUFF);
        return false;
    }
    return true;
}

int Epoll::EpollWait(std::vector<IOHandler*>& handlers, int timeout) 
{
    if(_Inited == false) {
        MY_LOG(ERROR, "%s", "Add event failed, Epoll has not been inited");
        return -1;
    }
    struct epoll_event epollEvents[1024];
    handlers.clear();
    int num = epoll_wait(_EpollFd, epollEvents, 1024, timeout);
    if(num < 0) {
        return num;
    }
    handlers.resize(num);
    for(int i = 0 ; i < num; i++) {
        IOHandler* handler = ((IOHandler*)(epollEvents[i].data.ptr))->CopyPtr();
        handler->_CurrentEvent = epollEvents[i].events;
        handlers[i] = handler;
    }
    return num;
}

}