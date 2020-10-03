#include "common/OpenSslCpp.h"
#include "ssl/SslServer.h"
#include "utils/Logger.h"

#include <csignal>
#include <cstdlib>
#include <getopt.h>
#include <iostream>
#include <openssl/evp.h>
#include <unistd.h>
#include <vector>

using namespace std;

static unique_ptr<SslServer> server = nullptr;

// Handle clean exit when pressing CTRL+C
void SignalCallbackHandler(int signum);

// Read CLI arguments and generate server configuration from it
unique_ptr<SslServerConfig> GetServerConfig(int argc, char* const argv[]);

int main(int argc, char* const argv[]) {
    signal(SIGINT, SignalCallbackHandler);

    server = make_unique<SslServer>(std::move(GetServerConfig(argc, argv)));
    server->Start();

    return 0;
}

void SignalCallbackHandler(int signum) {
    LOG_INFO("Stopping server...");

    if (server != nullptr) {
        server->Stop();
        server.reset();
    }

    EVP_cleanup();

    exit(signum);
}

unique_ptr<SslServerConfig> GetServerConfig(int argc, char* const argv[]) {
    int res = 0;
    int optionIndex = 0;

    auto conf = make_unique<SslServerConfig>();
    conf->isServer = true;
    conf->cipherList = "ALL";
    conf->listenPort = 5443;
    conf->listenIp = "192.168.244.1";
    conf->caCertificateFile = "./scerts/ca_cert.pem";
    conf->keyFile = "./scerts/key.pem";

    static struct option longOptions[] = {
        {"ciphersuites", required_argument, nullptr, 0}, {"port", required_argument, nullptr, 0},
        {"ip", required_argument, nullptr, 0},           {"ca", required_argument, nullptr, 0},
        {"key", required_argument, nullptr, 0},          {nullptr, 0, nullptr, 0}};

    while ((res = getopt_long(argc, argv, "", longOptions, &optionIndex)) != -1) {
        switch (res) {
        case 0:
            switch (optionIndex) {
            case 0:
                conf->cipherList = std::string(optarg);
                break;
            case 1:
                conf->listenPort = std::stoi(optarg);
                break;
            case 2:
                conf->listenIp = std::string(optarg);
                break;
            case 3:
                conf->caCertificateFile = std::string(optarg);
                break;
            case 4:
                conf->keyFile = std::string(optarg);
                break;
            }
            break;
        default:
            // do nothing
            break;
        }
    }

    return conf;
}