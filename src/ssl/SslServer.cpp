#include "ssl/SslServer.h"
#include "http/HttpMessage.h"
#include "http/HttpMessageBuilder.h"
#include "middleware/BackendSslLayer.h"
#include "middleware/FrontendSslLayer.h"
#include "middleware/HttpRewriteLayer.h"
#include "middleware/LogHttpLayer.h"
#include "ssl/SslClient.h"
#include "utils/CertOps.h"
#include "utils/Logger.h"
#include "utils/ThreadPool.h"
#include <arpa/inet.h>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <regex>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

SslServer::SslServer(std::unique_ptr<SslServerConfig> config)
    : _config(std::move(config)), _threadPool(std::make_unique<ThreadPool>(10)), _running(false),
      _ctx(nullptr, SSL_CTX_free), _socket(-1) {}

int32_t SslServer::CreateSocket() {
    int s = 0, res = 0, opt = 1;
    struct sockaddr_in localAddr;

    memset(&localAddr, 0, sizeof(localAddr));

    localAddr.sin_family = AF_INET;
    localAddr.sin_port = htons(_config->listenPort);
    localAddr.sin_addr.s_addr = inet_addr(_config->listenIp.c_str());

    s = res = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
        LOG_ERROR("Unable to create socket (" << std::strerror(errno) << ")");
        return res;
    }

    if ((res = setsockopt(s, SOL_SOCKET, SO_REUSEPORT, reinterpret_cast<char*>(&opt), sizeof(opt))) < 0) {
        LOG_ERROR("Unable to set socket options (" << std::strerror(errno) << ")");
        return res;
    }

    if ((res = bind(s, reinterpret_cast<struct sockaddr*>(&localAddr), sizeof(localAddr))) < 0) {
        LOG_ERROR("Unable to bind (" << std::strerror(errno) << ")");
        return res;
    }

    if ((res = listen(s, 1)) < 0) {
        LOG_ERROR("Unable to listen (" << std::strerror(errno) << ")");
        return res;
    }

    LOG_INFO("Listeining on " << _config->listenIp.c_str() << ":" << _config->listenPort << " for new connections...");

    return s;
}

X509_OPTR SslServer::FetchCertificate(const std::string& serverName, SSL_PTR ssl, int32_t port) {
    std::string certificateFile = "./certs/" + serverName;
    DEF_X509(res, nullptr);

    auto localServer = reinterpret_cast<SslServer*>(SSL_get_ex_data(ssl, 0));
    // dont want openssl to release SslServer*
    SSL_set_ex_data(ssl, 0, nullptr);

    if (localServer == nullptr) {
        res = nullptr;
        return res;
    }

    auto conf = std::make_unique<SslClientConfig>();
    conf->isServer = false;
    conf->localIp = localServer->GetConfig().listenIp;
    conf->serverIp = serverName;
    conf->serverPort = port;
    SslClient sclient(std::move(conf));

    // we want to connect either way, but clone the certificate only if its not
    // present in the cache
    auto bessl = sclient.Connect();

    if (bessl == nullptr) {
        res = nullptr;
        return res;
    }

    if (access(certificateFile.c_str(), F_OK) != -1) {
        res = x509::LoadPemCert(certificateFile);
    } else {
        if ((res = bessl->GetCertificate()) != nullptr &&
            (res = x509::ClonePemCert(std::move(res), localServer->GetConfig().caCertificateFile,
                                      localServer->GetConfig().keyFile)) != nullptr) {
            std::string certFileName = "./certs/" + serverName;
            x509::SaveCertificateToPemFile(res, certFileName);
        }
    }

    auto lastHandelingLayer = reinterpret_cast<HandlerLayer*>(SSL_get_ex_data(ssl, 1));
    // dont want openssl to release HandlerLayer*
    SSL_set_ex_data(ssl, 1, nullptr);
    if (lastHandelingLayer != nullptr) {
        lastHandelingLayer->SetNext(std::move(bessl));
    }

    return res;
}

int32_t SslServer::ServerNameCb(SSL_PTR ssl, int* ad, void* arg) {
    int* alreadyFetched = nullptr;

    if (ssl == nullptr) {
        return SSL_TLSEXT_ERR_NOACK;
    }

    if ((alreadyFetched = reinterpret_cast<int*>(SSL_get_ex_data(ssl, 2))) != nullptr) {
        delete alreadyFetched;
        SSL_set_ex_data(ssl, 2, nullptr);

        return SSL_TLSEXT_ERR_NOACK;
    }

    const char* servername = SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);
    if (!servername || servername[0] == '\0') {
        LOG_ERROR("No SNI");
        return SSL_TLSEXT_ERR_NOACK;
    }

    LOG_TRACE("Got request with sni: " << servername);
    std::string sni(servername);

    auto x509 = FetchCertificate(sni, ssl);

    if (x509 == nullptr) {
        LOG_ERROR("Failed to fetch certificate");
        return SSL_TLSEXT_ERR_ALERT_FATAL;
    } else if (SSL_use_certificate(ssl, x509) <= 0) {
        LOG_ERROR("Failed to use certificate");
        return SSL_TLSEXT_ERR_ALERT_FATAL;
    }

    return SSL_TLSEXT_ERR_OK;
}

bool SslServer::HandleHttpConnect(int32_t clientSocket, SSL_PTR ssl) {
    char content[1024];
    int32_t bytes = 0;
    bool res = true;

    if ((bytes = recv(clientSocket, &content, sizeof(content) - 1, MSG_PEEK)) >= 0) {
        content[bytes] = '\0';

        try {
            HttpMessage httpMessage(content);

            if (httpMessage.IsRequest() && httpMessage.Method() == HttpMessage::HttpMethod::CONNECT &&
                (bytes = recv(clientSocket, &content, sizeof(content) - 1, 0)) >= 0 &&
                bytes == static_cast<int32_t>(httpMessage.OriginalMessage().size())) {
                LOG_TRACE("Handling HTTP CONNECT");
                auto x509 = FetchCertificate(httpMessage.Host(), ssl, httpMessage.Port());

                if (x509 == nullptr) {
                    LOG_ERROR("Failed to fetch certificate");
                    res = false;
                } else if (SSL_use_certificate(ssl, x509) <= 0) {
                    LOG_ERROR("Failed to use certificate");
                    res = false;
                } else {
                    HttpMessageBuilder msg(false);
                    std::string& status = msg.Status();
                    std::string httpReply;

                    status = "200 Connection Established";
                    msg.Headers()["host"] = httpMessage.Headers().find("host")->second;
                    httpReply = msg.Build().ToString();

                    write(clientSocket, httpReply.c_str(), httpReply.size());

                    SSL_set_ex_data(ssl, 2, new int(1));
                }
            }
        } catch (std::invalid_argument* e) {
            LOG_TRACE("Not an HTTP Connect");
        }
    }

    return res;
}

bool SslServer::ConfigureSslContext(SSL_CTX_PTR ctx, const SslConfig& config) {
    auto serverConfig = reinterpret_cast<const SslServerConfig&>(config);

    SSL_CTX_set_ecdh_auto(ctx, 1);

    if (SSL_CTX_use_PrivateKey_file(ctx, serverConfig.keyFile.c_str(), SSL_FILETYPE_PEM) <= 0 ||
        SSL_CTX_set_cipher_list(ctx, serverConfig.cipherList.c_str()) <= 0 ||
        SSL_CTX_set_mode(ctx, SSL_MODE_RELEASE_BUFFERS) <= 0 ||
        SSL_CTX_set_tlsext_servername_callback(ctx, ServerNameCb) <= 0) {
        LOG_ERROR("Failed to set OpenSsl Context options");
        ERR_print_errors_fp(stderr);
        return false;
    }

    return true;
}

// not even based on purpose
void SslServer::Start() {
    _socket = -1;
    _ctx = nullptr;

    if ((_ctx = CreateSslContext(*_config)) == nullptr || (_socket = CreateSocket()) <= 0) {
        return;
    }

    _running = true;
    _threadPool->Start();

    /* Handle connections */
    while (_running) {
        struct sockaddr_in addr;
        uint len = sizeof(addr);

        int client = accept(_socket, reinterpret_cast<struct sockaddr*>(&addr), &len);
        if (client < 0) {
            LOG_ERROR("Unable to accept (" << std::strerror(errno) << ")");
            return;
        }

        LOG_TRACE("Adding task to handle client socket " << client);
        _threadPool->AddTask([this, client] {
            // create ssl object got the new client
            DEF_SSL(ssl, SSL_new(_ctx));
            SSL_set_fd(ssl, client);
            SSL_set_ex_data(ssl, 0, this);

            // build layers according to processing order
            auto middlewareRewrite = std::make_unique<HttpRewriteLayer>(nullptr);
            SSL_set_ex_data(ssl, 1, middlewareRewrite.get()); // do it now because after move this pointer is invalid

            auto middlewareLogger = std::make_unique<LogHttpLayer>(std::move(middlewareRewrite));

            if (HandleHttpConnect(client, ssl)) {
                LOG_TRACE("Create Ssl object " << ssl.Get() << " for socket " << client);

                FrontendSslLayer fssl(std::move(middlewareLogger), std::move(ssl));

                LOG_TRACE("Starting to process messages");
                auto res = fssl.ProcessMessage(std::make_shared<EmptyMessage>());
            } else {
                SSL_set_ex_data(ssl, 0, nullptr);
                SSL_set_ex_data(ssl, 1, nullptr);
                close(client);
            }
        });
    }
}

void SslServer::Stop() {
    _running = false;

    _threadPool->Stop();

    close(_socket);
    _socket = -1;
    _ctx = nullptr;
}
