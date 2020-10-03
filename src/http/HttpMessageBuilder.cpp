#include "http/HttpMessageBuilder.h"

#include <utility>

using namespace std;

static const string BASIC_REQUEST = "GET / HTTP/1.0\r\nUser-Agent: dummy\r\n\r\n";
static const string BASIC_RESPONSE = "HTTP/1.0 200 OK\r\nUser-Agent: dummy\r\n\r\n";

HttpMessageBuilder::HttpMessageBuilder(bool isRequest) : _msg(isRequest ? BASIC_REQUEST : BASIC_RESPONSE) {}

HttpMessageBuilder::HttpMessageBuilder(HttpMessage message) : _msg(std::move(message)) {}

HttpMessageBuilder::HttpMessageBuilder(const string& message) : _msg(HttpMessage(message)) {}

HttpMessage HttpMessageBuilder::Build() {
    return HttpMessage(_msg.ToString()); // do it to validate the message
}