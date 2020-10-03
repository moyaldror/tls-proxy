#pragma once

#include "core/HandlerLayer.h"
#include <iostream>

// A Middleware layer used to convert http keep-alive sessions to
// no-keep-alive sessions.
// We use this layer as we want a simple proxy to support 1 REQUEST -> 1 RESPONSE
class HttpRewriteLayer : public HandlerLayer {
  public:
    HttpRewriteLayer(std::unique_ptr<HandlerLayer> next) : HandlerLayer(std::move(next)) {}

    explicit HttpRewriteLayer() : HandlerLayer(nullptr) {}
    ~HttpRewriteLayer() override = default;

    std::string GetName() const override { return "HttpRewriteLayer"; };

    // Implements the data processing function
    std::shared_ptr<Message> ProcessMessage(std::shared_ptr<Message> msg) override;
};
