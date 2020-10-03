#pragma once

#include "common/OpenSslCpp.h"
#include "core/HandlerLayer.h"
#include "core/SslHandlerLayer.h"

// A Middleware layer to handle the backend ssl connection.
// This layer will be resposible to connect to the real HTTPS server,
// to fetch the server certificate and send the data from the client to the
// HTTPS server.
class BackendSslLayer : public HandlerLayer, public SslHandlerLayer {
  public:
    BackendSslLayer(std::unique_ptr<HandlerLayer> next, SSL_OPTR ssl)
        : HandlerLayer(std::move(next)), SslHandlerLayer(std::move(ssl)) {
        SSL_set_connect_state(_ssl);
    }

    explicit BackendSslLayer(SSL_OPTR ssl) : BackendSslLayer(nullptr, std::move(ssl)) {}
    ~BackendSslLayer() override = default;

    std::string GetName() const override { return "BackendSslLayer"; };

    // Will return the HTTPS server certificate
    X509_PTR GetCertificate();

    // Will connect to the HTTPS server
    bool Connect();

    // Implements the data processing function
    std::shared_ptr<Message> ProcessMessage(std::shared_ptr<Message> msg) override;
};