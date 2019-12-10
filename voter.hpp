#ifndef _VOTER_HPP
#define _VOTER_HPP
#include <array>
#include <stdexcept>

// Container has Random Access Iterator
template <typename Container, int FT>
struct Voter {
    static_assert(__cplusplus == 201703, "Requires -std=c++17 compile option");
    constexpr Voter() = default;

    bool operator() (Container & c) const
    {
        static constexpr bool rv = true;
        if constexpr (0 == FT)
            { goto done; }

        else if constexpr (FT) {
            using C = typename Container::value_type;
            std::array<C, 2*FT+1> vals;
            std::array<size_t, 2*FT+1> counts;
            size_t insert_pos = 0;
            for (const auto &cit : c) {
                for (size_t vit = 0; vit != insert_pos; vit++) {
                    if (cit == vals[vit]) {
                        counts[vit] += 1;
                        goto o_loop_next;
                    }
                }
                vals[insert_pos] = cit;
                counts[insert_pos] = 1;
                ++insert_pos;
            o_loop_next: ;
            }

            if (vals.size() == 1) // Terminate early on full agreement
                { goto done; }

            for (size_t count_it = 0; count_it != insert_pos; ++count_it) {
                if (counts[count_it] > FT) {
                    for (auto &cit : c) {
                        cit = vals[count_it];
                    }
                    goto done;
                }
            }

            throw std::runtime_error("no consensus found");
        }
done:
        return rv;
    }
};
#endif
