#include "regex.hpp"

int main()
{
	// a*bbbbcccccc+*
    rgx::Regex<> A("(a\\*b*)*c*\\+\\*");
    // rgx::Regex<> A("(a*b*)*c");
    // rgx::Regex<> A("(ab*)*c*\\*");
    std::cout << A.match("a*bbbbcccccc+***aa");

    std::cout << A.search("a*bbbbcccccc+***aaa*bbbbcccccc+***aa");
    // A.search("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    return 0;
}
