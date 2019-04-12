#include "regex.hpp"

int main()
{
    auto A = rgx::Regex("(AT|GA)((AG|AAA)*)");
    A.writeNFA("graphviz/DFA.txt");
    return 0;
}
