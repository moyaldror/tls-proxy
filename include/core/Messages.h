#pragma once

#include <atomic>
#include <iostream>
#include <string>
#include <utility>

enum class MessageTypes : int8_t { ERROR, OK, EMPTY, STRING_DATA, NUM_MESSAGE_TYPES };

// Class used to pass messages between layers, Messages can contain data of different types,
// error messages, etc.
class Message {
  public:
    Message(MessageTypes msgType) : _id(NextId()), _type(msgType) {}
    std::string ToString() const {
        return "<Id=" + std::to_string(_id) + ", type=" + MessageTypesToString(_type) + ">";
    }

    virtual ~Message() = default;

    static std::string MessageTypesToString(MessageTypes msgType) {
        switch (msgType) {
        case MessageTypes::ERROR:
            return "Error";
        case MessageTypes::OK:
            return "Ok";
        case MessageTypes::EMPTY:
            return "Empty";
        case MessageTypes::STRING_DATA:
            return "StringData";
        default:
            return "Unkown";
        }
    }

  protected:
    int32_t _id;
    MessageTypes _type;

  private:
    static int32_t NextId() {
        static int32_t id;
        return ++id;
    }
};

struct ErrorMessage : public Message {
    ErrorMessage(std::string error) : Message(MessageTypes::ERROR), ErrorString(std::move(error)) {}

    std::string ErrorString;
};

struct OkMessage : public Message {
    OkMessage() : Message(MessageTypes::OK) {}
};

struct EmptyMessage : public Message {
    EmptyMessage() : Message(MessageTypes::EMPTY) {}
};

struct StringDataMessage : public Message {
    StringDataMessage(std::string data) : Message(MessageTypes::STRING_DATA), Data(std::move(data)) {}

    std::string Data;
};