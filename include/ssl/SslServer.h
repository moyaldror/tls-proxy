#pragma once

#include "common/OpenSslCpp.h"
#include "ssl/SslConfig.h"
#include "ssl/SslHandler.h"
#include "utils/ThreadPool.h"
#include <memory>

// Server related configuration
struct SslServerConfig : public SslConfig {
    int32_t listenPort;
    std::string listenIp;
    std::string caCertificateFile;
    std::string keyFile;
};

// This class will handle income SSL connections.
// Each SSL connection will be handled by a separate thread from start to end.
class SslServer : public SslHandler {
  public:
    SslServer(std::unique_ptr<SslServerConfig> config);
    virtual ~SslServer() = default;

    // Start's the server, initialize related variables and start the main server loop
    void Start();

    // Stop the server main loop and clear the related variables
    void Stop();

    SslServerConfig GetConfig() const { return *_config; }

  protected:
    // Configure the OpenSSL CTX object
    bool ConfigureSslContext(SSL_CTX_PTR ctx, const SslConfig& config) override;

    // Handle the retrieval of the SSL Certificate either from the cache directory or by generating
    // one on the fly from a newly established SSL connection
    static X509_OPTR FetchCertificate(const std::string& serverName, SSL_PTR ssl, int32_t port = 443);

    // A callback provided to OpenSSL which will be called once an SNI extention is being parsed
    static int32_t ServerNameCb(SSL_PTR ssl, int* ad, void* arg);

  private:
    // Create a TCP socket
    int32_t CreateSocket();

    // After connection is established with the client, first peek in the first message
    // to check if this is an HTTP CONNECT message, if so - handle it, otherwise - ignore it
    bool HandleHttpConnect(int32_t clientSocket, SSL_PTR ssl);

    std::unique_ptr<SslServerConfig> _config;
    std::unique_ptr<ThreadPool> _threadPool;
    std::atomic<bool> _running;
    SSL_CTX_OPTR _ctx;
    int32_t _socket;
};
