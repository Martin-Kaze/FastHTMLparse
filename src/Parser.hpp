#pragma once
#include <cstddef>

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

public:
    void parsring(const char *buf, size_t size);
};