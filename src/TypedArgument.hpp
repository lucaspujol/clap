#pragma once

#include <functional>
#include <string>
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
                _validators.push_back(std::move(func));
                return self();
            }

        protected:
            /// parses a T value, validates the range & choices requirements
            T parse_value(std::string_view value) {
                T v = clap::parse_checked<T>(value, names(), type_name());
                validate(v, value);
                return v;
            }

        private:
            Derived& self() { return static_cast<Derived&>(*this); }

            /// validates the requirements for .range() & .choices()
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
