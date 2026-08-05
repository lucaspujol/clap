#include "HelpFormatter.hpp"
#include "Argument.hpp"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <sstream>

inline clap::HelpFormatter::HelpFormatter(std::string_view name, std::string_view description,
                                          const ArgList& options, const ArgList& positionals,
                                          std::vector<std::pair<std::string, std::string>> examples,
                                          std::string_view footer, bool footer_wrap)
    : _name(name), _description(description), _examples(std::move(examples)),
      _footer(footer), _footer_wrap(footer_wrap) {
    for (const auto& o : options) {
        if (o->is_hidden())
            continue;
        _options.push_back(o.get());
        if (o->group().empty()) {
            _ungrouped.push_back(o.get());
            continue;
        }
        auto it = std::find_if(_groups.begin(), _groups.end(),
                               [&](const auto& g) { return g.first == o->group(); });
        if (it == _groups.end())
            _groups.emplace_back(o->group(), std::vector<const Argument*>{o.get()});
        else
            it->second.push_back(o.get());
    }
    for (const auto& p : positionals)
        if (!p->is_hidden())
            _positionals.push_back(p.get());
}

inline std::string clap::HelpFormatter::usage_token(const clap::Argument& arg, bool positional) const {
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

inline std::string clap::HelpFormatter::name_col(const clap::Argument& arg) const {
    std::string s = "  " + std::string(arg.names());
    if (arg.is_multi())
        s += "...";
    return s;
}

inline std::string clap::HelpFormatter::type_col(const clap::Argument& arg) const {
    if (arg.type_name().empty())
        return "";
    return "<" + std::string(arg.type_name()) + ">";
}

inline std::vector<std::string> clap::HelpFormatter::annotation(const clap::Argument& arg) const {
    std::vector<std::string> units;
    if (arg.is_required())
        units.push_back("(required)");
    else if (!arg.default_str().empty())
        units.push_back("(default: " + arg.default_str() + ")");
    if (!arg.env_key().empty())
        units.push_back("(env: " + arg.env_key() + ")");
    for (const auto& hint : arg.hints())
        units.push_back("(" + hint + ")");
    return units;
}

inline void clap::HelpFormatter::append_units(std::vector<std::string>& lines,
                                              const std::vector<std::string>& units,
                                              size_t width) const {
    for (const auto& unit : units) {
        if (lines.back().empty())
            lines.back() = unit;
        else if (lines.back().size() + 1 + unit.size() <= width)
            lines.back() += " " + unit;
        else
            lines.push_back(unit);
    }
}

inline size_t clap::HelpFormatter::name_width() const {
    size_t max_w = 0;
    for (const auto* list : {&_options, &_positionals})
        for (const auto& a : *list) {
            const size_t w = name_col(*a).size() + 2;
            if (w <= _name_max)
                max_w = std::max(max_w, w);
        }
    return max_w == 0 ? _name_max : max_w;
}

inline size_t clap::HelpFormatter::desc_column() const {
    const size_t name_w = name_width();

    size_t max_w = 0;
    for (const auto* list : {&_options, &_positionals})
        for (const auto& a : *list) {
            const size_t w = prefix_col(*a, name_w).size() + 2;
            if (w <= _desc_max)
                max_w = std::max(max_w, w);
        }
    return max_w == 0 ? _desc_max : max_w;
}

inline std::string clap::HelpFormatter::prefix_col(const clap::Argument& arg,
                                                   size_t name_w) const {
    std::string prefix = name_col(arg);
    const std::string type = type_col(arg);
    if (type.empty())
        return prefix;
    if (prefix.size() < name_w)
        prefix.resize(name_w, ' ');
    else
        prefix += "  ";
    return prefix + type;
}

inline std::vector<std::string> clap::HelpFormatter::wrap(const std::string& text,
                                                          size_t width) const {
    std::vector<std::string> lines;
    std::istringstream paragraphs(text);
    std::string paragraph;

    while (std::getline(paragraphs, paragraph)) {
        std::istringstream words(paragraph);
        std::string word, line;

        while (words >> word) {
            if (line.empty())
                line = word;
            else if (line.size() + 1 + word.size() <= width)
                line += " " + word;
            else {
                lines.push_back(line);
                line = word;
            }
        }
        lines.push_back(line);
    }
    if (lines.empty())
        lines.push_back("");
    return lines;
}

inline std::string clap::HelpFormatter::row(const clap::Argument& arg, size_t name_w,
                                            size_t desc_col) const {
    std::string prefix = prefix_col(arg, name_w);

    static_assert(_desc_max < _line_width, "the description column must leave room");
    const size_t width = _line_width - desc_col;
    std::vector<std::string> lines = wrap(std::string(arg.description()), width);
    append_units(lines, annotation(arg), width);

    std::ostringstream oss;
    if (prefix.size() + 2 <= desc_col) {
        prefix.resize(desc_col, ' ');
        oss << prefix << lines.front() << "\n";
    } else {
        oss << prefix << "\n" << std::string(desc_col, ' ') << lines.front() << "\n";
    }
    for (size_t i = 1; i < lines.size(); ++i)
        oss << std::string(desc_col, ' ') << lines[i] << "\n";
    return oss.str();
}

inline std::string clap::HelpFormatter::table(const std::vector<const Argument*>& args,
                                              size_t name_w, size_t desc_col) const {
    std::ostringstream oss;
    for (const auto& a : args)
        oss << row(*a, name_w, desc_col);
    return oss.str();
}

inline std::vector<std::string> clap::HelpFormatter::usage_tokens(bool collapse) const {
    std::vector<std::string> tokens;
    if (collapse)
        tokens.push_back("[OPTIONS]");
    for (const auto& a : _options)
        if (!collapse || a->is_required())
            tokens.push_back(usage_token(*a, false));
    for (const auto& p : _positionals)
        tokens.push_back(usage_token(*p, true));
    return tokens;
}

inline std::vector<std::string> clap::HelpFormatter::pack(
    const std::vector<std::string>& tokens, const std::string& head, size_t indent) const {
    std::vector<std::string> lines;
    std::string line = head;
    for (const auto& token : tokens) {
        if (line.size() + 1 + token.size() <= _line_width)
            line += " " + token;
        else {
            lines.push_back(line);
            line = std::string(indent, ' ') + token;
        }
    }
    lines.push_back(line);
    return lines;
}

inline std::string clap::HelpFormatter::usage() const {
    const std::string head = "Usage: " + std::string(_name);
    const size_t indent =
        head.size() + 1 <= _usage_indent_max ? head.size() + 1 : std::strlen("Usage: ");

    std::vector<std::string> lines = pack(usage_tokens(false), head, indent);
    if (lines.size() > _usage_max_lines)
        lines = pack(usage_tokens(true), head, indent);

    std::ostringstream oss;
    for (size_t i = 0; i < lines.size(); ++i)
        oss << (i ? "\n" : "") << lines[i];
    return oss.str();
}

inline std::string clap::HelpFormatter::help() const {
    std::ostringstream oss;
    oss << usage() << "\n\n";
    for (const auto& line : wrap(std::string(_description), _line_width))
        oss << line << "\n";

    const size_t name_w = name_width();
    const size_t desc_col = desc_column();

    if (!_positionals.empty())
        oss << "\nPOSITIONALS:\n" << table(_positionals, name_w, desc_col);

    if (!_ungrouped.empty())
        oss << "\nOPTIONS:\n" << table(_ungrouped, name_w, desc_col);

    for (const auto& [group, args] : _groups)
        oss << "\n" << group << ":\n" << table(args, name_w, desc_col);

    if (!_examples.empty()) {
        oss << "\nEXAMPLES:\n";
        bool first = true;
        for (const auto& [example, description] : _examples) {
            if (!description.empty()) {
                if (!first)
                    oss << "\n";
                for (const auto& line : wrap(description, _line_width - 4))
                    oss << "  # " << line << "\n";
            }
            oss << "  > " << example << "\n";
            first = false;
        }
    }
    if (!_footer.empty()) {
        oss << "\n";
        if (_footer_wrap)
            for (const auto& line : wrap(std::string(_footer), _line_width))
                oss << line << "\n";
        else
            oss << _footer << "\n";
    }

    return oss.str();
}
