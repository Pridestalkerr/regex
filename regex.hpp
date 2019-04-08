#pragma once
#include<cstdint>
#include<stack>
#include<map>
#include<vector>
#include<set>
#include<bitset>
#include<cmath>
#include<queue>

#define UNION '|'
#define STAR '*'
#define CONCAT '+'
#define WILDCARD '.'
#define OPEN_PARENTHESIS '('
#define CLOSE_PARENTHESIS ')'
#define EPSILON '~'
#define MAX_CHAR 16 //-1

namespace rgx
{
template <class StateType, class SymbolType>
class NFA{
    std::map <StateType, std::map <SymbolType, std::set <StateType>>> table_;
    StateType initialState_;
    std::set <StateType> finalStates_;
    std::set <SymbolType> symbols_;
public:
    void insertFinalState(const StateType &finalState)
    {
        finalStates_.insert(finalState);
    }
    void setInitialState(const StateType &initialState)
    {
        initialState_ = initialState;
    }
    void insertTrasition(const StateType &state1, const SymbolType &symbol, const StateType &state2)
    {
        symbols_.insert(symbol);
        auto stateItr = table_.find(state1);
        if(stateItr != table_.end())
        {
            auto symbolItr = stateItr->second.find(symbol);
            if(symbolItr != stateItr->second.end())
                symbolItr->second.insert(state2);
            else
                table_->second.insert({symbol, { {state2} }});
        }
        else
            table_.insert({state1, { {symbol, { {state2} }} }});
    }
};

class Regex{
    std::string infixExp_;
    std::string postfixExp_;
    std::string postfixIndexes_;
    NFA <uint32_t, char> automaton_;
    std::bitset <MAX_CHAR> first_;
    std::bitset <MAX_CHAR> last_;
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
    std::string convertToNFA(std::string postfixExp)
    {
        //NFA convertedExp;

        //return convertedExp;
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
        std::vector <std::bitset <MAX_CHAR>> followSet {0};
        uint16_t itr = 1; //this will be our index
        postfixIndexes_.push_back('a');
        for(auto chr: postfixExp)
        {
            if(isCharacter(chr))
            {
                postfixIndexes_.push_back(chr);
                std::bitset <MAX_CHAR> bitMask;
                bitMask[itr] = true;
                firstStack.push(bitMask); //First(x) = {x}
                lastStack.push(bitMask); //Last(x) = {x}
                posStack.push(bitMask);

                //we push the empty bit to its follow()
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
                    for(int itr = 1; itr < MAX_CHAR; ++itr)
                        if(posStack.top().test(itr))
                            if(lastStack.top().test(itr))
                                followSet[itr] |= firstStack.top();
                }
                if(chr == UNION)
                {
                    //First(F | G) = First(F) U First(G)
                    std::bitset <MAX_CHAR> bitMask = firstStack.top();
                    firstStack.pop();
                    bitMask |= firstStack.top();
                    firstStack.pop();
                    firstStack.push(bitMask);

                    //Last(F | G) = Last(F) U Last(G)
                    bitMask = lastStack.top();
                    lastStack.pop();
                    bitMask |= lastStack.top();
                    lastStack.pop();
                    lastStack.push(bitMask);

                    //positions united
                    bitMask = posStack.top();
                    posStack.pop();
                    bitMask |= posStack.top();
                    posStack.pop();
                    posStack.push(bitMask);

                    //nothing changes for follow
                }
                if(chr == CONCAT)
                {
                    //First(F + G) = First(F) if epsilon DOES NOT belong to L(F); First(F) U First(G) if epsilon belongs to L(F)
                    std::bitset <MAX_CHAR> bitMask = firstStack.top();
                    auto firstG = bitMask;
                    firstStack.pop();
                    if(firstStack.top()[0] == true)
                    {
                        if(bitMask[0] == false)
                            firstStack.top()[0] = false;
                        bitMask |= firstStack.top();
                        firstStack.pop();
                        firstStack.push(bitMask);
                    }

                    //Last(F + G) = Last(F) if epsilon DOES NOT belong to L(F); Last(F) U Last(G) if epsilon belongs to L(F)
                    bitMask = lastStack.top();
                    lastStack.pop();
                    auto lastF = lastStack.top();
                    if(bitMask[0] == true)
                    {
                        if(lastStack.top()[0] == false)
                            bitMask[0] = false;
                        bitMask |= lastStack.top();
                        lastStack.pop();
                        lastStack.push(bitMask);
                    }
                    else
                    {
                        lastStack.pop();
                        lastStack.push(bitMask);
                    }

                    //positions concatenated (no difference)
                    bitMask = posStack.top();
                    posStack.pop();
                    bitMask |= posStack.top();
                    posStack.pop();
                    posStack.push(bitMask);

                    //follow
                    for(int itr = 1; itr < MAX_CHAR; ++itr)
                        if(posStack.top().test(itr))
                            if(lastF.test(itr))
                                followSet[itr] |= firstG;

                }
            }
        }
        first_ = firstStack.top();
        last_ = lastStack.top();
        follow_= followSet;
        //std::cout << firstStack.size();
        /*std::cout << firstStack.top()<<std::endl;
        std::cout << lastStack.top()<<std::endl;
        for(auto i: postfixIndexes_)
            std::cout<<i<<' ';
        std::cout<<std::endl;
        for(auto i: followSet)
            std::cout<<i<<std::endl;*/

    }
    uint8_t operatorPrecedence(const char &chr)
    {
        if(chr == STAR)
            return 10;
        if(chr == UNION)
            return 2;
        if(chr == CONCAT)
            return 5;
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
    void buildT()
    {
        /*std::vector <std::bitset <MAX_CHAR>> T;
        T.push_back(0);
        for(int i=0; i<=9;++i)
            for(int j=0; j<=pow(2,i)-1; ++j)
                T.push_back(follow_[i] | T[j]);
        std::cout<<std::endl<<T[4];*/
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
        //postfixExp_ = processInfixExp(infixExp);
        std::cout<<processConcat(infixExp);
        postfixExp_ = postfix(processConcat(infixExp));
        //convertToNFA(postfixExp_);
        computeGlushkovSets(postfixExp_, first_, last_, follow_);
        makeDFA(first_, last_, follow_, postfixIndexes_);
    }
    void checkWord(const std::string &word)
    {
        auto stateSimualtor = first_;
        bool isFinal = false;
        if(first_.test(0))
            isFinal = true;
        for(auto chr: word)
        {
            std::bitset <MAX_CHAR> transition;
            isFinal = false;
            for(std::size_t itr = 1; itr < stateSimualtor.size(); ++itr)
            {
                if(stateSimualtor.test(itr) && postfixIndexes_[itr] == chr)
                {
                    transition |= follow_[itr];
                    if(last_[itr])
                        isFinal = true;
                }
            }
            stateSimualtor = transition;
        }
        if(isFinal)
            std::cout<<"OK";
        else
            std::cout<<"NO";
    }
    void match(const std::string &sample)
    {

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
