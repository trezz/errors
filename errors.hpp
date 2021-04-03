#include <array>
#include <concepts>
#include <cstdio>
#include <cstring>
#include <string>
#include <type_traits>

namespace errors {

// clang-format off
namespace details {

template <class From, class To>
concept convertible_to =
    std::is_convertible_v<From, To> &&
    requires(std::add_rvalue_reference_t<From> (&f)()) {
        static_cast<To>(f());
    };

} // namespace details

template<typename T>
concept error =
    std::is_default_constructible_v<T> &&
    std::is_move_constructible_v<T> &&
    std::is_move_assignable_v<T> &&
    requires(T err) {
        // Implicitely convertible to bool.
        { err } -> details::convertible_to<bool>;
        // Access to the error message.
        { err.error() } -> std::same_as<std::string>;
    };

template<typename T>
concept unwrappable =
    requires(T v) {
        { v.unwrap() } -> error;
    };
// clang-format on

template<unwrappable E>
constexpr auto unwrap(const E& err)
{
    return err.unwrap();
}

template<error T, error E>
constexpr bool as(const E& err, T& target)
{
    if constexpr (std::is_assignable_v<T, E>) {
        target = err;
        return true;
    }
    if constexpr (std::is_constructible_v<T, E>) {
        T new_target(err);
        if constexpr (std::is_move_assignable_v<T>) {
            target = std::move(new_target);
            return true;
        }
        if constexpr (std::is_copy_assignable_v<T>) {
            target = new_target;
            return true;
        }
    }

    if constexpr (unwrappable<E>) {
        return as(unwrap(err), target);
    }
    return false;
}

template<error Target, error Err>
constexpr bool is(const Err& err)
{
    Target target;
    return as(err, target);
}

struct [[nodiscard]] basic_error
{
    operator bool() const { return not message.empty(); }
    std::string error() const { return message; }

    std::string message;
};

template<error Err = basic_error, typename... Args>
Err make(const std::string& msg, Args&&... args)
{
    return { msg, std::forward<Args>(args)... };
}

template<error Err = basic_error, typename... Args>
Err format(std::string_view fmt, Args&&... args)
{
    constexpr size_t buffer_size = 8192;
    std::string buffer;
    buffer.resize(buffer_size);

    std::snprintf(buffer.data(), buffer_size, fmt.data(), std::forward<Args>(args)...);

    return make<Err>(buffer.data());
}

template<typename T, error E = basic_error, typename U, error R, typename Then>
std::tuple<T, E> try_then(const std::tuple<U, R>& result, Then&& func)
{
    E err;

    auto [res, errRes] = result;
    if (errRes) {
        static T defaultT{};
        as(errRes, err);
        return { defaultT, err };
    }
    auto [v, errF] = func(res);

    as(errF, err);
    return { v, err };
}

} // namespace errors
