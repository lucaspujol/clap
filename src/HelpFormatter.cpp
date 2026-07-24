#include "HelpFormatter.hpp"
#include "Argument.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>

std::string clap::HelpFormatter::usage_token(const clap::Argument& arg, bool positional) const {
    if (positional) {
        std::string core = "<" + std::string(arg.names()) + ">";
        std::string tail = arg.is_multi() ? "..." : "";
        return arg.is_required() ? core + tail : "[" + core + "]" + tail;
    }
    if (!arg.takes_value())
        return "[" + std::string(arg.primary_name()) + "]";

    std::string core = std::string(arg.primary_name()) + " <" + std::string(arg.type_name()) + ">";
    std::string tail = arg.is_multi() ? "..." : "";
    return arg.is_required() ? core + tail : "[" + core + "]" + tail;
}

std::string clap::HelpFormatter::name_col(const clap::Argument& arg) const {
    std::string s = "  " + std::string(arg.names());
    if (arg.is_multi())
        s += "...";
    return s;
}

std::string clap::HelpFormatter::type_col(const clap::Argument& arg) const {
    if (arg.type_name().empty())
        return "";
    return "<" + std::string(arg.type_name()) + ">";
}

std::string clap::HelpFormatter::annotation(const clap::Argument& arg) const {
    if (arg.is_required())
        return " (required)";
    if (!arg.default_str().empty())
        return " (default: " + arg.default_str() + ")";
    return "";
}

size_t clap::HelpFormatter::name_width() const {
    size_t max_w = 0;
    for (const auto& a : _options)
        max_w = std::max(max_w, name_col(*a).size());
    for (const auto& p : _positionals)
        max_w = std::max(max_w, name_col(*p).size());
    return max_w + 2;
}

size_t clap::HelpFormatter::type_width() const {
    size_t max_w = 0;
    for (const auto& a : _options)
        max_w = std::max(max_w, type_col(*a).size());
    for (const auto& p : _positionals)
        max_w = std::max(max_w, type_col(*p).size());
    return max_w + 2;
}

std::string clap::HelpFormatter::usage() const {
    std::ostringstream oss;
    oss << "Usage: " << _name;
    for (const auto& a : _options)
        oss << " " << usage_token(*a, false);
    for (const auto& p : _positionals)
        oss << " " << usage_token(*p, true);
    return oss.str();
}

std::string clap::HelpFormatter::help() const {
    std::ostringstream oss;
    oss << usage() << "\n\n" << _description << "\n";

    const size_t name_w = name_width();
    const size_t type_w = type_width();

    oss << "\nOptions:\n";
    for (const auto& a : _options)
        oss << std::left << std::setw(name_w) << name_col(*a)
            << std::setw(type_w) << type_col(*a)
            << a->description() << annotation(*a) << "\n";

    if (!_positionals.empty()) {
        oss << "\nArguments:\n";
        for (const auto& p : _positionals)
            oss << std::left << std::setw(name_w) << name_col(*p)
                << std::setw(type_w) << type_col(*p)
                << p->description() << annotation(*p) << "\n";
    }
    return oss.str();
}
