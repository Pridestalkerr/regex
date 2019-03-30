#include<fstream>
#include<iostream>
#include "regex.hpp"

int main()
{
    auto A = rgx::Regex("a|b");
    std::cout << A.getExp();
    return 0;
}
