#include "ssl/SslClient.h"
#include "middleware/BackendSslLayer.h"
#include "utils/CertOps.h"
#include "utils/Logger.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;

SslClient::SslClient(std::unique_ptr<SslClientConfig> config)
    : _config(std::move(config)), _ctx(nullptr, SSL_CTX_free), _socket(-1) {}

int32_t SslClient::CreateSocket() {
    int s = 0, res = 0;
    struct sockaddr_in localAddr;
    struct sockaddr_in serverAddr;
    struct hostent* resolvedServerAddress;
    struct timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;

    memset(&localAddr, 0, sizeof(localAddr));
    memset(&serverAddr, 0, sizeof(serverAddr));

    s = res = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
        LOG_ERROR("Unable to create socket (" << std::strerror(errno) << ")");
        return res;
    }

    if ((res = setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char*>(&timeout), sizeof(timeout))) < 0 ||
        (res = setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<char*>(&timeout), sizeof(timeout))) < 0) {
        LOG_ERROR("Unable to set socket options (" << std::strerror(errno) << ")");
        return res;
    }

    if ((resolvedServerAddress = gethostbyname(_config->serverIp.c_str())) == nullptr) {
        LOG_ERROR("No such host");
        exit(0);
    }

    // set local ip and port bindings
    localAddr.sin_family = AF_INET;
    localAddr.sin_port = 0;
    localAddr.sin_addr.s_addr = inet_addr(_config->localIp.c_str());

    if ((res = bind(s, reinterpret_cast<struct sockaddr*>(&localAddr), sizeof(localAddr))) < 0) {
        LOG_ERROR("Unable to bind (" << std::strerror(errno) << ")");
        return res;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(_config->serverPort);
    serverAddr.sin_addr =
        *reinterpret_cast<struct in_addr*>(resolvedServerAddress->h_addr_list[0]); // use the first ip avaiable

    if ((res = connect(s, reinterpret_cast<struct sockaddr*>(&serverAddr), sizeof(serverAddr))) < 0) {
        LOG_ERROR("Unable to connect (" << std::strerror(errno) << ")");
        return res;
    }

    return s;
}

bool SslClient::ConfigureSslContext(SSL_CTX_PTR ctx, const SslConfig& config) {
    auto clientConfig = reinterpret_cast<const SslClientConfig&>(config);

    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, nullptr);

    return true;
}

std::unique_ptr<BackendSslLayer> SslClient::Connect() {
    _socket = -1;
    _ctx = nullptr;

    if ((_ctx = CreateSslContext(*_config)) == nullptr || (_socket = CreateSocket()) <= 0) {
        return nullptr;
    }

    DEF_SSL(ssl, SSL_new(_ctx));
    SSL_set_fd(ssl, _socket);
    SSL_set_tlsext_host_name(ssl, _config->serverIp.c_str());

    auto bessl = std::make_unique<BackendSslLayer>(nullptr, std::move(ssl));

    if (bessl->Connect()) {
        LOG_TRACE("Connected to server " << _config->serverIp);
        return bessl;
    } else {
        return nullptr;
    }
}
