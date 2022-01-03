#include <src/thread/thread_pool.h>
namespace my_net {

ThreadPool::ThreadPool()
    :_Name("unknow"),
     _Size(0),
     _Running(THREADPOOL_STAT::STOP)
{}

ThreadPool::ThreadPool(const std::string name, const size_t size)
    :_Name(name),
     _Size(size),
     _Running(THREADPOOL_STAT::STOP)
{}

ThreadPool::~ThreadPool() {
    Stop();
}

void ThreadPool::Runner() {
    while(true) {
        RunnablePtr task;
        // 出了这个作用域后 lock将析构 锁自动释放
        {
            std::unique_lock<std::mutex> lock(_Mutex);
            while(_TaskQueue.empty() && (_Running == THREADPOOL_STAT::RUNNING)) {
                _ConditionVariable.wait(lock);
            }
            if(_Running != THREADPOOL_STAT::RUNNING) {
                break;
            }
            task = _TaskQueue.front();
            _TaskQueue.pop();
        }
        if(task != NULL) {
            task->Run();
        }
    }
    while(true) {
        RunnablePtr task;
        // 完成剩余的任务
        {
            std::unique_lock<std::mutex> lock(_Mutex);
            if(_TaskQueue.empty()) {
                // 没有任务了
                break;
            }
            task = _TaskQueue.front();
            _TaskQueue.pop();
        }
        if(task != NULL) {
            task->Run();
        }
    }
}

bool ThreadPool::Start() {
    // 使用这个类进行加锁操作
    std::unique_lock<std::mutex> lock(_Mutex);
    if(_Running != THREADPOOL_STAT::STOP) {
        MY_LOG(ERROR, "start [%s] failed, already started", _Name.c_str());
        return false;
    }    
    _Running = THREADPOOL_STAT::RUNNING;

    for(size_t i = 0; i < _Size; i++) {
        _Pool.push_back(std::move(
            std::thread(&ThreadPool::Runner, this)
        ));
    }
    MY_LOG(INFO, "thread pool [%s] started.", _Name.c_str());
    return true;
}

void ThreadPool::Stop() {
    {
        std::unique_lock<std::mutex> lock(_Mutex);
        if(_Running != THREADPOOL_STAT::RUNNING) {
            return;
        }
        _Running = THREADPOOL_STAT::STOPPING;
        _ConditionVariable.notify_all();
    }
    for(auto& t : _Pool) {
        t.join();
    }
    {
        std::unique_lock<std::mutex> lock(_Mutex);
        _Running = THREADPOOL_STAT::STOP;
        _Pool.clear();
    }

    MY_LOG(INFO, "thread pool [%s] stopped", _Name.c_str());
}

bool ThreadPool::AddTask(RunnablePtr task) {
    std::unique_lock<std::mutex> lock(_Mutex);
    if(_Running != THREADPOOL_STAT::RUNNING) {
        MY_LOG(ERROR, "Add task into [%s] failed, not running.", _Name.c_str());
    }
    _TaskQueue.push(task);
    _ConditionVariable.notify_one();
    return true;
}

} // end namespace
