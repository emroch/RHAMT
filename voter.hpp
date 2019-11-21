#include <typeinfo>
#include <type_traits>
#include <iterator>
#include <iostream>
#include <vector>

// Container has Random Access Iterator
template <typename Container, int IF>
struct Voter {
    bool operator() (Container & c)
    {
        using C = typename Container::value_type;
        std::vector<C> vals;
        std::vector<int> counts;
        bool rv = true;

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

        for (auto count_it = counts.begin(); count_it != counts.end();
                                             ++count_it) {
            if (*count_it == c.size()) {
                goto done;
            }
            else if (*count_it > IF) {
                for (auto &cit : c) {
                    cit = vals[count_it - counts.begin()];
                }
                goto done;
            }
        }

        rv = false;
done:
        return rv;
    }
};
