#pragma once

#include <filesystem>
#include <initializer_list>
#include <ios>
#include <locale>
#include <string>
#include <sstream>
#include <system_error>
#include <utility>
#include <vector>

namespace clap {
    /// Builtin validators. Each is a callable that takes a value and returns an
    /// empty string on success or an error message on failure. Each also has
    /// optional members label() and hint() that describe the constraint for help.
    namespace detail {
        /// Range: accepts [lo, hi]. needs <= and <<. label: "lo..hi"
        template<typename T>
        class RangeFn {
            public:
                constexpr RangeFn(T lo, T hi) : _lo(lo), _hi(hi) {}

                std::string operator()(const T& v) const {
                    if (!(_lo <= v && v <= _hi))
                        return "value out of range.";
                    return "";
                }

                std::string label() const {
                    std::ostringstream oss;
                    oss.imbue(std::locale::classic());
                    oss << std::boolalpha << _lo << ".." << _hi;
                    return oss.str();
                }

            private:
                T _lo;
                T _hi;
        };

        /// Choices: accepts only listed values. needs == and <<. label: "a|b|c"
        template<typename T>
        class ChoicesFn {
            public:
                ChoicesFn(std::vector<T> vals) : _choices(std::move(vals)) {}

                std::string operator()(const T& v) const {
                    for (const auto& current : _choices)
                        if (v == current)
                            return "";
                    return "value not in allowed choices.";
                }

                std::string label() const {
                    std::ostringstream oss;
                    oss.imbue(std::locale::classic());
                    oss << std::boolalpha;
                    bool first = true;
                    for (const auto& current : _choices) {
                        if (!first) oss << '|';
                        oss << current;
                        first = false;
                    }
                    return oss.str();
                }

            private:
                std::vector<T> _choices;
        };

        /// Min: accepts >= min. needs <= and <<. hint: ">= min"
        template<typename T>
        class MinFn {
            public:
                constexpr MinFn(T min) : _min(min) {}

                std::string operator()(const T& v) const {
                    // positive form so NaN, for which every comparison is
                    // false, is rejected instead of accepted.
                    if (!(_min <= v))
                        return "value too small";
                    return "";
                }

                std::string hint() const {
                    std::ostringstream oss;
                    oss.imbue(std::locale::classic());
                    oss << std::boolalpha << ">= " << _min;
                    return oss.str();
                }

            private:
                T _min;
        };

        /// Max: accepts <= max. needs <= and <<. hint: "<= max"
        template<typename T>
        class MaxFn {
            public:
                constexpr MaxFn(T max) : _max(max) {}

                std::string operator()(const T& v) const {
                    if (!(v <= _max))
                        return "value too big";
                    return "";
                }

                std::string hint() const {
                    std::ostringstream oss;
                    oss.imbue(std::locale::classic());
                    oss << std::boolalpha << "<= " << _max;
                    return oss.str();
                }

            private:
                T _max;
        };

        /// FileExists: accepts only existing files. hint: "must exist"
        class FileExistsFn {
            public:
                std::string operator()(const std::filesystem::path& v) const {
                    std::error_code ec;
                    if (std::filesystem::is_directory(v, ec))
                        return v.string() + " is a directory";
                    if (!std::filesystem::is_regular_file(v, ec))
                        return "file does not exist";
                    return "";
                }

                std::string hint() const { return "must exist"; }
        };

        /// DirExists: accepts only existing directories. hint: "must be a directory"
        class DirExistsFn {
            public:
            std::string operator()(const std::filesystem::path& v) const {
                std::error_code ec;
                if (std::filesystem::is_regular_file(v, ec))
                    return v.string() + " is a regular file";
                if (!std::filesystem::is_directory(v, ec))
                    return "directory does not exist";
                return "";
            }

            std::string hint() const { return "must be a directory"; }
        };

        /// NonexistentPath: accepts only paths that do not exist. hint: "must not exist"
        class NonexistentPathFn {
            public:
                std::string operator()(const std::filesystem::path& v) const {
                    std::error_code ec;
                    // symlink_status, not exists: a dangling link reads as
                    // absent but still occupies the name, and creating there
                    // fails with EEXIST.
                    if (std::filesystem::exists(std::filesystem::symlink_status(v, ec)))
                        return "path already exists";
                    return "";
                }

                std::string hint() const { return "must not exist"; }
        };

        /// NonEmpty: accepts only non-empty values.    needs empty(). hint: "non-empty"
        class NonEmptyFn {
            public:
                template<typename T>
                requires requires (const T& v) { { v.empty() } -> std::convertible_to<bool>; }
                std::string operator()(const T& v) const {
                    if (v.empty())
                        return "value cannot be empty";
                    return "";
                }

                std::string hint() const { return "non-empty"; }
        };

    } // namespace detail

    /// Each comparing validator carries a const char* overload that maps to
    /// std::string. Without it T deduces as const char* and the validator keeps
    /// raw pointers: == would compare addresses instead of text, <= would order
    /// by address, and a pointer to anything but a literal dangles as soon as
    /// the caller's temporary dies. The overload makes the conversion implicit,
    /// so literals go straight into Choices(), Range(), Min() and Max().

    /// Usage:
    /// clap::Range(lo, hi)
    template<typename T>
    detail::RangeFn<T> Range(T lo, T hi) { return detail::RangeFn<T>(std::move(lo), std::move(hi)); }
    inline detail::RangeFn<std::string> Range(const char* lo, const char* hi) {
        return detail::RangeFn<std::string>(lo, hi);
    }

    /// Usage:
    /// clap::Choices(vals) where is a vector or initializer_list of T
    /// clap::Choices({a, b, c})
    template<typename T>
    detail::ChoicesFn<T> Choices(std::vector<T> vals) { return detail::ChoicesFn<T>(std::move(vals)); }
    template<typename T>
    detail::ChoicesFn<T> Choices(std::initializer_list<T> vals) { return detail::ChoicesFn<T>(vals); }
    inline detail::ChoicesFn<std::string> Choices(std::initializer_list<const char*> vals) {
        return detail::ChoicesFn<std::string>(std::vector<std::string>(vals.begin(), vals.end()));
    }

    /// Usage:
    /// clap::Min(min)
    template<typename T>
    detail::MinFn<T> Min(T min) { return detail::MinFn<T>(std::move(min)); }
    inline detail::MinFn<std::string> Min(const char* min) { return detail::MinFn<std::string>(min); }

    /// Usage:
    /// clap::Max(max)
    template<typename T>
    detail::MaxFn<T> Max(T max) { return detail::MaxFn<T>(std::move(max)); }
    inline detail::MaxFn<std::string> Max(const char* max) { return detail::MaxFn<std::string>(max); }

    /// Usage:
    /// clap::FileExists
    /// clap::DirExists
    /// clap::NonexistentPath
    /// clap::NonEmpty
    inline constexpr detail::FileExistsFn FileExists{};
    inline constexpr detail::DirExistsFn DirExists{};
    inline constexpr detail::NonexistentPathFn NonexistentPath{};
    inline constexpr detail::NonEmptyFn NonEmpty{};

} // namespace clap
