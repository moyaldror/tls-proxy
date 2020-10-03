#pragma once

#include "http/HttpMessage.h"
#include <string>
#include <unordered_map>
#include <vector>

// A class used to build a HTTP message.
class HttpMessageBuilder {
  public:
    // C'tors
    HttpMessageBuilder(bool isRequest);
    explicit HttpMessageBuilder(HttpMessage message);
    explicit HttpMessageBuilder(const std::string& message);

    // Access to HTTP members
    inline bool& IsRequest() { return _msg._isRequest; }
    inline HttpMessage::HttpMethod& Method() { return _msg._method; }
    inline std::string& Host() { return _msg._host; }
    inline std::string& Path() { return _msg._path; }
    inline uint32_t& Port() { return _msg._port; }
    inline std::unordered_map<std::string, std::string>& Headers() { return _msg._headers; }
    inline HttpMessage::HttpVersion& Version() { return _msg._version; }
    inline HttpMessage::HttpStatusCodeGroup& StatusCodeGroup() { return _msg._statusCodeGroup; }
    inline std::string& Status() { return _msg._status; }
    inline std::string& Data() { return _msg._data; }

    // Build a HttpMessage
    HttpMessage Build();

  private:
    HttpMessage _msg;
};