#ifndef _VOTER_HPP
#define _VOTER_HPP
#include <vector>

#if __cplusplus == 201703L
#define constif if constexpr
#else
#define constif if
#endif

// Container has Random Access Iterator
template <typename Container, int FT>
struct Voter {
    constexpr Voter()  = default;
    bool operator() (Container & c) const
    {
        using C = typename Container::value_type;
        std::vector<C> vals;
        std::vector<int> counts;
        bool rv = true;
        // If we have no reliability guarantees, simply return true

        constif (0 == FT)
            { goto done; }

        else constif (FT) {
            for (const auto &cit : c) {
                for (auto vit = vals.cbegin(); vit != vals.cend(); vit++) {
                    if (cit == *vit) {
                        counts[vit - vals.cbegin()] += 1;
                        goto o_loop_next;
                    }
                }
                vals.push_back(cit);
                counts.push_back(1);
    o_loop_next: ;
            }

            // Terminate early if there is full agreement
            if (vals.size() == 1)
                { goto done; }

            for (auto count_it = counts.begin(); count_it != counts.end();
                                                 ++count_it) {
                if (*count_it == c.size()) {
                    goto done;
                }
                else if (*count_it > FT) {
                    for (auto &cit : c) {
                        cit = vals[count_it - counts.begin()];
                    }
                    goto done;
                }
            }

            rv = false;
            // Fuck it
            throw "Unrecoverable Failure Exception";
        }
done:
        return rv;
    }
};
#undef constif
#endif
