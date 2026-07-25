#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace clap {
    /// Internals. Not part of the public API; may change without notice.
    namespace detail {
        /// Edit distance between s1 and s2, counting an adjacent swap as one
        /// edit instead of two substitutions.
        int damerau_osa(const std::string& s1, const std::string& s2);

        /// The candidate closest to unknown, or "" when nothing is close enough
        /// to be worth printing. Leading dashes are ignored when comparing.
        std::string suggest(std::string_view unknown,
                            const std::vector<std::string>& candidates);
    }
}
