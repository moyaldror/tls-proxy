#pragma once

#include "common/OpenSslCpp.h"
#include "core/HandlerLayer.h"
#include "core/SslHandlerLayer.h"

// A Middleware layer to handle the frontend ssl connection.
// This layer will be resposible handle the connection with the client (probably a web-browser)
// This layer will be used to generate the dynamic certificate to mimic the real server certificate
// using 1 of the following ways:
// 1. HTTP Connect:
//     When a client use this proxy server as a real proxy server, it will first send an HTTP CONNECT request
//     with the details of the server it wishes to connect to.
// 2. SNI (Server Name Indication) Extension:
//     When using this proxy in transparent mode (the client is not aware of it), it will send a request with
//     SNI extension indicating the name of the server he wishes to connect to.
// 3. When a request will arrive with no HTTP CONNECT an no SNI -> the connection will be dropped.
class FrontendSslLayer : public HandlerLayer, public SslHandlerLayer {
  public:
    FrontendSslLayer(std::unique_ptr<HandlerLayer> next, SSL_OPTR ssl)
        : HandlerLayer(std::move(next)), SslHandlerLayer(std::move(ssl)) {
        SSL_set_accept_state(_ssl);
    }

    explicit FrontendSslLayer(SSL_OPTR ssl) : FrontendSslLayer(nullptr, std::move(ssl)) {}
    ~FrontendSslLayer() override = default;

    std::string GetName() const override { return "FrontendSslLayer"; };

    // Implements the data processing function
    std::shared_ptr<Message> ProcessMessage(std::shared_ptr<Message> msg) override;
};