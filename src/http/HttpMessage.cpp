#include "http/HttpMessage.h"
#include "utils/Logger.h"
#include <regex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace std;

static const string NEW_LINE = "\r\n";

string Ltrim(const string& s) {
    static regex re = regex(R"(^\s+)");
    return regex_replace(s, re, string(""));
}

string Rtrim(const string& s) {
    static regex re = regex(R"(\s+$)");
    return regex_replace(s, re, string(""));
}

string Trim(const string& s) { return Ltrim(Rtrim(s)); }

string ToLower(const string& s) {
    string lower;
    lower.resize(s.size());
    std::transform(s.begin(), s.end(), lower.begin(), ::tolower);
    return lower;
}

const string& HttpMessage::HttpMethodToString(HttpMessage::HttpMethod method) const {
    static unordered_map<HttpMessage::HttpMethod, string> methodToStringDict{
        {HttpMessage::HttpMethod::NONE, "NONE"},       {HttpMessage::HttpMethod::GET, "GET"},
        {HttpMessage::HttpMethod::HEAD, "HEAD"},       {HttpMessage::HttpMethod::POST, "POST"},
        {HttpMessage::HttpMethod::PUT, "PUT"},         {HttpMessage::HttpMethod::DELETE, "DELETE"},
        {HttpMessage::HttpMethod::CONNECT, "CONNECT"}, {HttpMessage::HttpMethod::OPTIONS, "OPTIONS"},
        {HttpMessage::HttpMethod::TRACE, "TRACE"},     {HttpMessage::HttpMethod::PATCH, "PATCH"},
    };

    return methodToStringDict[method];
}

HttpMessage::HttpMethod StringToHttpMethod(const string& method) {
    static unordered_map<string, HttpMessage::HttpMethod> stringToMethodDict{
        {"NONE", HttpMessage::HttpMethod::NONE},       {"GET", HttpMessage::HttpMethod::GET},
        {"HEAD", HttpMessage::HttpMethod::HEAD},       {"POST", HttpMessage::HttpMethod::POST},
        {"PUT", HttpMessage::HttpMethod::PUT},         {"DELETE", HttpMessage::HttpMethod::DELETE},
        {"CONNECT", HttpMessage::HttpMethod::CONNECT}, {"OPTIONS", HttpMessage::HttpMethod::OPTIONS},
        {"TRACE", HttpMessage::HttpMethod::TRACE},     {"PATCH", HttpMessage::HttpMethod::PATCH},
    };

    return stringToMethodDict[method];
}

const string& HttpMessage::HttpStatusCodeGroupToString(HttpMessage::HttpStatusCodeGroup status) const {
    static unordered_map<HttpMessage::HttpStatusCodeGroup, string> statusToStringDict{
        {HttpMessage::HttpStatusCodeGroup::NONE, "NONE"}, {HttpMessage::HttpStatusCodeGroup::S1XX, "1XX"},
        {HttpMessage::HttpStatusCodeGroup::S2XX, "2XX"},  {HttpMessage::HttpStatusCodeGroup::S3XX, "3XX"},
        {HttpMessage::HttpStatusCodeGroup::S4XX, "4XX"},  {HttpMessage::HttpStatusCodeGroup::S5XX, "5XX"},
    };

    return statusToStringDict[status];
}

HttpMessage::HttpStatusCodeGroup StringToHttpStatusCodeGroup(const string& status) {
    static unordered_map<string, HttpMessage::HttpStatusCodeGroup> stringToStatusDict{
        {"NONE", HttpMessage::HttpStatusCodeGroup::NONE},     {R"(1\d\d)", HttpMessage::HttpStatusCodeGroup::S1XX},
        {R"(2\d\d)", HttpMessage::HttpStatusCodeGroup::S2XX}, {R"(3\d\d)", HttpMessage::HttpStatusCodeGroup::S3XX},
        {R"(4\d\d)", HttpMessage::HttpStatusCodeGroup::S4XX}, {R"(5\d\d)", HttpMessage::HttpStatusCodeGroup::S5XX},
    };

    HttpMessage::HttpStatusCodeGroup result = HttpMessage::HttpStatusCodeGroup::NONE;
    smatch res;
    for (const auto& kv : stringToStatusDict) {
        if (regex_search(status, res, regex(kv.first))) {
            result = kv.second;
        }
    }

    return result;
}

const string& HttpMessage::HttpVersionToString(HttpMessage::HttpVersion version) const {
    static unordered_map<HttpMessage::HttpVersion, string> versionToStringDict{
        {HttpMessage::HttpVersion::UNKNOWN, "NONE"},
        {HttpMessage::HttpVersion::V1_0, "HTTP/1.0"},
        {HttpMessage::HttpVersion::V1_1, "HTTP/1.1"},
    };

    return versionToStringDict[version];
}

HttpMessage::HttpVersion StringToHttpVersion(const string& version) {
    static unordered_map<string, HttpMessage::HttpVersion> stringToVersionDict{
        {"NONE", HttpMessage::HttpVersion::UNKNOWN},
        {"HTTP/1.0", HttpMessage::HttpVersion::V1_0},
        {"HTTP/1.1", HttpMessage::HttpVersion::V1_1},
    };

    return stringToVersionDict[version];
}

bool HttpMessage::ParseStartLine(const string& s) {
    vector<string> startLineParts;
    string item;
    stringstream ss(s);

    // Start line is the first line of the HTTP message
    // In request it looks like this:
    //     METHOD PATH HTTP_VERSION
    // In response:
    //     HTTP_VERSION RESPONSE_CODE RESPONSE_DESCRIPTION
    while (getline(ss, item, ' ')) {
        if (item.length() > 0) {
            startLineParts.emplace_back(item);
        }
    }

    if (startLineParts[0].find("HTTP") == 0) {
        _isRequest = false;
        _version = StringToHttpVersion(startLineParts[0]);
        _statusCodeGroup = StringToHttpStatusCodeGroup(startLineParts[1]);

        ss.str("");
        ss.clear();

        for (size_t i = 1; i < startLineParts.size(); i++) {
            ss << startLineParts[i] << " ";
        }

        _status = Trim(ss.str());
    } else {
        _isRequest = true;
        _method = StringToHttpMethod(startLineParts[0]);
        _path = startLineParts[1];
        _version = StringToHttpVersion(startLineParts[2]);
    }

    return true;
}

bool HttpMessage::AddNewHeader(const string& s) {
    string key, value;
    size_t delimLocation = s.find_first_of(":");
    key = ToLower(Trim(string(s, 0, delimLocation)));
    value = Trim(string(s, delimLocation + 1, s.size() - delimLocation - 1));

    _headers.emplace(key, value);

    if (key == "host") {
        string host = value, port = "443";
        delimLocation = value.find(":");
        if (delimLocation != string::npos) {
            host = string(value, 0, delimLocation);
            port = string(value, delimLocation + 1, value.size() - delimLocation - 1);
        }

        _host = host;
        try {
            _port = stoi(port);
        } catch (std::invalid_argument e) {
            throw new invalid_argument(e.what());
        }
    }

    return true;
}

bool HttpMessage::AddData(const string& s) {
    _data += s;
    return true;
}

bool HttpMessage::ParseMessage() {
    bool parsingResult = true, parsingDone = false;
    string data = _originalMessage;
    static regex newLine(R"(.*\r\n)");
    smatch res;
    HttpMessage::HttpParsingParts parsingPart = HttpMessage::HttpParsingParts::START_LINE;

    while (parsingResult && !parsingDone && regex_search(data, res, newLine)) {
        string line = Trim(res.str());

        if (!line.empty()) {
            switch (parsingPart) {
            case HttpMessage::HttpParsingParts::START_LINE:
                parsingResult = ParseStartLine(line);
                parsingPart = HttpMessage::HttpParsingParts::HEADERS;
                break;
            case HttpMessage::HttpParsingParts::HEADERS:
                parsingResult = AddNewHeader(line);
                break;
            case HttpMessage::HttpParsingParts::DATA:
            default:
                parsingResult = false;
                break;
            }
        } else {
            switch (parsingPart) {
                // Empty line after the start line means a parsing error
            case HttpMessage::HttpParsingParts::START_LINE:
                parsingResult = false;
                break;
                // Empty line after the headers means end of the headers
            case HttpMessage::HttpParsingParts::HEADERS:
                parsingPart = HttpMessage::HttpParsingParts::DATA;
                parsingDone = true;
                break;
            case HttpMessage::HttpParsingParts::DATA:
            default:
                parsingResult = false;
                break;
            }
        }

        data = res.suffix();
    }

    if (parsingResult && parsingPart == HttpMessage::HttpParsingParts::DATA && !data.empty()) {
        parsingResult = AddData(data);
    }

    return parsingResult;
}

bool HttpMessage::Validate() const {
    bool isValid = _isRequest ? (_method != HttpMessage::HttpMethod::NONE && !_path.empty())
                              : (_statusCodeGroup != HttpMessage::HttpStatusCodeGroup::NONE);

    isValid = isValid && !_headers.empty() && _version != HttpMessage::HttpVersion::UNKNOWN;

    return isValid;
}

HttpMessage::HttpMessage(const string& message)
    : _originalMessage(std::move(message)), _isRequest(false), _method(HttpMessage::HttpMethod::NONE), _host(""),
      _path(""), _port(443), _headers({}), _version(HttpMessage::HttpVersion::UNKNOWN),
      _statusCodeGroup(HttpMessage::HttpStatusCodeGroup::NONE), _status(""), _data("") {
    bool res = ParseMessage();

    if (!res) {
        LOG_TRACE("Failed to parse HTTP message");
        throw new invalid_argument("Failed to parse HTTP message");
    }

    res = Validate();

    if (!res) {
        LOG_TRACE("Invalid HTTP message");
        throw new invalid_argument("Invalid HTTP message");
    }
}

std::string HttpMessage::CommonHttpMessageToString() const {
    stringstream ss;

    for (const auto& kv : _headers) {
        ss << kv.first << ": " << kv.second << NEW_LINE;
    }

    ss << NEW_LINE;

    if (!_data.empty()) {
        ss << _data;
    }

    return ss.str();
}

std::string HttpMessage::RequestToString() const {
    stringstream ss;

    ss << HttpMethodToString(_method) << " " << _path << " " << HttpVersionToString(_version) << NEW_LINE;

    ss << CommonHttpMessageToString();

    return ss.str();
}

std::string HttpMessage::ResponseToString() const {
    stringstream ss;

    ss << HttpVersionToString(_version) << " " << _status << NEW_LINE;

    ss << CommonHttpMessageToString();

    return ss.str();
}

std::string HttpMessage::ToString() const { return _isRequest ? RequestToString() : ResponseToString(); }