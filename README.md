# regex

### What is this?
A header-only lightweight regex library for C++17.


### What can it do?
It allows you to search or match constructed regexes against input.


### What regex operators are available?
* Kleene Star, `*`
* Boolean OR, `|`
* Grouping, `( )`

You can escape operators with `\`.


### Usage
Instantiation:
```cpp
rgx::Regex<> re("(a\\*b*)*c*\\+\\*");
```
Matching:
```cpp
A.match("a*bbbbcccccc+*");    // will match
```
Searching:
```cpp
A.search("a*bbbbcccccc+***aaa*bbbbcccccc+***aa");    // will find 2 occurences at index 0 and 18
```


### Some details
The library should be fairly portable as it uses `std::locale()`. For the same reason, it is NOT a compile-time library (look into CTRE if you're interested in that).

Unlike other, this implementation doesn't build the corresponding DFA for the given regular expression. Instead, it uses **Glushkov's construction** to build an NFA with it's states being represented by bitsets. This allows us to not have to build the DFA (which might end up being very large) but still be able to *virtually parse* it thanks to bitwise operations on the current transition without losing *significant* performance.

For the bitmasking of states my own implementation of a dynamic bitset was also used.


### Should you use this?
**No**. At the moment this is just a for fun project. Extensive testing/benchmarking has not been performed and the features don't really hold up to other libraries.
