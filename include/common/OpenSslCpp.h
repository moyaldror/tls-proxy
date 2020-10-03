#pragma once

#include <memory>

extern "C" {
using EVP_PKEY = struct evp_pkey_st;
using X509 = struct x509_st;
using BIO = struct bio_st;
using SSL = struct ssl_st;
using SSL_CTX = struct ssl_ctx_st;
};

// A Utility class to wrap OpenSSL pointer types
// for better memory managment and ease of use
template <class T, class D>
class OpensslWrapper {
    std::unique_ptr<T, D> _value;

  public:
    OpensslWrapper(T* value, D deleter) : _value(value, deleter) {}
    OpensslWrapper(std::unique_ptr<T, D> value) : _value(std::move(value)) {}
    OpensslWrapper(OpensslWrapper&& other) : _value(std::move(other._value)) {}
    ~OpensslWrapper() { _value.reset(); }

    OpensslWrapper& operator=(OpensslWrapper&& other) {
        if (*this != other) {
            _value.swap(other._value);
        }
        return *this;
    }
    OpensslWrapper& operator=(T* value) {
        if (_value.get() != value) {
            _value.reset(value);
        }
        return *this;
    }
    OpensslWrapper& operator=(std::unique_ptr<T, D> value) {
        if (_value != value) {
            _value.swap(value);
        }
        return *this;
    }

    bool operator==(const OpensslWrapper& y) { return _value == y._value; }
    bool operator!=(const OpensslWrapper& y) { return _value != y._value; }
    bool operator==(std::unique_ptr<T, D> y) { return _value == y; }
    bool operator!=(std::unique_ptr<T, D> y) { return _value != y; }
    bool operator==(const T* y) { return _value.get() == y; }
    bool operator!=(const T* y) { return _value.get() != y; }

    bool operator!() { return _value.get() == nullptr; }

    T* Get() { return _value.get(); }
    T* Pop() { return _value.release(); }
    operator T*() { return _value.get(); }
    operator const T*() { return _value.get(); }

    void Reset() { _value.reset(); }
};

// aliasing OpenSSL pointer types
using EVP_PKEY_PTR = EVP_PKEY*;
using X509_PTR = X509*;
using BIO_PTR = BIO*;
using SSL_PTR = SSL*;
using SSL_CTX_PTR = SSL_CTX*;

// aliasing OpenSSL free() function
using EVP_PKEY_deleter_t = void (*)(EVP_PKEY_PTR);
using X509_deleter_t = void (*)(X509_PTR);
using BIO_deleter_t = int (*)(BIO_PTR);
using SSL_deleter_t = void (*)(SSL_PTR);
using SSL_CTX_deleter_t = void (*)(SSL_CTX_PTR);

// aliasing OpenSSL utility class with the corret free() functions
using EVP_PKEY_OPTR = OpensslWrapper<EVP_PKEY, EVP_PKEY_deleter_t>;
using X509_OPTR = OpensslWrapper<X509, X509_deleter_t>;
using BIO_OPTR = OpensslWrapper<BIO, BIO_deleter_t>;
using SSL_OPTR = OpensslWrapper<SSL, SSL_deleter_t>;
using SSL_CTX_OPTR = OpensslWrapper<SSL_CTX, SSL_CTX_deleter_t>;

// macros to ease the creation of new OpenSSL objects
#define DEF_VARIABLE(var_type, name, init_value) var_type##_OPTR name{(init_value), var_type##_free};
#define DEF_EVP_PKEY(var_name, var_value) DEF_VARIABLE(EVP_PKEY, var_name, (var_value))
#define DEF_X509(var_name, var_value) DEF_VARIABLE(X509, var_name, (var_value))
#define DEF_BIO(var_name, var_value) DEF_VARIABLE(BIO, var_name, (var_value))
#define DEF_SSL(var_name, var_value) DEF_VARIABLE(SSL, var_name, (var_value))
#define DEF_SSL_CTX(var_name, var_value) DEF_VARIABLE(SSL_CTX, var_name, (var_value))
