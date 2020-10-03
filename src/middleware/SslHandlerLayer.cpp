#include <fcntl.h>
#include <iostream>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <sys/select.h>
#include <unistd.h>

#include "common/OpenSslCpp.h"
#include "core/SslHandlerLayer.h"
#include "utils/Logger.h"

constexpr int32_t BUFFER_SIZE = 1024000;

bool SslHandlerLayer::HandleSslError(int32_t ret) {
    bool stopOp = false;
    int error = SSL_get_error(_ssl, ret);
    char* errBioBuff;
    long errStrLen = 0;
    DEF_BIO(errBio, BIO_new(BIO_s_mem()));

    LOG_TRACE("Handeling error " << error);

    switch (error) {
    case SSL_ERROR_NONE:
    case SSL_ERROR_WANT_READ:
    case SSL_ERROR_WANT_WRITE:
        break;
    case SSL_ERROR_ZERO_RETURN:
        if (ret == -1) {
            break;
        }

        stopOp = true;
        ERR_print_errors(errBio);
        DoClose(true);
        break;
    case SSL_ERROR_SYSCALL:
    case SSL_ERROR_SSL:
        stopOp = true;
        ERR_print_errors(errBio);
        DoClose(false);
        break;
    }

    if ((errStrLen = BIO_get_mem_data(errBio, &errBioBuff)) > 0) {
        LOG_ERROR(errBioBuff);
    }

    return stopOp;
}

std::string SslHandlerLayer::DoSslRead() {
    if (_ssl == nullptr) {
        return "";
    }

    ToggleBlockingMode();

    char buffer[BUFFER_SIZE] = {0};
    int bufferIndex = 0;
    int32_t bytes = 0;
    fd_set rfds;
    struct timeval tv;
    int retval;

    tv.tv_sec = 1;
    tv.tv_usec = 0;

    for (;;) {
        FD_ZERO(&rfds);
        FD_SET(_socket, &rfds);

        retval = select(_socket + 1, &rfds, nullptr, nullptr, &tv);

        if (retval == -1) {
            LOG_ERROR("Error in select");
        } else if (retval) {
            bytes = SSL_read(_ssl, buffer + bufferIndex, BUFFER_SIZE - 1);

            if (bytes > 0) {
                bufferIndex += bytes;
            } else {
                if (HandleSslError(bytes)) {
                    break;
                }
            }
        } else {
            break;
        }
    }

    ToggleBlockingMode();
    return std::string(buffer, bufferIndex); // in order to support null characters use the explicit ctor
}

int32_t SslHandlerLayer::DoSslWrite(const std::string& data) {
    if (_ssl == nullptr) {
        return 0;
    }

    int32_t bytes = SSL_write(_ssl, data.c_str(), data.size());

    if (bytes <= 0) {
        HandleSslError(bytes);
    }

    return bytes;
}

bool SslHandlerLayer::DoSslConnectAccept() {
    int32_t res = 0;

    if (_ssl == nullptr) {
        return false;
    }

    if (SSL_is_server(_ssl)) {
        res = SSL_accept(_ssl);
    } else {
        res = SSL_connect(_ssl);
    }

    LOG_TRACE("Ssl connected");
    if (res <= 0) {
        HandleSslError(res);
    }

    return res > 0;
}

void ShutdownSsl(SSL_PTR ssl) {
    int32_t ret = 0;

    do {
        ret = SSL_shutdown(ssl);
        if (ret < 0) {
            switch (SSL_get_error(ssl, ret)) {
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE:
            case SSL_ERROR_WANT_ASYNC:
            case SSL_ERROR_WANT_ASYNC_JOB:
                continue;
            }
            ret = 0;
        }
    } while (ret < 0);
}

void SslHandlerLayer::DoClose(bool shutdown) {
    if (!_connectionWasClosed && _ssl != nullptr) {
        _connectionWasClosed = true;
        if (shutdown) {
            ShutdownSsl(_ssl);
        }
        close(_socket);

        _ssl.Reset();
        _socket = -1;
    }
}

void SslHandlerLayer::ToggleBlockingMode() {
    int32_t flags = fcntl(_socket, F_GETFL, 0);

    if (flags >= 0) {
        flags = (flags & O_NONBLOCK) ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
        fcntl(_socket, F_SETFL, flags);
    }
}