#include <src/io_loop/timer.h>
#include <src/util/time/time_tools.h>

namespace my_net {

//-------TimeItem---------
TimeItem::TimeItem()
    :_Time(0)
{}

TimeItem::~TimeItem()
{}

//-------Timer------------

Timer::Timer()
    :_NetExpireTime(0)
{}

Timer::~Timer()
{}

bool Timer::AddTimeItem(TimeItemPtr timeItem, int timeout) {
    // 如果用户设置的超时时间小于0 则设置超时时间为0
    if(timeout < 0) {
        timeout = 0;
    }

    // 当前时间 + 设置的超时时间 = 超时发送的时间 
    int64_t expire = TimeTools::GetTimeMs() + timeout;
    timeItem->SetTime(expire);

    std::unique_lock<std::mutex> lock(_Mutex);
    // 下一个超时时间永远是距离现在最近的
    if(_NetExpireTime > expire || _NetExpireTime == 0) {
        _NetExpireTime = expire;
    }
    _TimeMap[expire].push_back(timeItem);
    return true;
}

bool Timer::DeleteTimeItem(TimeItemPtr timeItem) {
    int64_t expire = timeItem->GetTime();

    std::unique_lock<std::mutex> lock(_Mutex);
    // 从map 中找到对应时间的队列
    TimeMapIter it = _TimeMap.find(expire);
    if(it == _TimeMap.end()) {
        // 没找到要删除的timeItem
        return false;
    }
    TimeItemList& timeList = it->second;
    // 从队列中找这个timeItem
    TimeItemListIter timeItemIt;
    for(timeItemIt = timeList.begin() ; timeItemIt != timeList.end(); timeItemIt++) {
        // 比较原始地址
        if(timeItemIt->get() == timeItem.get()) {
            break; 
        }
    }
    if(timeItemIt == timeList.end()) {
        // 没找到要删除的timeItem
        return false;
    }
    
    // 删除
    timeList.erase(timeItemIt);
    // 如果删除后 这个队列空了 要从map上删除这个队列
    if(timeList.empty()) {
        _TimeMap.erase(it);
        // 删除完以后 要看下是不是需要更新_NetExpireTime
        // 如果删除的就是下一个最近的超时item 就要更新
        if(_NetExpireTime == expire) {
            if(_TimeMap.empty()) {
                // 如果一个超时item都没有了
                _NetExpireTime = 0;
            } else {
                // 取出最近的超时时间
                // 因为map的实现是红黑树
                // 所以这里取map的begin的元素 一定就是最小的
                // 也就是最近的超时时间
                _NetExpireTime = _TimeMap.begin()->first;           
            }
        }
    }

    // 清空一下这个item的时间设置
    timeItem->SetTime(0);
    return true;
}

int Timer::GetNetExpireTime() {
    std::unique_lock<std::mutex> lock(_Mutex);
    // 如果当前没有timeItem
    if(_TimeMap.empty()) {
        return -1;
    }
    // 获取当前时间
    int64_t now = TimeTools::GetTimeMs();
    int time = _NetExpireTime - now; // 计算距离最近的超时item还有多长时间

    // 如果已经过了最近的超时时间 返回0 表示已经过了最近的超时时间了
    if(time < 0) {
        return 0;
    }
    return time;
}

void Timer::Process() {
    // 上锁
    std::shared_ptr<std::unique_lock<std::mutex>> lockPtr(new std::unique_lock<std::mutex>(_Mutex)); 
    int64_t now = TimeTools::GetTimeMs();
    while(!_TimeMap.empty()) {
        TimeMapIter it = _TimeMap.begin();
        // 没有超时事件了
        if(it->first > now) {
            _NetExpireTime = it->first;
            break;
        }
    
        // 开始处理超时时间
        TimeItemList& timeList = it->second;
        // 这个链上没有事件要处理了
        if(timeList.empty()) {
            _TimeMap.erase(it);
            if(_TimeMap.empty()) {
                _NetExpireTime = 0;
            }
            continue;
        }

        // 每次只处理链上的一个事件
        TimeItemPtr timeItem = timeList.front();
        timeList.pop_front();

        // 解锁 因为处理超时时间的可能需要耗费比较久的时间 这个时候要解锁
        // 其他线程就可以在这段时间里继续向计时器中注册超时事件
        lockPtr.reset();
        timeItem->SetTime(0);
        timeItem->Process();
        // 处理完事件后 要重新加锁
        lockPtr.reset(new std::unique_lock<std::mutex>(_Mutex)); 
    }
    // 结束后 lockPtr会自动释放 无需手动解锁
}

}
