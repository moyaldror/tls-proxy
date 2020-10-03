#pragma once

#include "core/Messages.h"
#include "utils/Logger.h"
#include <memory>

// This is the base class for all middleware layers.
// It's called a Handler Layer because each middleware layer should handle the data
// passing through it.
class HandlerLayer {
  public:
    HandlerLayer(std::unique_ptr<HandlerLayer> next) : _next(std::move(next)) {}
    HandlerLayer() : HandlerLayer(nullptr) {}
    virtual ~HandlerLayer() = default;

    virtual std::string GetName() const = 0;
    inline void SetNext(std::unique_ptr<HandlerLayer> next);

    // Should be implamented by each middleware class.
    // This function will be called each time a layer will push data down to the next
    // middleware layer.
    virtual std::shared_ptr<Message> ProcessMessage(std::shared_ptr<Message> msg) = 0;
    inline std::shared_ptr<Message> PushToNext(std::shared_ptr<Message> msg);

  private:
    std::unique_ptr<HandlerLayer> _next;
};

std::shared_ptr<Message> HandlerLayer::PushToNext(std::shared_ptr<Message> msg) {
    if (_next != nullptr) {
        return _next->ProcessMessage(msg);
    } else {
        return std::make_shared<ErrorMessage>("Next layer is null");
    }
}

void HandlerLayer::SetNext(std::unique_ptr<HandlerLayer> next) { _next = std::move(next); }
