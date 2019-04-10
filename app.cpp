#include<fstream>
#include<iostream>
#include "regex.hpp"

void f(int **p)
{

}

int main()
{
    auto A = rgx::Regex("k*|lm");
    /*for(int i = 0; i <= 30; ++i)
    {
        std::bitset <MAX_CHAR> a(i);
        std::cout<<i<<". "<<a<<" - ";
        A.checkWord(a.to_string());
        std::cout<<std::endl;
    }*/
    A.checkWord("");
    return 0;
}
