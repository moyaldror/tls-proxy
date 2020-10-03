#pragma once

#include <openssl/ssl.h>
#include <string>

#include "common/OpenSslCpp.h"

// This is a base class for all ssl middleware layers.
// It will ease the use of OpenSSL read/write/connect/accept/close APIs, will handle errors
// and socket connection type (blocking/non-blocking)
class SslHandlerLayer {
  public:
    SslHandlerLayer(SSL_OPTR ssl) : _ssl(std::move(ssl)), _socket(SSL_get_fd(_ssl)), _connectionWasClosed(false) {}
    virtual ~SslHandlerLayer() { DoClose(true); };

    // Read from an SSL Socket and return the data as string
    std::string DoSslRead();

    // Write data to an SSL Socket and return number of bytes written
    int32_t DoSslWrite(const std::string& data);

    // Perform connect/accept depending on the SSL Socket type
    // Client will perform connect, Server will perform accept
    bool DoSslConnectAccept();

    // Close SSL Socket
    void DoClose(bool shutdown);

  protected:
    SSL_OPTR _ssl;
    int32_t _socket;
    bool _connectionWasClosed;

  private:
    bool HandleSslError(int32_t ret);
    void ToggleBlockingMode();
};