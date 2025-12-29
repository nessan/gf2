#pragma once
// SPDX-FileCopyrightText: 2025 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT

/// @file
/// Macros that improve on [standard assert](https://en.cppreference.com/w/cpp/error/assert). <br>
/// See the [Assertions](docs/pages/assert.md) page for all the details.

#include <format>
#include <print>
#include <string>
#include <source_location>

// Avoid macro redefinition warnings ...
#ifdef gf2_assert
    #undef gf2_assert
#endif
#ifdef gf2_assert_eq
    #undef gf2_assert_eq
#endif
#ifdef gf2_debug_assert
    #undef gf2_debug_assert
#endif
#ifdef gf2_debug_assert_eq
    #undef gf2_debug_assert_eq
#endif
#ifdef gf2_always_assert
    #undef gf2_always_assert
#endif
#ifdef gf2_always_assert_eq
    #undef gf2_always_assert_eq
#endif

/// A confirmation macro that checks a boolean condition `cond`. <br>
/// On failure, the confirmation prints an error message and exits the program with a non-zero status code.
/// The message always includes the source code location of the confirmation and a stringified version of the condition
/// that failed.
///
/// The first argument is the condition to check, the (optional) rest are passed to `std::format` to form a custom
/// extra message that is printed if the condition fails.
#define gf2_always_assert(expr, ...) \
    if (!(expr)) [[unlikely]] { gf2::assert_failed(#expr, std::source_location::current() __VA_OPT__(, __VA_ARGS__)); }

/// A confirmation macro that checks the equality of two values `lhs` and `rhs`. <br>
/// On failure, the confirmation prints an error message and exits the program with a non-zero status code.
/// The message always includes the source code location of the confirmation and a stringified version of the values
/// that failed the equality check.
///
/// The first two arguments are the values to check for equality, the (optional) rest are passed to `std::format` to
/// form a custom extra message that is printed if the values are not equal.
#define gf2_always_assert_eq(a, b, ...)                                                                     \
    if (!((a) == (b))) [[unlikely]] {                                                                       \
        gf2::assert_eq_failed(#a, #b, (a), (b), std::source_location::current() __VA_OPT__(, __VA_ARGS__)); \
    }

/// A confirmation macro that checks a boolean condition `cond`. <br>
/// On failure, the confirmation prints an error message and exits the program with a non-zero status code.
/// The message always includes the source code location of the confirmation and a stringified version of the condition
/// that failed.
///
/// @note The `gf2_debug_assert` macro expands to a no-op **unless** the `DEBUG` flag is set.
///
/// The first argument is the condition to check, the (optional) rest are passed to `std::format` to form a custom
/// extra message that is printed if the condition fails.
#ifdef DEBUG
    #define gf2_debug_assert(cond, ...) gf2_always_assert(cond __VA_OPT__(, __VA_ARGS__))
#else
    #define gf2_debug_assert(cond, ...) void(0)
#endif

/// A confirmation macro that checks the equality of two values `lhs` and `rhs`. <br>
/// On failure, the confirmation prints an error message and exits the program with a non-zero status code.
/// The message always includes the source code location of the confirmation and a stringified version of the values
/// that failed the equality check.
///
/// @note The `gf2_debug_assert_eq` macro expands to a no-op **unless** the `DEBUG` flag is set.
///
/// The first two arguments are the values to check for equality, the (optional) rest are passed to `std::format` to
/// form a custom extra message that is printed if the values are not equal.
#ifdef DEBUG
    #define gf2_debug_assert_eq(lhs, rhs, ...) gf2_always_assert_eq(lhs, rhs __VA_OPT__(, __VA_ARGS__))
#else
    #define gf2_debug_assert_eq(a, b, ...) void(0)
#endif

/// A confirmation macro that checks a boolean condition `cond`. <br>
/// On failure, the confirmation prints an error message and exits the program with a non-zero status code.
/// The message always includes the source code location of the confirmation and a stringified version of the condition
/// that failed.
///
/// @note The `gf2_assert` macro expands to a no-op **only** when the `NDEBUG` flag is set.
///
/// The first argument is the condition to check, the (optional) rest are passed to `std::format` to form a custom
/// extra message that is printed if the condition fails.
#ifdef NDEBUG
    #define gf2_assert(cond, ...) void(0)
#else
    #define gf2_assert(cond, ...) gf2_always_assert(cond __VA_OPT__(, __VA_ARGS__))
#endif

/// A confirmation macro that checks the equality of two values `lhs` and `rhs`. <br>
/// On failure, the confirmation prints an error message and exits the program with a non-zero status code.
/// The message always includes the source code location of the confirmation and a stringified version of the values
/// that failed the equality check.
///
/// @note The `gf2_assert_eq` macro expands to a no-op **only** when the `NDEBUG` flag is set.
///
/// The first two arguments are the values to check for equality, the (optional) rest are passed to `std::format` to
/// form a custom extra message that is printed if the values are not equal.
#ifdef NDEBUG
    #define gf2_assert_eq(lhs, rhs, ...) void(0)
#else
    #define gf2_assert_eq(lhs, rhs, ...) gf2_always_assert_eq(lhs, rhs __VA_OPT__(, __VA_ARGS__))
#endif

// ---------------------------------------------------------------------------------------------------------------------
// Functions to handle confirmation failures ...
// ---------------------------------------------------------------------------------------------------------------------
namespace gf2 {

/// By default, all failed confirmations exit the program. <br>
/// You can set this variable to `false` to prevent this behavior (useful for unit tests).
inline static auto exit_on_assert_failure = true;

// Given a path like `/home/jj/dev/project/src/foo.cpp` this returns its "basename" `foo.cpp`
constexpr auto
basename(std::string_view path) {
    const auto pos = path.find_last_of("/\\");
    if (pos == std::string_view::npos) { return path; }
    return path.substr(pos + 1);
}

// Prints an error message with source code location information and optionally exits the program.
// This handles simple boolean gf2_assert `gf2_assert(expr, ...)` failures.
void
assert_failed(std::string_view expr_str, std::source_location loc, std::string_view msg_fmt = "", auto... msg_args) {
    std::println(stderr);
    std::println(stderr, "GF2 FAILED: `gf2_assert({})` [{}:{}]", expr_str, basename(loc.file_name()), loc.line());
    if (!msg_fmt.empty()) {
        std::vprint_nonunicode(stderr, msg_fmt, std::make_format_args(msg_args...));
        std::println(stderr);
    }
    std::println(stderr);
    if (exit_on_assert_failure) { ::exit(1); }
}

// Prints an error message with source code location information and optionally exits the program.
// This handles fails for equality confirmations `gf2_assert_eq(x, y, ...)`.
void
assert_eq_failed(std::string_view lhs_str, std::string_view rhs_str, auto lhs, auto rhs, std::source_location loc,
                 std::string_view msg_fmt = "", auto... msg_args) {
    std::println(stderr);
    std::println(stderr, "GF2 FAILED: `gf2_assert_eq({}, {})` [{}:{}]", lhs_str, rhs_str, basename(loc.file_name()),
                 loc.line());
    if (!msg_fmt.empty()) {
        std::vprint_nonunicode(stderr, msg_fmt, std::make_format_args(msg_args...));
        std::println(stderr);
    }
    std::println(stderr, "lhs = {}", lhs);
    std::println(stderr, "rhs = {}", rhs);
    std::println(stderr);
    if (exit_on_assert_failure) { ::exit(1); }
}

} // namespace gf2
