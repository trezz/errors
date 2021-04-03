#include <chrono>
#include <cstdio>
#include <exception>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <iostream>
#include <type_traits>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "errors.hpp"

TEST_CASE("error basics")
{
    const auto err = errors::make("abc");
    CHECK(err);
    CHECK(err.error() == "abc");
}

namespace fs {

template<errors::error E = errors::basic_error>
struct path_error
{
    std::string op;
    std::string path;
    E err;

    operator bool() const { return err; }
    std::string error() const { return op + " " + path + ": " + err.error(); }
    E unwrap() const { return err; }
};

} // namespace fs

errors::basic_error errno_err()
{
    const auto code = errno;
    const auto message = std::error_code(code, std::generic_category()).message();
    return errors::format("%s (%d)", message.data(), code);
}

std::tuple<FILE*, fs::path_error<>> open_file(std::string_view path, std::string_view mode)
{
    auto* f = std::fopen(path.data(), mode.data());
    if (f == nullptr) {
        return { nullptr, fs::path_error<>{ .op = "fopen", .path = path.data(), .err = errno_err() } };
    }
    return { f, {} };
}

std::tuple<std::string, errors::basic_error> read_file(FILE* f)
{
    constexpr size_t buffer_size = 8192;
    std::string buffer;
    buffer.resize(buffer_size);

    errno = 0;
    std::fread(buffer.data(), sizeof(char), buffer_size, f);
    if (errno != 0) {
        return { {}, errno_err() };
    }

    return { buffer, {} };
}

std::tuple<int, errors::basic_error> to_int(const std::string& s)
{
    try {
        const auto v = std::stoi(s);
        if (v == 0 && (s.empty() || s[0] != '0')) {
            return { 0, errors::format("stoi %s: not an integer", s.data()) };
        }
        return { v, {} };
    } catch (const std::exception& e) {
        return { 0, { e.what() } };
    }
}

TEST_CASE("unwrap as")
{
    auto [f42, err42] = open_file("testdata/42.txt", "r");
    CHECK(not err42);
    CHECK(f42 != nullptr);

    auto [f1, err1] = open_file("testdata/1.txt", "r");
    CHECK(err1);
    CHECK(err1.error() == "fopen testdata/1.txt: No such file or directory (2)");
    auto errno_err = errors::unwrap(err1);
    CHECK(errno_err.error() == "No such file or directory (2)");
    errors::basic_error base;
    errors::as(err1, base);
    CHECK(base);
    CHECK(base.error() == "No such file or directory (2)");
}

TEST_CASE("if_ok")
{
    auto [v, err] = errors::try_then<int>(open_file("testdata/42.txt", "r"), [](FILE* f) {
        return errors::try_then<int>(read_file(f), [](const std::string& content) { return to_int(content); });
    });
    CHECK(not err);
    CHECK(v == 42);

    auto [v1, err1] = errors::try_then<int>(open_file("testdata/1.txt", "r"), [](FILE* f) {
        return errors::try_then<int>(read_file(f), [](const std::string& content) { return to_int(content); });
    });
    CHECK(err1);
    CHECK(err1.error() == "No such file or directory (2)");

    auto [v2, err2] = errors::try_then<int>(open_file("testdata/toto.txt", "r"), [](FILE* f) {
        return errors::try_then<int>(read_file(f), [](const std::string& content) { return to_int(content); });
    });
    CHECK(err2);
    CHECK(err2.error() == "stoi: no conversion");
}
