#include <src/util/time/time_tools.h>
namespace my_net {

#include <stdio.h>

int64_t TiemTools::GetTimeSec() {
    struct timeval tval;
    gettimeofday(&tval, NULL);
    return tval.tv_sec;
}


int64_t TiemTools::GetTimeMs() {
    struct timeval tval;
    gettimeofday(&tval, NULL);
    return tval.tv_sec * 1000 + tval.tv_usec / 1000;
}

int64_t TiemTools::GetTimeUs() {
    struct timeval tval;
    gettimeofday(&tval, NULL);
    return tval.tv_sec * 1000000 + tval.tv_usec;
}

}
