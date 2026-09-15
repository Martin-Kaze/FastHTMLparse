#pragma once
#include <cstddef>
#include <string>

class Parser
{

    enum class State
    {
        DATA,
        TAG_OPEN,
        TAG_NAME,
        ATTRIBUTE_NAME,
        ATTRIBUTE_VALUE,
    };

    State state = State::DATA;
    bool closing = false;
    std::string tag_names;

public:
    void parsring(const char *buf, size_t size);
};