#include "regex.hpp"

int main()
{
    auto A = rgx::Regex("a*");
    A.writeNFA("graphviz/DFA.txt");
    A.search("aaaaa");
    return 0;
}
