#ifndef __my_net_src_util_url_tools_addr_parser__
#define __my_net_src_util_url_tools_addr_parser__
#include <src/common/common.h>
#include <src/common/log.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>

namespace my_net {

class AddrParser {
public:
    enum PROTO {
        UNKNOW, 
        TCP,
        HTTP,
        HTTPS
    };
    static bool ParseUrl(const std::string& url, int& proto, in_addr_t& addr, uint16_t& port);
};
}
#endif // __my_net_src_util_url_tools_addr_parser__

