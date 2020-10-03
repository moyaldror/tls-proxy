#pragma once

#include "common/OpenSslCpp.h"

struct SslConfig;

// This is a base class used by both SslClient and SslServer
// It handles the configration of OpenSSL CTX object
class SslHandler {
  public:
    virtual ~SslHandler() = default;

  protected:
    SSL_CTX_OPTR CreateSslContext(const SslConfig& config);
    virtual bool ConfigureSslContext(SSL_CTX_PTR ctx, const SslConfig& config) = 0;
};
