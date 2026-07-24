#pragma once

#include <algorithm>
#include <optional>
#include <string>
#include <sstream>
#include <utility>
#include <vector>

#include "Argument.hpp"
#include "TypeNames.hpp"
#include "ClapExceptions.hpp"
#include "ParseValue.hpp"

namespace clap {

    template<typename T, typename Derived>
    class TypedArgument : public Argument {
        public:
            TypedArgument(std::string names, std::string description)
            : Argument(std::move(names), std::move(description)) {}
            
            /// help label checks the choices first: <xml|json|yaml>, not <string>
            std::string_view type_name() const override {
                if (!_choices_label.empty()) return _choices_label;
                if (!_range_label.empty())   return _range_label;
                return clap::TypeName<T>::value;
            }

            /// Restrict accepted values to an explicit set. needs == and <<
            Derived& choices(std::vector<T> allowed) {
                std::ostringstream oss;
                for (size_t i = 0; i < allowed.size(); ++i) {
                    if (i) oss << '|';
                    oss << allowed[i];
                }
                _choices_label = oss.str();
                _choices = std::move(allowed);
                return self();
            } 

            /// Restrict accepted values to [lo, hi] range. needs < and <<
            Derived& range(T lo, T hi) {
                _range_label = std::string(clap::TypeName<T>::value) + " " + label(lo) + ".." + label(hi);
                _range.emplace(std::move(lo), std::move(hi));
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
                if (!_choices.empty() && std::find(_choices.begin(), _choices.end(), v) == _choices.end()) {
                    throw clap::InvalidValue(
                        std::string(raw),
                        std::string(names()),
                        std::string(type_name())
                    );
                }
                if (_range && (v < _range->first || _range->second < v)) {
                    throw clap::InvalidValue(
                        std::string(raw),
                        std::string(names()),
                        std::string(type_name())
                    );
                }
            }

            /// for formatting in InvalidValue hint
            static std::string label(const T& v) {
                std::ostringstream oss;
                oss << v;
                return oss.str();
            }
            
            std::vector<T> _choices;
            std::string _choices_label;
            std::optional<std::pair<T, T>> _range;
            std::string _range_label;
    };
}