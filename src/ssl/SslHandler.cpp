#include <openssl/err.h>
#include <openssl/ssl.h>

#include "ssl/SslConfig.h"
#include "ssl/SslHandler.h"
#include "utils/Logger.h"

SSL_CTX_OPTR SslHandler::CreateSslContext(const SslConfig& config) {
    const SSL_METHOD* method = config.isServer ? SSLv23_server_method() : SSLv23_client_method();

    DEF_SSL_CTX(ctx, SSL_CTX_new(method));

    if (!ctx) {
        LOG_ERROR("Unable to create SSL context");
        ERR_print_errors_fp(stderr);
    } else {
        ConfigureSslContext(ctx.Get(), config);
    }

    return ctx;
}
