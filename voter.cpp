#include "voter.hpp"
#include <vector>
#include <iostream>


int main(void)
{
    Voter<std::vector<int>, 2> vint;
    std::vector<int> v;
    v.push_back(2);
    v.push_back(2);
    v.push_back(9);
    v.push_back(9);
    v.push_back(9);
    bool rv = vint(v);
    std::cout << rv << std::endl;
    std::cout << v[2] << std::endl;
}
