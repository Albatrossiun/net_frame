#include <src/util/time/time_tools.h>
namespace my_net {

#include <stdio.h>

int64_t TimeTools::GetTimeSec() {
    struct timeval tval;
    gettimeofday(&tval, NULL);
    return tval.tv_sec;
}


int64_t TimeTools::GetTimeMs() {
    struct timeval tval;
    gettimeofday(&tval, NULL);
    return tval.tv_sec * 1000 + tval.tv_usec / 1000;
}

int64_t TimeTools::GetTimeUs() {
    struct timeval tval;
    gettimeofday(&tval, NULL);
    return tval.tv_sec * 1000000 + tval.tv_usec;
}

}
