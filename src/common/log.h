#ifndef __my_net_src_common_log__
#define __my_net_src_common_log__

enum LOG_LEVEL{
    DEBUG,   // 0
    WARN,    // 1
    INFO,    // 2
    ERROR,   // 3
};

#define DEBUG LOG_LEVEL::DEBUG
#define WARN LOG_LEVEL::WARN
#define INFO LOG_LEVEL::INFO
#define ERROR LOG_LEVEL::ERROR

#define LEVEL 0

#define MY_LOG(level, format, ...) \
    do{ \
        if(level < LEVEL) break;\
        switch(level) { \
        case DEBUG : \
            printf("[DEBUG][%s][%d]" format "\n", \
                __FILE__, __LINE__, __VA_ARGS__);\
            break;\
        case WARN : \
            printf("[WARN][%s][%d]" format "\n", \
                __FILE__, __LINE__, __VA_ARGS__);\
            break;\
        case INFO: \
            printf("[INFO][%s][%d]" format "\n", \
                __FILE__, __LINE__, __VA_ARGS__);\
            break;\
        case ERROR: \
            printf("[ERROR][%s][%d]" format "\n", \
                __FILE__, __LINE__, __VA_ARGS__);\
        }\
    }while(0);

#endif // __my_net_src_common_log__
