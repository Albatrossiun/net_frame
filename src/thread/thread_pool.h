#ifndef __my_net_src_thread_thread_pool__
#define __my_net_src_thread_thread_pool__

#include <src/common/common.h>
#include <src/common/log.h>
#include <src/thread/runnable.h>
#include <thread>
#include <condition_variable>


namespace my_net {

enum THREADPOOL_STAT{
    STOP, // 0 停止的
    RUNNING, // 1 正在运行的
    STOPPING, // 2 正在停止的
};

class ThreadPool {
public:
    ThreadPool();
    ThreadPool(const std::string name, const size_t size);
    ~ThreadPool();

    bool AddTask(RunnablePtr task);

    bool SetName(const std::string name) {
        if(_Running) return false;
        _Name = name;
        return true;
    }
    std::string GetName() const { return _Name; }
    bool SetThreadNum(const size_t size) {
        if(_Running) return false;
        _Size = size;
        return true;
    }
    size_t GetThreadNum() const {return _Size; }

    bool Start();
    void Stop();
private:
    void Runner();
private:
    std::string _Name;
    size_t _Size;
    volatile THREADPOOL_STAT _Running;
    std::vector<std::thread> _Pool;
    std::queue<RunnablePtr> _TaskQueue;

    std::mutex _Mutex;
    std::condition_variable _ConditionVariable;
};

} // end namespace

#endif // __my_net_src_thread_thread_pool__
