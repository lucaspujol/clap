#include "damerau_osa.hpp"
#include <algorithm>
#include <vector>

// Names shorter than this are not compared: at one or two characters every
// name is a near-miss for every other, so the suggestion would be noise.
namespace clap::detail {
inline constexpr size_t MIN_SUGGEST_LEN = 3;

inline std::string_view strip_dashes(std::string_view s) {
    size_t i = 0;
    while (i < s.size() && s[i] == '-') i++;
    return s.substr(i);
}
}

// levenshtein distance implementation. adds damerau transposition
// so mistakes like (--forec instead of --force) gives a smaller cost
inline int clap::detail::damerau_osa(const std::string& s1, const std::string& s2) {
    std::vector<std::vector<int>> d(s1.size() + 1, std::vector<int>(s2.size() + 1));
    size_t i, j;
    int cost = 0;

    // init d
    for (i = 0; i < s1.size() + 1; i++) d[i][0] = static_cast<int>(i);
    for (j = 0; j < s2.size() + 1; j++) d[0][j] = static_cast<int>(j);

    for (i = 1; i < s1.size() + 1; i++) {
        for (j = 1; j < s2.size() + 1; j++) {
            cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
            d[i][j] = std::min({
                d[i - 1][j    ] + 1,
                d[i    ][j - 1] + 1,
                d[i - 1][j - 1] + cost
            });
            // transposition (osa)
            if (i > 1 && j > 1 && s1[i - 1] == s2[j - 2] && s1[i - 2] == s2[j - 1])
                d[i][j] = std::min(d[i][j], d[i - 2][j - 2] + cost);
        }
    }
    return d[s1.size()][s2.size()];
}

inline std::string clap::detail::suggest(std::string_view unknown,
                                  const std::vector<std::string>& candidates) {
    std::string input(clap::detail::strip_dashes(unknown));
    if (input.size() < clap::detail::MIN_SUGGEST_LEN)
        return "";

    const std::string* best = nullptr;
    int best_dist = 0;

    for (const std::string& candidate : candidates) {
        std::string name(clap::detail::strip_dashes(candidate));
        if (name.size() < clap::detail::MIN_SUGGEST_LEN)
            continue;
        int dist = clap::detail::damerau_osa(input, name);
        if (!best || dist < best_dist) {
            best_dist = dist;
            best = &candidate;
        }
    }

    if (!best)
        return "";

    // One edit tolerated per three characters of the longer name, so short
    // names stay strict and long ones do not demand a perfect guess.
    size_t len = std::max(input.size(), clap::detail::strip_dashes(*best).size());
    int max_dist = std::max(static_cast<int>(len) / 3, 1);

    return best_dist <= max_dist ? *best : "";
}
