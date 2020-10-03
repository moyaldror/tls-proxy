#pragma once

#include <string>
#include <unordered_map>
#include <vector>

// A class used to parse and represent a HTTP message.
class HttpMessage {
    // Forward decleration
    friend class HttpMessageBuilder;

  public:
    enum class HttpMethod : uint8_t { NONE, GET, HEAD, POST, PUT, DELETE, CONNECT, OPTIONS, TRACE, PATCH };

    enum class HttpVersion : uint8_t { UNKNOWN, V1_0, V1_1 };

    enum class HttpStatusCodeGroup : uint8_t { NONE, S1XX, S2XX, S3XX, S4XX, S5XX };

    enum class HttpParsingParts : uint8_t { START_LINE, HEADERS, DATA };

    explicit HttpMessage(const std::string& message);

    // Getters
    inline const std::string& OriginalMessage() const { return _originalMessage; }
    inline bool IsRequest() const { return _isRequest; }
    inline HttpMethod Method() const { return _method; }
    inline const std::string& Host() const { return _host; }
    inline const std::string& Path() const { return _path; }
    inline uint32_t Port() const { return _port; }
    inline const std::unordered_map<std::string, std::string>& Headers() const { return _headers; }
    inline HttpVersion Version() const { return _version; }
    inline HttpStatusCodeGroup StatusCodeGroup() const { return _statusCodeGroup; }
    inline const std::string& Status() const { return _status; }
    inline const std::string& Data() const { return _data; }

    // Utility functions to convers enums to string representation
    const std::string& HttpVersionToString(HttpVersion version) const;
    const std::string& HttpStatusCodeGroupToString(HttpStatusCodeGroup status) const;
    const std::string& HttpMethodToString(HttpMethod method) const;

    std::string ToString() const;

  private:
    bool ParseMessage();
    bool Validate() const;
    bool ParseStartLine(const std::string& s);
    bool AddNewHeader(const std::string& s);
    bool AddData(const std::string& s);

    std::string CommonHttpMessageToString() const;
    std::string RequestToString() const;
    std::string ResponseToString() const;

    std::string _originalMessage;
    bool _isRequest;
    HttpMethod _method;
    std::string _host;
    std::string _path;
    uint32_t _port;
    std::unordered_map<std::string, std::string> _headers;
    HttpVersion _version;
    HttpStatusCodeGroup _statusCodeGroup;
    std::string _status;
    std::string _data;
};
