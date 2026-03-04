#pragma once

#include <string>
#include <sstream>
#include <vector>

namespace clap {
    class IArgument {
        public:
            IArgument(std::string names, std::string description)
                : _names_raw(std::move(names)),
                  _names(split(_names_raw, ',')),
                  _description(std::move(description)) {}
            virtual ~IArgument() = default;

            virtual void parse(std::string_view value) = 0;
            virtual std::string_view type_name() const = 0;

            virtual std::string prefix() const {
                std::ostringstream oss;
                oss << "  " << _names_raw;
                if (!type_name().empty())
                    oss << " <" << type_name() << ">";
                return oss.str();
            }

            std::string_view names() const noexcept { return _names_raw; }
            std::string_view description() const noexcept { return _description; }

            bool is_required() const noexcept { return _required; }

            virtual bool is_set() const noexcept = 0;
            virtual bool takes_value() const noexcept = 0;

            bool matches(std::string_view name) const {
                for (const auto& arg_name : _names) {
                    if (arg_name == name)
                        return true;
                }
                return false;
            }

        protected:
            void set_required() noexcept { _required = true; }

        private:
            std::string _names_raw;
            std::vector<std::string> _names;
            std::string _description;
            bool _required = false;

            static std::vector<std::string> split(const std::string &str, char delimiter) {
                std::vector<std::string> tokens;
                std::istringstream iss(str);
                std::string token;

                while (std::getline(iss, token, delimiter)) {
                    token.erase(0, token.find_first_not_of(" \t\n\r\f\v"));
                    token.erase(token.find_last_not_of(" \t\n\r\f\v") + 1);
                    if (!token.empty())
                        tokens.push_back(token);
                }
                return tokens;
            }
    };    
}
