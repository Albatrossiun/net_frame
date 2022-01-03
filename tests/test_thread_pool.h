#include <src/common/common.h>
#include <src/thread/thread_pool.h>
#include <src/thread/runnable.h>

using namespace std;
using namespace my_net;

class MyJob : public Runnable {
public:
    void Run() {
        cout << "Hello world!" << endl;
    }
};

bool TestThreadPool() {
    ThreadPool tp("Test", 10);
    tp.Start();
    tp.AddTask(RunnablePtr(new MyJob()));
    tp.AddTask(RunnablePtr(new MyJob()));
    tp.AddTask(RunnablePtr(new MyJob()));
    tp.AddTask(RunnablePtr(new MyJob()));
    tp.Stop();
    return true;
}