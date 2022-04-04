#include <src/my_net/my_net.h>
#include <iostream>
#include <string>
#include <unistd.h>

using namespace my_net;
using namespace std;

class ServerPacketHandler : public MyNetUserHandler {
public:
    ServerPacketHandler()
    {}

    ~ServerPacketHandler()
    {}

    bool Init() {
        return true;
    }
private:
    bool ReadFile(const std::string& path, std::string& data) {
        const std::string root = "/home/ubuntu/work/my_net/resource";
        std::string p = root + path;
        fstream f(p);
        if(!f.good() || !f.is_open()) {
            // MY_LOG(ERROR, "read [%s] failed", p.c_str());
            return false;
        }
        f.seekg(0, std::ios::end);
        size_t size = f.tellg();
        std::string buffer(size, ' ');
        f.seekg(0);
        f.read(&buffer[0], size); 
        data = std::move(buffer);
        return true;
    }

    bool GetResurce(HttpRequestPacket* request, HttpResponsePacketPtr response, const std::string& path, int& code) {
        std::string buffer;
        if(!ReadFile(path, buffer)) {
            code = 404;
            std::string errhtml("<html><body>404 not found</body></html>");
            response->AppendBody(errhtml.c_str(), errhtml.size());
            return false;
        }
        response->AppendBody(buffer.c_str(), buffer.size());
        return true;
    }


private:
    virtual bool Process(HttpHandler& httpHandler, std::shared_ptr<HttpPacket>& packetPtr, int error) {
        if(error != 0) {
            if(error != BaseIoHandler::PROCESS_ERROR::PE_CONNECT_CLOSE) {
                MY_LOG(ERROR, "发生了错误[%d]", error);
            }
            httpHandler.Destroy();
            return false;
        }
        HttpServerHandler* serverHandler = dynamic_cast<HttpServerHandler*>(&httpHandler);
        HttpRequestPacket* packet = dynamic_cast<HttpRequestPacket*>(packetPtr.get());

        std::string uri = packet->GetUri();
        
        HttpResponsePacketPtr response(new HttpResponsePacket);
        response->SetHttpVersion(packet->GetHttpVersion());
        response->SetSerialNumber(packet->GetSerialNumber());
       
        bool ok = true;
        int code = 200;
        
        if(uri == "" || uri == "/" || uri == "/index.html") {
            ok = GetResurce(packet, response, "/index.html", code);
        } else if(packet->GetMethod() == HTTP_METHOD::HTTP_METHOD_GET){
            // 把参数剥离掉
            auto index = uri.find("?");
            if(index != std::string::npos) {
                uri = uri.substr(0, index);
            }
            // MY_LOG(DEBUG, "uri is [%s]", uri.c_str());
            ok = GetResurce(packet, response, uri, code);
        } else {
            ok = false;
            code = 400;
        }

        response->SetStatusCode(code);

        if(!serverHandler->Reply(response)) {
            MY_LOG(ERROR, "%s", "发送响应失败");
            return false;
        }
        return ok;
    }
};


int main() {
    MyNet myNet;
    MyNetConfig config;
    config._ThreadNum = 4;

    myNet.Init(config);

    myNet.StartServer("*:8989", MyNetUserHandlerPtr(new ServerPacketHandler));

    // 服务器永不退出
    while(1) {
        sleep(100);   
    }
    return 0;
}
