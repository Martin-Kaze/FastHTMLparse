#include <fasthtml/fasthtml.hpp>

#include <stdexcept>

#include "engine.hpp"
#include "selector_compiler.hpp"

namespace fasthtml
{
    struct Selector::Impl
    {
        std::string text;
        detail::SelectorCompiler compiled;
    };

    std::optional<Selector> Selector::compile(std::string_view text, std::string *error)
    {
        auto impl = std::make_shared<Impl>();
        impl->text.assign(text.data(), text.size());
        if (!impl->compiled.compile(text))
        {
            if (error)
                *error = impl->compiled.error();
            return std::nullopt;
        }
        if (error)
            error->clear();
        return Selector(std::move(impl));
    }

    std::string_view Selector::text() const
    {
        return impl_->text;
    }

    Matcher::Matcher(const Selector &selector)
        : selector_(selector.impl_), engine_(std::make_unique<detail::Engine>())
    {
    }

    Matcher::~Matcher() = default;
    Matcher::Matcher(Matcher &&) noexcept = default;
    Matcher &Matcher::operator=(Matcher &&) noexcept = default;

    void Matcher::on_match(Callback callback, void *user)
    {
        engine_->set_callback(callback, user);
    }

    void Matcher::feed(const char *data, std::size_t size)
    {
        engine_->feed(data, size, selector_->compiled);
    }

    void Matcher::finish()
    {
        engine_->finish();
    }

    void Matcher::reset()
    {
        engine_->reset();
    }

    std::size_t Matcher::match_count() const
    {
        return engine_->matches();
    }

    Stats Matcher::stats() const
    {
        Stats s;
        s.matches = engine_->matches();
        s.tag_matches = engine_->tag_matches();
        s.attribute_matches = engine_->attribute_matches();
        s.attribute_value_matches = engine_->attribute_value_matches();
        return s;
    }

    std::size_t count(std::string_view html, const Selector &selector)
    {
        Matcher m(selector);
        m.feed(html);
        m.finish();
        return m.match_count();
    }

    std::vector<std::string> extract_text(std::string_view html, const Selector &selector)
    {
        std::vector<std::string> out;
        Matcher m(selector);
        m.on_match([](std::string_view text, void *user) { static_cast<std::vector<std::string> *>(user)->emplace_back(text); },
                   &out);
        m.feed(html);
        m.finish();
        return out;
    }

    namespace
    {
        Selector compile_or_throw(std::string_view text)
        {
            std::string error;
            auto sel = Selector::compile(text, &error);
            if (!sel)
                throw std::invalid_argument("fasthtml: invalid selector \"" + std::string(text) + "\": " + error);
            return *std::move(sel);
        }
    }

    std::size_t count(std::string_view html, std::string_view selector)
    {
        return count(html, compile_or_throw(selector));
    }

    std::vector<std::string> extract_text(std::string_view html, std::string_view selector)
    {
        return extract_text(html, compile_or_throw(selector));
    }
}
