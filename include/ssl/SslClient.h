#pragma once

#include "common/OpenSslCpp.h"
#include "ssl/SslConfig.h"
#include "ssl/SslHandler.h"
#include "utils/ThreadPool.h"
#include <memory>

// Client related configuration
struct SslClientConfig : public SslConfig {
    int32_t serverPort;
    std::string serverIp;
    std::string localIp;
};

class BackendSslLayer;

// This class will create the BackendSslLayer.
// It is used to ease the creation and configuration of the TCP socket,
// connecting it with the SSL socket and configuring it.
class SslClient : public SslHandler {
  public:
    SslClient(std::unique_ptr<SslClientConfig> config);
    virtual ~SslClient() = default;

    // Create connection with the HTTPS server and return the BackendSslLayer that handles this connection
    std::unique_ptr<BackendSslLayer> Connect();

  protected:
    // Configure the OpenSSL CTX object
    bool ConfigureSslContext(SSL_CTX_PTR ctx, const SslConfig& config) override;

  private:
    // Create a TCP socket
    int32_t CreateSocket();

    std::unique_ptr<SslClientConfig> _config;
    SSL_CTX_OPTR _ctx;
    int32_t _socket;
};
