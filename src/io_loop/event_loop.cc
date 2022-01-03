#include <src/io_loop/event_loop.h>
#include <src/util/time/time_tools.h>
#include <src/io_loop/epoll.h>
#include <src/io_loop/io_handler.h>

namespace my_net{

EventLoop::EventLoop()
    :_Inited(false),
     _Stoped(true)
{}

EventLoop::~EventLoop()
{
    Stop();
}

bool EventLoop::Init() {
    if(_Inited == true) {
        MY_LOG(ERROR,"%s","event loop has been inited.");
        return false;
    }
    _EpollPtr.reset(new Epoll);
    _EpollPtr->Init();
    _TimerPtr.reset(new Timer);
    _Inited = true;
    return true;
}

void EventLoop::Stop() {
    _Inited = false;
}

void EventLoop::Run() {
    _Stoped = false;
    while(!_Stoped) {
        int time = _TimerPtr->GetNetExpireTime() - TimeTools::GetTimeMs();
        if(time <= 0) {
            time = 20;
        } else if (time > 200) {
            time = 200;
        }
        std::vector<IOHandler*> events;
        int num = _EpollPtr->EpollWait(events, time);
    }
}

}