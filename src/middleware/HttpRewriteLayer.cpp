#include <iostream>
#include <regex>

#include "http/HttpMessageBuilder.h"
#include "middleware/HttpRewriteLayer.h"
#include <stdexcept>

std::string ForceConnectionClose(const std::string& httpMessage) {
    HttpMessageBuilder msg(httpMessage);

    msg.Headers()["connection"] = "keep-alive";

    return msg.Build().ToString();
}

std::shared_ptr<Message> HttpRewriteLayer::ProcessMessage(std::shared_ptr<Message> msg) {
    std::shared_ptr<Message> res = nullptr;
    auto data = std::dynamic_pointer_cast<StringDataMessage>(msg);

    if (data == nullptr) {
        res = std::make_shared<ErrorMessage>("Message should be of type StringData");
    } else {
        try {
            res = PushToNext(std::make_shared<StringDataMessage>(ForceConnectionClose(data->Data)));

            data = std::dynamic_pointer_cast<StringDataMessage>(res);
            if (data == nullptr) {
                res = std::make_shared<ErrorMessage>("Message should be of type StringData");
            } else {
                res = std::make_shared<StringDataMessage>(ForceConnectionClose(data->Data));
            }
        } catch (std::invalid_argument* e) {
            res = std::make_shared<ErrorMessage>(e->what());
            LOG_ERROR(e->what());
        }
    }

    return res;
}