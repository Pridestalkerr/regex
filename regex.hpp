#pragma once
#include<cstdint>
#include<stack>
#include<map>
#include<vector>
#include<set>
#include<bitset>
#include<cmath>
#include<queue>
#include<fstream>
#include<iostream>
#include<tuple>
#include<list>

#define UNION '|'
#define STAR '*'
#define CONCAT '+'
#define WILDCARD '.'
#define OPEN_PARENTHESIS '('
#define CLOSE_PARENTHESIS ')'
#define MAX_CHAR 16 //-1

namespace rgx
{
class Regex{
    std::string infixExp_;
    std::string postfixExp_;
    std::string postfixIndexes_; //rename to transitions (or something similar?)
    std::bitset <MAX_CHAR> first_; //merge with follow[0]
    std::bitset <MAX_CHAR> last_; //rename to final
    std::vector <std::bitset <MAX_CHAR>> follow_;
    std::string postfix(const std::string &infixExp)
    {
        std::string postfixExp;
        std::stack <char> conversionStack;
        for(auto chr: infixExp)
        {
            if(isCharacter(chr))
            {
                postfixExp.push_back(chr);
                continue;
            }
            if(chr == OPEN_PARENTHESIS)
            {
                conversionStack.push(chr);
                continue;
            }
            if(isOperator(chr))
            {
                while(operatorPrecedence(conversionStack.top()) >= operatorPrecedence(chr))
                {
                    postfixExp.push_back(conversionStack.top());
                    conversionStack.pop();
                }
                conversionStack.push(chr);
                continue;
            }
            while(conversionStack.top() != OPEN_PARENTHESIS)
            {
                postfixExp.push_back(conversionStack.top());
                conversionStack.pop();
            }
            conversionStack.pop();
        }
        return postfixExp;
    }
    std::string processConcat(const std::string &infixExp)
    {
        std::string processedExp;
        processedExp.push_back(OPEN_PARENTHESIS);
        for(auto chr: infixExp)
        {
            if(detectConcat(processedExp.back(), chr))
                processedExp.push_back(CONCAT);
            processedExp.push_back(chr);
        }
        processedExp.push_back(CLOSE_PARENTHESIS);
        return processedExp;
    }
    void computeGlushkovSets(const std::string &postfixExp, std::bitset <MAX_CHAR> &first, std::bitset <MAX_CHAR> &last, std::vector <std::bitset <MAX_CHAR>> &follow)
    {
        std::stack <std::bitset <MAX_CHAR>> firstStack;
        std::stack <std::bitset <MAX_CHAR>> lastStack;
        std::stack <std::bitset <MAX_CHAR>> posStack; //indexes that are contained within the stack expression
        std::vector <std::bitset <MAX_CHAR>> followSet;
        followSet.push_back(0);
        std::size_t itr = 1; //this will be our index
        postfixIndexes_.push_back('a');
        for(auto chr: postfixExp)
        {
            if(isCharacter(chr))
            {
                postfixIndexes_.push_back(chr);
                std::bitset <MAX_CHAR> bitMask;
                bitMask.set(itr, true);
                firstStack.push(bitMask); //First(x) = {x}
                lastStack.push(bitMask); //Last(x) = {x}
                posStack.push(bitMask);

                //we push the empty bit to its follow() since they come in order
                followSet.push_back(0);

                itr++;
            }
            else
            {
                if(chr == STAR)
                {
                    firstStack.top()[0] = true; //First(E*) = First(E+) = First(E)
                    lastStack.top()[0] = true; //Last(E*) = Last(E+) = Last(E)
                    //we also keep track whether epsilon belongs to the current subsequence of RE using the 0th byte

                    //follow
                    for(std::size_t itr = 1; itr < MAX_CHAR; ++itr)
                        if(posStack.top().test(itr) && lastStack.top().test(itr))
                            followSet[itr] |= firstStack.top();
                }
                if(chr == UNION)
                {
                    //First(F | G) = First(F) U First(G)
                    std::bitset <MAX_CHAR> bitMask = firstStack.top();
                    firstStack.pop();
                    firstStack.top() |= bitMask;

                    //Last(F | G) = Last(F) U Last(G)
                    bitMask = lastStack.top();
                    lastStack.pop();
                    lastStack.top() |= bitMask;

                    //positions united
                    bitMask = posStack.top();
                    posStack.pop();
                    posStack.top() |= bitMask;

                    //nothing changes for follow
                }
                if(chr == CONCAT)
                {
                    //First(F + G) = First(F) if epsilon DOES NOT belong to L(F); First(F) U First(G) if epsilon belongs to L(F)
                    std::bitset <MAX_CHAR> bitMask = firstStack.top();
                    auto firstG = bitMask;
                    firstStack.pop();
                    if(firstStack.top().test(0))
                    {
                        if(!bitMask.test(0))
                            firstStack.top().set(0, false);
                        firstStack.top() |= bitMask;
                    }

                    //Last(F + G) = Last(F) if epsilon DOES NOT belong to L(F); Last(F) U Last(G) if epsilon belongs to L(F)
                    bitMask = lastStack.top();
                    lastStack.pop();
                    auto lastF = lastStack.top();
                    if(bitMask.test(0))
                    {
                        if(!lastStack.top().test(0))
                            bitMask.set(0, false);
                        lastStack.top() |= bitMask;
                    }
                    else
                    {
                        lastStack.pop();
                        lastStack.push(bitMask);
                    }

                    //positions concatenated (no difference)
                    bitMask = posStack.top();
                    posStack.pop();
                    posStack.top() |= bitMask;

                    //follow
                    for(std::size_t itr = 1; itr < MAX_CHAR; ++itr)
                        if(posStack.top().test(itr) && lastF.test(itr))
                            followSet[itr] |= firstG;
                }
            }
        }
        first = firstStack.top();
        last = lastStack.top();
        follow = followSet;
    }
    //consider moving outside class
    uint8_t operatorPrecedence(const char &chr)
    {
        if(chr == STAR)
            return 3;
        if(chr == CONCAT)
            return 2;
        if(chr == UNION)
            return 1;
        return 0;
    }
    bool detectConcat(const char &chr1, const char &chr2)
    {
        if((isCharacter(chr1) || chr1 == CLOSE_PARENTHESIS || chr1 == STAR) && (isCharacter(chr2) || chr2 == OPEN_PARENTHESIS))
            return true;
        return false;
    }
    bool isCharacter(const char &chr)
    {
        if(isOperator(chr) || isBracket(chr))
            return false;
        return true;
    }
    bool isOperator(const char &chr)
    {
        if(chr == UNION || chr == STAR || chr == CONCAT)
            return true;
        return false;
    }
    bool isBracket(const char &chr)
    {
        if(chr == OPEN_PARENTHESIS || chr == CLOSE_PARENTHESIS)
            return true;
        return false;
    }
    void makeDFA(std::bitset <MAX_CHAR> first, std::bitset <MAX_CHAR> last, std::vector <std::bitset <MAX_CHAR>> follow, std::string postfixIndexes)
    {
        /*NFA <std::bitset <MAX_CHAR>, char> table;
        std::queue <std::bitset <MAX_CHAR>> states;
        states.push(first);
        //check for epsilon please.
        while(!states.empty())
        {
            auto currentState = states.front();
            states.pop();
            std::map <char, std::bitset <MAX_CHAR>> newEntry;
            for(std::size_t itr = 1; itr < currentState.size(); ++itr)
            {
                if(currentState.test(itr))
                {
                    auto symbolItr = newEntry.find(postfixIndexes[itr]);
                    if(symbolItr != newEntry.end())
                    {
                        symbolItr->second |= follow[itr];
                    }
                    else
                    {
                        newEntry.insert({postfixIndexes[itr], follow[itr]});
                    }
                }
            }
        }*/
    }
public:
    Regex(const std::string &infixExp)
    {
        infixExp_ = infixExp;
        postfixExp_ = postfix(processConcat(infixExp));
        computeGlushkovSets(postfixExp_, first_, last_, follow_);
        makeDFA(first_, last_, follow_, postfixIndexes_);
    }
    void checkWord(const std::string &word)
    {
        auto stateSimulator = first_;
        bool isFinal = false;
        if(first_.test(0))
            isFinal = true;
        for(auto chr: word)
        {
            std::bitset <MAX_CHAR> transition;
            isFinal = false;
            for(std::size_t itr = 1; itr < stateSimulator.size(); ++itr)
            {
                if(stateSimulator.test(itr) && postfixIndexes_[itr] == chr)
                {
                    transition |= follow_[itr];
                    if(last_[itr])
                        isFinal = true;
                }
            }
            stateSimulator = transition;
        }
        if(isFinal)
            std::cout<<"OK";
        else
            std::cout<<"NO";
    }
    void search(const std::string &sample)
    {
        //empty words will be ignored
        /*std::list <std::tuple <std::bitset <MAX_CHAR>, std::size_t, std::size_t, bool>> matches; //transition, begin, begin+size
        for(std::size_t itr = 0; itr < sample.size(); )
        {
            matches.push_back({first_, itr, 0});
            for(auto itr: matches)
            {
                if()
            }

        }*/
    }
    std::string getInfix()
    {
        return infixExp_;
    }
    std::string getPostfix()
    {
        return postfixExp_;
    }
    void writeNFA(std::string fileName)
    {
        std::ofstream file(fileName);
        for(std::size_t itr = 0; itr < MAX_CHAR; ++itr)
            if(last_.test(itr))
                file << itr << ' ';
        file << std::endl;
        follow_[0] = first_;
        for(std::size_t itr = 0; itr < follow_.size(); ++itr)
            for(std::size_t jtr = 1; jtr < MAX_CHAR; ++jtr)
                if(follow_[itr].test(jtr))
                    file << itr << ' ' << postfixIndexes_[jtr] << ' ' << jtr << std::endl;
    }
};
}
