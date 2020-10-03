#include <arpa/inet.h>
#include <cstring>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "middleware/BackendSslLayer.h"

std::shared_ptr<Message> BackendSslLayer::ProcessMessage(std::shared_ptr<Message> msg) {
    std::shared_ptr<Message> res = nullptr;
    auto data = std::dynamic_pointer_cast<StringDataMessage>(msg);

    if (_ssl == nullptr) {
        res = std::make_shared<ErrorMessage>("SSL object is null");
    } else if (data == nullptr) {
        res = std::make_shared<ErrorMessage>("Message should be of type StringData");
    } else {
        // Check if the backend ssl connection was already established
        if (SSL_is_init_finished(_ssl) <= 0 && !Connect()) {
            ERR_print_errors_fp(stderr);
        } else {
            DoSslWrite(data->Data);
            std::string serverResponse = DoSslRead();
            DoClose(true);
            res = std::make_shared<StringDataMessage>(serverResponse);
        }
    }

    return res;
}

X509_PTR BackendSslLayer::GetCertificate() {
    DEF_X509(res, nullptr);

    // Check if the backend ssl connection was already established
    if (SSL_is_init_finished(_ssl) <= 0 && !Connect()) {
        ERR_print_errors_fp(stderr);
    } else {
        res = SSL_get_peer_certificate(_ssl);
    }

    return res;
}

bool BackendSslLayer::Connect() { return DoSslConnectAccept(); }