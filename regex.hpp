#pragma once

#include <vector>
#include <bitset>
#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include <locale>
#include <stdexcept>
#include <stack>
#include <cstdint>
#include <list>
#include <algorithm>

#include "dynamic_bitset.hpp"



namespace rgx
{



template <typename String_T = std::string>
class Regex
{
    using Char_T        = typename String_T::value_type;
    using Traits_T      = typename String_T::traits_type;
    using Alloc_T       = typename String_T::allocator_type;
    using Exp_value_T   = typename std::pair <Char_T, uint8_t>;
    using Exp_T         = typename std::vector <Exp_value_T>;
    using State_T       = typename bit::Bitset <uint64_t>;
    using NFA_T         = typename std::vector <State_T>;


    static constexpr const uint8_t CHAR_VAL     = 0;

    static constexpr const uint8_t UNION_VAL    = 1;
    static constexpr const uint8_t STAR_VAL     = 2;
    static constexpr const uint8_t PLUS_VAL     = 3;
    static constexpr const uint8_t CONCAT_VAL   = 4;

    static constexpr const uint8_t OPEN_VAL     = 5;
    static constexpr const uint8_t CLOSE_VAL    = 6;

    static constexpr const uint8_t ESCAPE_VAL   = 7;


    const std::ctype <Char_T> &facet = std::use_facet <std::ctype <Char_T>>(
        std::locale()
    );
    const Char_T UNION   = facet.widen('|');
    const Char_T STAR    = facet.widen('*');
    const Char_T PLUS    = facet.widen('+');
    const Char_T CONCAT  = facet.widen('.');
    const Char_T OPEN    = facet.widen('(');
    const Char_T CLOSE   = facet.widen(')');
    const Char_T ESCAPE  = facet.widen('\\');


private:


    NFA_T m_NFA;

    std::size_t m_size;

    Exp_T m_transition_labels;

    const Exp_T m_postfix;


    Exp_T process_escape(const String_T&) const;
    Exp_T process_concat(const Exp_T&) const;

    Exp_T process_exp(const String_T&) const;
    Exp_T process_postfix(const Exp_T&);

    void compute_NFA(const Exp_T&);

    //helpers
    bool is_operator(const Char_T) const noexcept;
    bool is_bracket(const Char_T) const noexcept;
    bool is_character(const Char_T) const noexcept;
    bool detect_concat(const Char_T, const Char_T) const noexcept;
    uint8_t operator_precedence(const Char_T) const noexcept;

    static constexpr bool is_operator(const Exp_value_T) noexcept;
    static constexpr bool is_bracket(const Exp_value_T) noexcept;
    static constexpr bool is_character(const Exp_value_T) noexcept;
    static constexpr bool detect_concat(const Exp_value_T, const Exp_value_T) noexcept;
    static constexpr uint8_t operator_precedence(const Exp_value_T) noexcept;


    uint8_t op_value(const Char_T) const noexcept;


public:

    Regex(const String_T &);


    bool match(const String_T &word);

    bool search(const String_T &sample);

    bool search_results(const String_T &sample, const std::size_t max_res) const;

};



template <typename String_T>
Regex <String_T>::Regex(const String_T &exp)
    :
    m_postfix(
        process_postfix(
            process_exp(exp)
        )
    )
{   
    compute_NFA(m_postfix);
    std::for_each(m_transition_labels.begin(), m_transition_labels.end(), [](auto i)->void {std::cout << i.first;});
};


// returns processed exp, ready for postfixing
template <typename String_T>
typename Regex<String_T>::Exp_T
Regex<String_T>::process_exp(const String_T &exp) const
{

    // std::cout << exp;
    // std::cout << std::endl;

    Exp_T processed_exp;
    processed_exp.reserve(exp.size() * 2 + 2);

    processed_exp.push_back({OPEN, OPEN_VAL});

    std::size_t itr = 0;
    for(itr; itr < exp.size() - 1; ++itr)
    {
        // escape characters
        const Exp_value_T curr_ch = (exp[itr] == ESCAPE)
            ? Exp_value_T{exp[++itr], CHAR_VAL}
            : Exp_value_T{exp[itr], op_value(exp[itr])};

        // Exp_value_T curr_ch;
        // if(exp[itr] == ESCAPE)
        // {
        //     ++itr;
        //     curr_ch = Exp_value_T{exp[itr], CHAR_VA L}
        // }
        // else
        // {
        //     curr_ch = Exp_value_T{exp[itr], op_value(exp[itr])}
        // }

        // add concat operators
        if(detect_concat(processed_exp.back(), curr_ch))
        {
            processed_exp.push_back({CONCAT, CONCAT_VAL});
        }

        processed_exp.push_back(curr_ch);
    }

    if(itr < exp.size())
    {
        // add concat operators
        if(detect_concat(processed_exp.back().first, exp[itr]))
        {
            processed_exp.push_back({CONCAT, CONCAT_VAL});
        }

        processed_exp.push_back({exp[itr], op_value(exp[itr])});
    }

    processed_exp.push_back({CLOSE, CLOSE_VAL});

    // processed_exp.shrink_to_fit();
    return std::move(processed_exp);
};


template <typename String_T>
typename Regex<String_T>::Exp_T
Regex<String_T>::process_escape(const String_T &exp) const
{

    // std::cout << exp;
    // std::cout << std::endl;

    Exp_T processed_exp;
    processed_exp.reserve(exp.size());

    std::size_t itr = 0;
    for(itr; itr < exp.size() - 1; ++itr)
    {
        if(exp[itr] == ESCAPE)
        {
            processed_exp.push_back({exp[++itr], CHAR_VAL});
        }
        else
        {
            processed_exp.push_back({exp[itr], op_value(exp[itr])});
        }
    }

    if(itr < exp.size())
    {
        processed_exp.push_back({exp[itr], op_value(exp[itr])});
    }

    processed_exp.shrink_to_fit();
    return std::move(processed_exp);
};


template <typename String_T>
typename Regex<String_T>::Exp_T
Regex<String_T>::process_concat(const Exp_T &exp) const
{

    // std::for_each(exp.begin(), exp.end(), [](auto i)->void {std::cout << i.first;});
    // std::for_each(exp.begin(), exp.end(), [](auto i)->void {std::cout << (unsigned int)i.second << ' ';});
    // std::cout << std::endl;

    Exp_T processed_exp;
    processed_exp.reserve(exp.size() * 2 + 2);

    processed_exp.push_back({OPEN, OPEN_VAL});

    for(const Exp_value_T chr : exp)
    {
        if(detect_concat(processed_exp.back(), chr))
        {
            processed_exp.push_back({CONCAT, CONCAT_VAL});
        }

        processed_exp.push_back(chr);
    }

    processed_exp.push_back({CLOSE, CLOSE_VAL});

    processed_exp.shrink_to_fit();
    return std::move(processed_exp);
};


// calculates nr of states
template <typename String_T>
typename Regex<String_T>::Exp_T
Regex <String_T>::process_postfix(const Exp_T &exp)
{
    m_size = 0;

    // std::for_each(exp.begin(), exp.end(), [](auto i)->void {std::cout << i.first;});
    // std::for_each(exp.begin(), exp.end(), [](auto i)->void {std::cout << (unsigned int)i.second << ' ';});
    // std::cout << std::endl;

    Exp_T processed_exp;
    processed_exp.reserve(exp.size());

    // a fixed size stack would help,
    // repeated allocations are still expensive
    // also leads to caching struggles
    std::stack <Exp_value_T> conversion_stack;

    for(const Exp_value_T chr : exp)
    {
        if(is_character(chr))
        {
            processed_exp.push_back(chr);

            ++m_size;    // increment number of states

            continue;
        }

        if(chr.second == OPEN_VAL)
        {
            conversion_stack.push(chr);

            continue;
        }

        if(is_operator(chr))
        {
            while(operator_precedence(conversion_stack.top()) >= operator_precedence(chr))
            {
                processed_exp.push_back(conversion_stack.top());
                conversion_stack.pop();
            }

            conversion_stack.push(chr);

            continue;
        }

        while(conversion_stack.top().second != OPEN_VAL)
        {
            processed_exp.push_back(conversion_stack.top());
            conversion_stack.pop();
        }

        conversion_stack.pop();
    }

    // processed_exp.shrink_to_fit();
    return std::move(processed_exp);
};




template <typename String_T>
void Regex <String_T>::compute_NFA(const Exp_T &exp)
{
    std::for_each(m_postfix.begin(), m_postfix.end(), [](auto i)->void {std::cout << i.first;});
    std::cout << std::endl;
    std::for_each(m_postfix.begin(), m_postfix.end(), [](auto i)->void {std::cout << (unsigned int)i.second << ' ';});
    std::cout << std::endl;

    // std::cout << std::endl << m_size;

    m_NFA.reserve(m_size + 1);
    m_transition_labels.reserve(m_size + 1);

    std::stack <State_T> first_stack;
    std::stack <State_T> last_stack;

    //indexes that are contained within the stack expression
    std::stack <State_T> pos_stack;

    m_NFA.push_back(State_T(m_size + 1));    //first
    m_transition_labels.push_back({'/', 0});

    std::size_t itr = 1;
    for(const Exp_value_T chr : exp)
    {
        if(is_character(chr))
        {
            // each character has a state 
            m_transition_labels.push_back(chr);

            State_T mask(m_size + 1);
            mask.set(itr);

            // First(x) = {x}
            first_stack.push(mask);
            // Last(x) = {x}
            last_stack.push(mask);

            pos_stack.push(mask);

            // we push the empty bit to its follow() since they come in order
            m_NFA.push_back(State_T(m_size + 1));

            itr++;
        }
        else
        {
            if(chr.second == STAR_VAL)
            {
                // we keep track whether epsilon belongs to the current 
                // subsequence of RE using the 0th byte

                // First(E*) = First(E+) = First(E)
                first_stack.top().set(0);

                // Last(E*) = Last(E+) = Last(E)
                last_stack.top().set(0);

                // follow
                for(std::size_t itr = 1; itr < m_NFA.size(); ++itr)
                    if(pos_stack.top().test(itr) && last_stack.top().test(itr))
                        m_NFA[itr] |= first_stack.top();
            }
            if(chr.second == UNION_VAL)
            {
                // First(F | G) = First(F) U First(G)
                State_T mask = std::move(first_stack.top());
                first_stack.pop();
                first_stack.top() |= mask;

                // Last(F | G) = Last(F) U Last(G)
                mask = std::move(last_stack.top());
                last_stack.pop();
                last_stack.top() |= mask;

                // positions united
                mask = std::move(pos_stack.top());
                pos_stack.pop();
                pos_stack.top() |= mask;

                // nothing changes for follow
            }
            if(chr.second == CONCAT_VAL)
            {
                // First(F + G) = First(F) if epsilon DOES NOT belong to L(F);
                // First(F + G) = First(F) U First(G) if epsilon belongs to L(F)

                State_T mask = std::move(first_stack.top());
                first_stack.pop();

                State_T firstG = mask;    // save for follow...

                if(first_stack.top().test(0))
                {
                    if(!mask.test(0))
                    {
                        first_stack.top().reset(0);
                    }

                    first_stack.top() |= mask;
                }


                // Last(F + G) = Last(F) if epsilon DOES NOT belong to L(F);
                // Last(F + G) = Last(F) U Last(G) if epsilon belongs to L(F)

                mask = std::move(last_stack.top());
                last_stack.pop();

                State_T lastF = last_stack.top();    // save for follow...

                if(mask.test(0))
                {
                    if(!last_stack.top().test(0))
                    {
                        mask.reset(0);
                    }

                    last_stack.top() |= mask;
                }
                else
                {
                    last_stack.emplace(std::move(mask));
                }


                // positions concatenated (no difference)

                mask = std::move(pos_stack.top());
                pos_stack.pop();
                pos_stack.top() |= mask;


                // follow
                for(std::size_t itr = 1; itr < m_NFA.size(); ++itr)
                {
                    if(pos_stack.top().test(itr) && lastF.test(itr))
                    {
                        m_NFA[itr] |= firstG;
                    }
                }
            }
        }
    }

    m_NFA[0] = std::move(first_stack.top());
    for(std::size_t itr = 1; itr < m_NFA.size(); ++itr)
    {
        m_NFA[itr].set(0, last_stack.top().test(itr));
    }
};


template <typename String_T>
bool Regex <String_T>::match(const String_T &word)
{
    State_T state_simulator = m_NFA[0];

    std::cout << word << std::endl;

    for(const Char_T chr : word)
    {
        State_T transition(m_size + 1);

        for(std::size_t itr = 1; itr < m_NFA.size(); ++itr)
        {
            // break if transition is null
            if(state_simulator.test(itr) && m_transition_labels[itr].first == chr)
            {
                transition |= m_NFA[itr];
            }
        }
        state_simulator = std::move(transition);

        std::cout << chr << " " << state_simulator << std::endl;
    }

    return state_simulator.test(0);
};


template <typename String_T>
bool Regex <String_T>::search(const String_T &sample)
{
    // empty words will be ignored

    // std::cout << "Searching for expression: \"" << infixExp_ <<"\" on given string..." << std::endl;

    std::size_t count = 0;
    std::list <std::pair <State_T, std::pair <std::size_t, std::size_t>>> matches; // pair(transition, pair(begin, end))

    for(std::size_t itr = 0; itr < sample.size(); ++itr)
    {
        matches.push_back({m_NFA[0], {itr, -1}});

        auto match_itr = matches.begin();
        while(match_itr != matches.end())
        {
            State_T transition(m_size + 1);
            for(std::size_t jtr = 1; jtr < m_NFA.size(); ++jtr)
            {
                if(match_itr->first.test(jtr) && m_transition_labels[jtr].first == sample[itr])
                {
                    transition |= m_NFA[jtr];
                }
            }

            if(transition.none())
            {
                // cant transition (haha transphobic jokes am i right) :/ yikes bro
                if(match_itr->second.second != -1)
                {
                    std::cout << "Found: \""<< sample.substr(match_itr->second.first, match_itr->second.second - match_itr->second.first + 1) << "\" at index " << match_itr->second.first << std::endl;
                    ++count;
                }
                match_itr = matches.erase(match_itr);
            }
            else
            {
                match_itr->first = std::move(transition); // new state
                if(transition.test(0))
                {
                    match_itr->second.second = itr; // bigger match length   
                    match_itr = matches.erase(std::next(match_itr, 1), matches.end());
                    break;
                }
                match_itr = std::next(match_itr);
            }
        }
    }
    if(matches.begin() != matches.end() && matches.begin()->second.second != -1)
    {
        std::cout << "Found: \""<< sample.substr(matches.begin()->second.first, matches.begin()->second.second - matches.begin()->second.first + 1) << "\" at index " << matches.begin()->second.first << std::endl;
        ++count;
    }
    if(count == 0)
    {
        std::cout << "No matches." << std::endl;

        return false;
    }
    else
    {
        std::cout << "Found a total of " << count << " matches." << std::endl;

        return true;
    }
};




/*
================================================================================
------------------------------ rgx::Regex HELPERS ------------------------------
================================================================================
*/

template <typename String_T>
constexpr bool
Regex <String_T>::is_operator(const Exp_value_T chr) noexcept
{    
    return 1 <= chr.second && chr.second <= 4; 
};

template <typename String_T>
bool Regex <String_T>::is_operator(const Char_T chr) const noexcept
{    
    return chr == UNION || chr == STAR || chr == CONCAT;
};


template <typename String_T>
constexpr bool
Regex <String_T>::is_bracket(const Exp_value_T chr) noexcept
{
    return chr.second == OPEN_VAL || chr.second == CLOSE_VAL;
};

template <typename String_T>
bool Regex <String_T>::is_bracket(const Char_T chr) const noexcept
{
    return chr == OPEN || chr == CLOSE;
};


template <typename String_T>
constexpr bool
Regex <String_T>::is_character(const Exp_value_T chr) noexcept
{
    return chr.second == CHAR_VAL;
};

template <typename String_T>
bool Regex <String_T>::is_character(const Char_T chr) const noexcept
{
    return !(is_operator(chr) || is_bracket(chr) || chr == ESCAPE);
};


template <typename String_T>
constexpr bool
Regex <String_T>::detect_concat(const Exp_value_T chr1, const Exp_value_T chr2)
noexcept
{
    return (
        chr1.second == CHAR_VAL || chr1.second == CLOSE_VAL ||
        chr1.second == STAR_VAL || chr1.second == PLUS_VAL
    ) && (
        chr2.second == CHAR_VAL || chr2.second == OPEN_VAL
    );
};

template <typename String_T>
bool Regex <String_T>::detect_concat(const Char_T chr1, const Char_T chr2)
const noexcept
{
    return(
        is_character(chr1) || chr1 == CLOSE || chr1 == STAR || chr1 == PLUS
    ) && (
        is_character(chr2) || chr2 == OPEN
    );
};


template <typename String_T>
constexpr uint8_t
Regex <String_T>::operator_precedence(const Exp_value_T chr) noexcept
{
    if(chr.second == STAR_VAL || chr.second == PLUS_VAL)
    {
        return 3;
    }

    if(chr.second == CONCAT_VAL)
    {
        return 2;
    }

    if(chr.second == UNION_VAL)
    {
        return 1;
    }

    return 0;
};

template <typename String_T>
uint8_t Regex <String_T>::operator_precedence(const Char_T chr)
const noexcept
{
    if(chr == STAR || chr == PLUS)
    {
        return 3;
    }

    if(chr == CONCAT)
    {
        return 2;
    }

    if(chr == UNION)
    {
        return 1;
    }

    return 0;
};


template <typename String_T>
uint8_t Regex <String_T>::op_value(const Char_T chr) const noexcept
{
    if(chr == UNION)
        return UNION_VAL;

    if(chr == STAR)
        return STAR_VAL;

    if(chr == PLUS)
        return PLUS_VAL;

    if(chr == CONCAT)
        return CONCAT_VAL;

    if(chr == OPEN)
        return OPEN_VAL;

    if(chr == CLOSE)
        return CLOSE_VAL;

    if(chr == ESCAPE)
        return ESCAPE_VAL;

    return CHAR_VAL;
};



}
