// fasthtml: a streaming CSS-selector matcher for HTML.
//
//   auto sel = fasthtml::Selector::compile("div.content > p").value();
//   fasthtml::Matcher m(sel);
//   m.on_match([](std::string_view text, void *) { /* ... */ });
//   m.feed(chunk1); m.feed(chunk2); ...
//   m.finish();
//
// No DOM is built. The document can be fed in pieces of any size, split anywhere.
#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace fasthtml
{
    namespace detail
    {
        class Engine;
    }

    inline constexpr int version_major = 1;
    inline constexpr int version_minor = 0;
    inline constexpr int version_patch = 0;

    // A compiled selector. Immutable, cheap to copy, and safe to share between threads.
    //
    // Supported: tag, #id, .class (at most one id and one class per part, in any order),
    // the child combinator "a > b" and the descendant combinator "a b". Up to 16 parts.
    // Tag names are case-insensitive, ids and classes are case-sensitive.
    class Selector
    {
    public:
        // Returns std::nullopt for an invalid or unsupported selector; *error then explains why.
        static std::optional<Selector> compile(std::string_view text, std::string *error = nullptr);

        // The selector text as it was given.
        std::string_view text() const;

    private:
        struct Impl;
        explicit Selector(std::shared_ptr<const Impl> impl) : impl_(std::move(impl)) {}
        std::shared_ptr<const Impl> impl_;
        friend class Matcher;
    };

    // Counters since construction or the last reset().
    struct Stats
    {
        std::size_t matches = 0;                 // elements matching the whole selector
        std::size_t tag_matches = 0;             // tag names equal to a selector part's tag
        std::size_t attribute_matches = 0;       // id= / class= attributes seen
        std::size_t attribute_value_matches = 0; // id / class values equal to a selector part's
    };

    // Matches one selector against one document stream. Not thread-safe: use one per thread.
    class Matcher
    {
    public:
        // Receives the text content of a matched element (tags removed, entities not decoded).
        // The view is only valid during the call. Void elements (<img>, <br>, ...) give "".
        using Callback = void (*)(std::string_view text, void *user);

        explicit Matcher(const Selector &selector);
        ~Matcher();
        Matcher(Matcher &&) noexcept;            // a moved-from Matcher must not be used again
        Matcher &operator=(Matcher &&) noexcept;
        Matcher(const Matcher &) = delete;
        Matcher &operator=(const Matcher &) = delete;

        // Without a callback only the counters are updated (fastest).
        void on_match(Callback callback, void *user = nullptr);

        // Any callable taking std::string_view, e.g. a lambda with captures.
        // It is stored by reference: it must outlive the Matcher (or the next on_match call).
        template <class F>
            requires(std::is_invocable_v<F &, std::string_view> && !std::is_convertible_v<F &, Callback>)
        void on_match(F &callable)
        {
            on_match([](std::string_view text, void *user) { (*static_cast<F *>(user))(text); }, &callable);
        }

        // Parses the next piece of the document.
        void feed(const char *data, std::size_t size);
        void feed(std::string_view data) { feed(data.data(), data.size()); }

        // End of the document: emits the text of matched elements that were never closed.
        void finish();

        // Starts over for a new document. Keeps the selector and the callback.
        void reset();

        std::size_t match_count() const;
        Stats stats() const;

    private:
        std::shared_ptr<const Selector::Impl> selector_;
        std::unique_ptr<detail::Engine> engine_;
    };

    // One-call helpers for a whole document in memory.
    std::size_t count(std::string_view html, const Selector &selector);
    std::vector<std::string> extract_text(std::string_view html, const Selector &selector);

    // Same, but compile the selector first. Throw std::invalid_argument for an invalid selector.
    std::size_t count(std::string_view html, std::string_view selector);
    std::vector<std::string> extract_text(std::string_view html, std::string_view selector);
}
