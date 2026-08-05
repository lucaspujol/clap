#pragma once

#include <functional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "Argument.hpp"
#include "TypeNames.hpp"
#include "ClapExceptions.hpp"
#include "ParseValue.hpp"
#include "Validators.hpp"

namespace clap {

    template<typename T, typename Derived>
    class TypedArgument : public Argument {
        public:
            TypedArgument(std::string names, std::string description)
            : Argument(std::move(names), std::move(description)) {}

            /// every typed argument consumes a value token. (Flag doesn't, and
            /// isn't typed.)
            bool takes_value() const noexcept override { return true; }

            /// the type is always shown; labelled validators narrow it after it.
            /// <string json|xml|yaml>, <int 1..10>
            std::string_view type_name() const override {
                if (!_label.empty()) return _label;
                return clap::TypeName<T>::value;
            }

            /// Restrict accepted values to an explicit set. needs == and <<
            Derived& choices(std::vector<T> allowed) {
                validator(clap::Choices(allowed));
                return self();
            }

            /// Restrict accepted values to [lo, hi] range. needs <= and <<
            Derived& range(T lo, T hi) {
                validator(clap::Range(lo, hi));
                return self();
            }

            const std::vector<std::string>& hints() const override { return _hints; }

            /// Register a custom validator. The function should return an empty
            /// string on success, or an error message on failure.
            ///
            /// Two optional members describe it in the help: label() narrows the
            /// type slot, for constraints on the shape of the token itself
            /// (<string json|xml|yaml>); hint() states a requirement next to the
            /// description ("must exist"), for everything else.
            template<typename F>
            Derived& validator(F func) {
                static_assert(std::is_invocable_r_v<std::string, const F&, const T&>,
                    "validator must be callable as std::string(const T&). "
                    "Check the value type: clap::FileExists/DirExists/NonexistentPath "
                    "need std::filesystem::path, clap::NonEmpty needs a type with .empty().");
                if constexpr ( requires { func.label(); }) {
                    std::string l = func.label();
                    if (!l.empty()) {
                        if (_label.empty())
                            _label = clap::TypeName<T>::value;
                        _label += " " + l;
                    }
                }
                if constexpr ( requires { func.hint(); }) {
                    std::string h = func.hint();
                    if (!h.empty())
                        _hints.push_back(std::move(h));
                }
                if constexpr (std::is_invocable_r_v<std::string, const F&, const T&>)
                    _validators.push_back(std::move(func));
                return self();
            }

            /// Keep this argument out of the help text and the usage line.
            /// It still parses exactly as before.
            Derived& hidden() {
                this->set_hidden();
                return self();
            }

            /// File this argument under a named section of the help instead of
            /// the default one. The name prints verbatim, casing included.
            ///
            /// Positionals cannot be grouped. This class backs positional("x")
            /// and variadic("x") as well as option("-x"), so the refusal is a
            /// runtime check on where App filed it rather than an absent method.
            Derived& group(std::string name) {
                if (this->is_positional())
                    throw clap::ConfigError(this->location(),
                        "group() is not available on positionals");
                this->set_group(std::move(name));
                return self();
            }

        protected:
            /// parses a T value, then runs it past every registered validator
            T parse_value(std::string_view value) {
                T v = clap::parse_checked<T>(value, names(), type_name());
                validate(v, value);
                return v;
            }

        private:
            Derived& self() { return static_cast<Derived&>(*this); }

            /// runs the validators in registration order. The first one to
            /// return a reason throws, so only one is ever reported.
            void validate(const T& v, std::string_view raw) {
                for (const auto& f : _validators) {
                    std::string msg = f(v);
                    if (!msg.empty())
                        throw clap::InvalidValue(std::string(raw), std::string(names()),
                                                 std::string(type_name()), msg);
                }
            }

            std::vector<std::function<std::string(const T&)>> _validators;
            std::string _label;
            std::vector<std::string> _hints;
    };
}
