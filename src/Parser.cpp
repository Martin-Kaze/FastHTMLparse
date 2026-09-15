#include "Parser.hpp"
#include <iostream>

void Parser::parsring(const char *buf, size_t size)
{

    for (size_t i = 0; i < size; i++)
    {

        char c = *(buf + i);

        switch (state)
        {

        case State::DATA:

            if (c == '<')
            {
                state = State::TAG_OPEN;
            }
            break;

        case State::TAG_OPEN:

            if (c == '/')
            {
                closing = true;
                state = State::TAG_NAME;
            }
            else
            {
                closing = false;
                state = State::TAG_NAME;
                tag_names.push_back(c);
            }
            break;

        case State::TAG_NAME:
            if (c == '>')
            {
                state = State::DATA;
                tag_names.push_back(' ');
            }
            else if (c == ' ')
            {
                state = State::ATTRIBUTE_NAME;
                tag_names.push_back(' ');
            }
            else
            {
                tag_names.push_back(c);
            }
            break;

        case State::ATTRIBUTE_NAME:
            if (c == '=')
            {
                state = State::ATTRIBUTE_VALUE;
            }
            else if (c == '>')
            {
                state = State::DATA;
            }
            break;

        case State::ATTRIBUTE_VALUE:
            if (c == '>')
            {
                state = State::DATA;
            }
            break;
        }
    }

    std::cout << "Hi: ";
    for (auto it : tag_names)
    {
        std::cout << it;
    }
}