#ifndef __src_net_io_loop_epoll__
#define __src_net_io_loop_epoll__
#include <src/common/common.h>
#include <src/common/log.h>
#include <src/io_loop/io_handler.h>
#include <sys/epoll.h>

namespace my_net{

class Epoll {
public:
    Epoll();
    ~Epoll();

    bool Init();
    bool AddEvent(IOHandler* handler);
    bool ModifyEvent(IOHandler* handler);
    int EpollWait(std::vector<IOHandler*>& Handlers, int timeout);

private:
    bool _Inited;
    int _EpollFd;
};
typedef std::shared_ptr<Epoll> EpollPtr;

}

#endif
