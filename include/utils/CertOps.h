#pragma once

#include "common/OpenSslCpp.h"
#include <openssl/evp.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <string>

namespace x509 {

// RAII structure to avoid leaking resources
struct FileHandle {
    FILE* _file;

    explicit FileHandle(const std::string& file, const std::string& permissions) {
        _file = fopen(file.c_str(), permissions.c_str());
    }

    ~FileHandle() {
        if (_file != nullptr) {
            fclose(_file);
        }
    }

    operator FILE*() { return _file; }
};

// Load a key from a PEM file
EVP_PKEY_OPTR LoadPemKey(const std::string& file, const std::string& pass);

// Load a X509 certificate from a PEM file
X509_OPTR LoadPemCert(const std::string& file);

// Clone a X509 certificate, the new certificate will be signed ny the CA certificate loaded
// from caFile and will use the key loaded from KeyFile
X509_OPTR ClonePemCert(X509_OPTR cert, const std::string& caFile, const std::string& keyFile);

// Clone a X509 certificate, the new certificate will be signed ny the CA certificate loaded
// from caFile and will use the key loaded from KeyFile
X509_OPTR ClonePemCert(const std::string& certFile, const std::string& caFile, const std::string& keyFile);

// Save a certificate to PEM file
int SaveCertificateToPemFile(X509_PTR certificate, const std::string& certFile);

// Save a key to PEM file
int SaveKeyToPemFile(EVP_PKEY_PTR key, const std::string& keyFile);
}; // namespace x509
