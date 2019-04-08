#include<fstream>
#include<iostream>
#include "regex.hpp"

int main()
{
    auto A = rgx::Regex("(0|(1(01*0)*1))*");
    for(int i = 0; i <= 30; ++i)
    {
        std::bitset <MAX_CHAR> a(i);
        std::cout<<i<<". "<<a<<" - ";
        A.checkWord(a.to_string());
        std::cout<<std::endl;
    }
    //A.checkWord("");
    return 0;
}
