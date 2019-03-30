#pragma once
#include<cstdint>
namespace rgx
{

class Regex{
    std::string infixExp_;
    std::string postfixExp_;
    //DFA automaton_;
    std::string postfix(std::string infixExp)
    {
        std::string postfixExp;
        return postfixExp;
    }
public:
    Regex(const std::string &infixExp)
    {
        infixExp_ = infixExp;
        postfixExp_ = postfix(infixExp);
    }
    std::string getInfix()
    {
        return infixExp_;
    }
    std::string getPostfix()
    {
        return postfixExp_;
    }
};

}
