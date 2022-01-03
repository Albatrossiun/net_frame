#ifndef __src_net_io_loop_timer_h__
#define __src_net_io_loop_timer_h__

#include <src/common/common.h>

namespace my_net{

class TimeItem {
public:
    TimeItem();
    virtual ~TimeItem();
    virtual void Process() {}
    void SetTime(int64_t time) { _Time = time; }
    int64_t GetTime() const { return _Time; }

protected:
    int64_t _Time; 
};
typedef std::shared_ptr<TimeItem> TimeItemPtr;

// 计时器
//     将若干个超时时间设置到计时器里
//     可以从计时器里获取到最近的超时时间
class Timer {
public:
    typedef std::list<TimeItemPtr> TimeItemList;
    typedef TimeItemList::const_iterator TimeItemListCIter;
    typedef TimeItemList::iterator TimeItemListIter;
    typedef std::map<int64_t, TimeItemList> TimeMap;
    typedef TimeMap::const_iterator TimeMapCIter;
    typedef TimeMap::iterator TimeMapIter;
public:
    Timer();

    virtual ~Timer();

    // 增加一个时间事件
    virtual bool AddTimeItem(TimeItemPtr timeItem, int timeout);

    // 删除一个时间事件
    virtual bool DeleteTimeItem(TimeItemPtr timeItem);

    // 获取距离最近的超时item还有多长时间
    virtual int GetNetExpireTime();

    // 处理超时事件
    virtual void Process();

private:
    std::mutex _Mutex;    
    TimeMap _TimeMap;
    int64_t _NetExpireTime;
};
typedef std::shared_ptr<Timer> TimerPtr;

}
#endif
