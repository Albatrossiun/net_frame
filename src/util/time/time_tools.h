#ifndef __my_net_src_util_time_time_tools__
#define __my_net_src_util_time_time_tools__

#include <src/common/common.h>
#include <sys/time.h>
#include <stdint.h>

namespace my_net {

class TiemTools {
public:
    // 获取当前时间 单位 秒
    static int64_t GetTimeSec();

    // 获取当前时间 单位 毫秒
    static int64_t GetTimeMs();

    // 获取当前时间 单位 微秒 
    static int64_t GetTimeUs();
}; 

}

#endif // __my_net_src_util_time_time_tools__
