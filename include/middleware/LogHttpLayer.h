#pragma once

#include "core/HandlerLayer.h"

class HttpMessage;

// A Middleware layer used to log HTTP messages.
class LogHttpLayer : public HandlerLayer {
  public:
    LogHttpLayer(std::unique_ptr<HandlerLayer> next) : HandlerLayer(std::move(next)), _remoteHost("") {}

    explicit LogHttpLayer() : HandlerLayer(nullptr) {}
    ~LogHttpLayer() override = default;

    std::string GetName() const override { return "LogHttpLayer"; };

    // Implements the data processing function
    std::shared_ptr<Message> ProcessMessage(std::shared_ptr<Message> msg) override;

    // Will log the HTTP message
    void LogHttpMessage(const HttpMessage& httpMessage);

  private:
    std::string _remoteHost;
    uint32_t _remotePort;
};
