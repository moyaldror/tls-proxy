#pragma once

#include <string>

// SSL common config to both client and server
struct SslConfig {
    bool isServer;
    std::string cipherList;
};
