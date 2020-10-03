#include <chrono>
#include <iomanip>
#include <iostream>
#include <regex>

#include "http/HttpMessage.h"
#include "middleware/LogHttpLayer.h"

void LogHttpLayer::LogHttpMessage(const HttpMessage& httpMessage) {
    std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::stringstream ss;

    ss << "[HTTP ] [" << std::put_time(std::localtime(&now), "%T %F") << "] " << std::endl;
    ss << "\t" << (httpMessage.IsRequest() ? "Send request (" : "Got response (")
       << (httpMessage.IsRequest() ? httpMessage.HttpMethodToString(httpMessage.Method()) : httpMessage.Status())
       << ") " << (httpMessage.IsRequest() ? "to " : "from ") << _remoteHost << " on port " << _remotePort << std::endl;

    ss << "\tHeaders:" << std::endl;
    for (const auto& header : httpMessage.Headers()) {
        ss << "\t  " << header.first << ": " << header.second << std::endl;
    }

    ss << "\tData size: " << httpMessage.Data().size() << std::endl;

    std::cout << ss.str();
}

std::shared_ptr<Message> LogHttpLayer::ProcessMessage(std::shared_ptr<Message> msg) {
    std::shared_ptr<Message> res = nullptr;
    auto data = std::dynamic_pointer_cast<StringDataMessage>(msg);

    if (data == nullptr) {
        res = std::make_shared<ErrorMessage>("Message should be of type StringData");
    } else {
        HttpMessage request(data->Data);
        _remoteHost = request.Host();
        _remotePort = request.Port();

        LogHttpMessage(request);
        res = PushToNext(data);

        data = std::dynamic_pointer_cast<StringDataMessage>(res);
        if (data == nullptr) {
            res = std::make_shared<ErrorMessage>("Message should be of type StringData");
        } else {
            HttpMessage response(data->Data);
            LogHttpMessage(response);
        }
    }

    return res;
}
