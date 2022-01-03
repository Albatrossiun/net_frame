#ifndef __my_net_src_runnable__
#define __my_net_src_runnable__

#include <src/common/common.h>
#include <src/common/log.h>

namespace my_net {

class Runnable {
public:
    Runnable();
    virtual ~Runnable();
    virtual void Run();
};
typedef std::shared_ptr<Runnable> RunnablePtr;

} // end namespace
#endif // __my_net_src_runnable__
