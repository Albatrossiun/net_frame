#include <src/thread/runnable.h>
#include <src/common/common.h>

namespace my_net {

Runnable::Runnable()
{}

Runnable::~Runnable()
{}

void Runnable::Run()
{
    MY_LOG(ERROR, "%s", "not define.")
}
}
