#include <src/util/url_tools/addr_parser.h>
namespace my_net {

// "http://www.baidu.com"
// "http://127.0.0.1:8989"
// "www.baidu.com"

bool AddrParser::ParseUrl(const std::string& url, int& proto, in_addr_t& addr, uint16_t& port) {
    std::vector<std::string> tokens;
    // 按照冒号分隔
    {
        size_t n = 0;
        size_t last = 0;
        while(n != std::string::npos) {
            n = url.find(":", n);
            if(n != std::string::npos) {
                if(n != last) {
                    tokens.emplace_back(url.substr(last, n - last));
                }
                n += 1;
                last = n;
            }
        }
        if(last < url.size()) {
            tokens.emplace_back(url.substr(last, url.size() - last));
        }
    }

    if(tokens.size() == 0 || tokens.size() > 3) {
        MY_LOG(ERROR, "parser url [%s] failed", url.c_str()); 
        return false;
    }

    std::string protoStr;
    std::string hostStr;
    std::string portStr;

    if(tokens.size() == 1) {
        protoStr = "http";
        hostStr = tokens[0];
    } else if(tokens.size() == 2) {
        // http //www.baidu.com
        if(tokens[1].size() >= 2 && tokens[1][0] == '/') {
            protoStr = tokens[0];
            hostStr = tokens[1].substr(2, tokens[1].size() - 2);;
        } else { //127.0.0.1 8080
            protoStr = "http";
            hostStr = tokens[0];
            portStr = tokens[1];
        }
    } else {
        protoStr = tokens[0];
        hostStr = tokens[1].substr(2, tokens[1].size() - 2);
        portStr = tokens[2];
    }


    proto = PROTO::UNKNOW; 
    if(protoStr == "tcp") {
        proto = PROTO::TCP;
    } else if(protoStr == "http") {
        proto = PROTO::HTTP;
    } else if(protoStr == "https") {
        proto = PROTO::HTTPS;
    } else {
        MY_LOG(ERROR, "peraer url faild, unknow procoto type : [%s]", protoStr.c_str());
        return false;
    }
    
    if(hostStr == "*") {
        addr = INADDR_ANY;
    } else {
        // 使用DNS解析域名
        addrinfo hint;
        addrinfo* result;
        memset(&hint, 0, sizeof(hint));
        hint.ai_family = AF_INET;
        if (getaddrinfo(hostStr.c_str(), protoStr.c_str(), &hint, &result) != 0) { 
            MY_LOG(ERROR, "getaddrinfo of [%s] failed", hostStr.c_str());
            return false;
        }
        addr = (uint32_t)((sockaddr_in *)(result->ai_addr))->sin_addr.s_addr;
        freeaddrinfo(result); 
        if(portStr.empty()) {
            port = ntohs(((sockaddr_in *)(result->ai_addr))->sin_port);
            //MY_LOG(DEBUG, "%u", port);
        }
    }
    if(!portStr.empty()) {
        port = atoi(portStr.c_str());
    }
    return true; 
}

}