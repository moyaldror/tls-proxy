#include <iostream>
#include <memory>
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/x509v3.h>

#include "utils/CertOps.h"

using namespace std;

namespace x509 {

EVP_PKEY_OPTR LoadPemKey(const string& file, const string& pass) {
    DEF_EVP_PKEY(pkey, nullptr);
    DEF_BIO(inputFile, BIO_new(BIO_s_file()));

    if ((BIO_read_filename(inputFile, file.c_str()) <= 0) ||
        ((pkey = PEM_read_bio_PrivateKey(inputFile, nullptr, nullptr, nullptr)) == nullptr)) {
        // will return null unique_ptr
    }

    return pkey;
}

X509_OPTR LoadPemCert(const string& file) {
    DEF_X509(certificate, nullptr);
    DEF_BIO(inputFile, BIO_new(BIO_s_file()));

    if ((BIO_read_filename(inputFile, file.c_str()) <= 0) ||
        ((certificate = PEM_read_bio_X509_AUX(inputFile, nullptr, nullptr, nullptr)) == nullptr)) {
        // will return null unique_ptr
    }

    return certificate;
}

int SaveCertificateToPemFile(X509_PTR certificate, const string& certFile) {
    FileHandle file{certFile, "w+"};
    DEF_BIO(outputFile, BIO_new_fp(file, BIO_NOCLOSE));

    int res = PEM_write_bio_X509(outputFile, certificate);

    return res;
}

int SaveKeyToPemFile(EVP_PKEY_PTR key, const string& keyFile) {
    FileHandle file{keyFile, "w+"};
    DEF_BIO(outputFile, BIO_new_fp(file, BIO_NOCLOSE));

    int res = PEM_write_bio_PrivateKey(outputFile, key, EVP_aes_256_cbc_hmac_sha256(), nullptr, 0, nullptr, nullptr);

    return res;
}

X509_OPTR ClonePemCert(X509_OPTR cert, const string& caFile, const string& keyFile) {
    DEF_X509(caCert, LoadPemCert(caFile).Pop());
    DEF_X509(newCert, X509_new());
    DEF_EVP_PKEY(privateKey, LoadPemKey(keyFile, "").Pop());
    DEF_EVP_PKEY(caPrivateKey, LoadPemKey(caFile, "").Pop());

    X509_EXTENSION* ext = nullptr;
    int extIndex = -1;

    // Copy only relevant fields, not all of them
    // Copy all fields may cause issues - an example - copy AuthorityInformationAccess extension will lead for browser
    // to try and verify our generated certificate which will cause connection drop
    if (!X509_set_pubkey(newCert, privateKey) || !X509_set_serialNumber(newCert, X509_get_serialNumber(cert)) ||
        !X509_set_version(newCert, X509_get_version(cert)) ||
        !X509_set_issuer_name(newCert, X509_get_subject_name(caCert)) ||
        !X509_set_subject_name(newCert, X509_get_subject_name(cert)) ||
        !X509_set1_notBefore(newCert, X509_get_notBefore(cert)) ||
        !X509_set1_notAfter(newCert, X509_get_notAfter(cert)) ||
        !(((extIndex = X509_get_ext_by_NID(cert, NID_subject_alt_name, -1)) >= 0 &&
           (ext = X509_get_ext(cert, extIndex)) != nullptr && X509_add_ext(newCert, ext, -1)) ||
          (extIndex < 0)) ||
        !X509_sign(newCert, caPrivateKey, EVP_sha512()) || X509_check_issued(caCert, newCert) != X509_V_OK) {
        cerr << "Failed to generate new certificate" << endl;
        newCert = nullptr;
    }

    return newCert;
}

X509_OPTR ClonePemCert(const string& certFile, const string& caFile, const string& keyFile) {
    DEF_X509(cert, LoadPemCert(certFile).Pop());

    return ClonePemCert(std::move(cert), caFile, keyFile);
}

}; // namespace x509