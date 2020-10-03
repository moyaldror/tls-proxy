#include <arpa/inet.h>
#include <cstring>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "middleware/FrontendSslLayer.h"

std::shared_ptr<Message> FrontendSslLayer::ProcessMessage(std::shared_ptr<Message> msg) {
    std::shared_ptr<Message> res = nullptr;

    if (_ssl == nullptr) {
        res = std::make_shared<ErrorMessage>("SSL object is null");
    } else {
        // Finish the connection to the client
        if (SSL_in_accept_init(_ssl) && DoSslConnectAccept()) {
            std::string clientRequest = DoSslRead();

            if (clientRequest.empty()) {
                res = std::make_shared<ErrorMessage>("Empty request");
            } else {
                res = PushToNext(std::make_shared<StringDataMessage>(clientRequest));
                auto data = std::dynamic_pointer_cast<StringDataMessage>(res);

                if (data == nullptr) {
                    res = std::make_shared<ErrorMessage>("Message should be of type StringData");
                } else {
                    DoSslWrite(data->Data);
                }
            }

            DoClose(true);
        }
    }

    return res;
}