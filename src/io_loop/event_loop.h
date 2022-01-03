#ifndef __src_net_io_loop_event_loop__
#define __src_net_io_loop_event_loop__
#include <src/common/common.h>
#include <src/common/log.h>
#include <src/io_loop/io_handler.h>
#include <sys/epoll.h>
#include <src/thread/thread_pool.h>
#include <src/thread/runnable.h>
#include <src/io_loop/timer.h>
#include <src/io_loop/epoll.h>

namespace my_net{

class EventLoop : public Runnable{
public:
    EventLoop();
    virtual ~EventLoop();

    // 初始化
    bool Init();

    // 启动
    virtual void Run();

    // 停止
    void Stop();

    // 注册IO事件
    bool AddIOEvent(IOHandler* handler);

    // 修改或者删除IO事件
    bool ModifyIOEvent(IOHandler* handler);

    // 注册定时事件
    bool AddTimeEvent(TimeItemPtr timeItem, int timeout);

    // 删除定时事件
    bool DeleteTimeEvent(TimeItemPtr timeItem);

private:
    bool _Inited;
    bool _Stoped;
    EpollPtr _EpollPtr;
    TimerPtr _TimerPtr;
    std::mutex _Mutex;
};
typedef std::shared_ptr<EventLoop> EventLoopPtr;
}
#endif