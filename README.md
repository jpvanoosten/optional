![Build status](https://github.com/jpvanoosten/optional/workflows/C/C++%20CI/badge.svg?branch=dev)

# Optional

A C++11 implementation of [std::optional] (C++17) based on [boost::optional]. Also inspired by [https://github.com/akrzemi1/Optional](https://github.com/akrzemi1/Optional).

## Motivation

For another project I'm working on, I needed a way to return a "maybe not a value" from a function. The function uses smart pointers ([std::shared_ptr] and [std::weak_ptr]) to dereference an object, call another function on that object, and return the result of caling that function. But what should be returned if the smart pointer was not valid? I needed a way to return the concept of "this might not be a valid result".

There are several solutions to this problem:

1. If it's a pointer type, return `nullptr`
2. If it's an `int`, return `-1`?
3. If it's an `unsigned int`, return `UINT_MAX`?
4. If it's a `float`, return ?
5. Throw an exception?
6. Pass an additional parameter to the function which indicates an error code?
7. `assert`?

Option 1 may be valid in certain cases, but for primitive types (int, float, bool, etc...) there may not be a good value to return to indicate "the response from this function is not valid". Any choice in the "error sentinal" we make here seems arbitrary.

Throwing an exception may seem like a valid solution, but not everyone wants to have to deal with exceptions in their applications and in some cases, the overhead of exception handling may be deemed unacceptable and is usually disabled at the compiler level.

Returning an error code or passing an additional parameter (by pointer or by reference) to the function that is used to return an error code is a popular solution in many C APIs but that means that either the API can _only_ return an error code from a function or that every function has an additional error code parameter. This might be useful in the case where you need to indicate that one of many possible errors occured in this function, but in the general case, this feels like overkill.

But what if the function needs to return a reference to an object? In this case, there isn't a good way to say that the object being returned is an invalid object (since references can't be null). Or you have to have some kind of garantee that the returned object will have a function to check if the object being returned from the function is not valid.

`assert` may not be what you want either because `assert` only works in debug builds but it's possible the error condition can occur in release builds.

## Solution

Most of these cases can be handled elegently if there was a method to return a "maybe not a value" object from the function. Checking if the object contains a valid value should be easy, and getting to the underlying value should also be easy. If one tries to access the underlying value of an invalid reslut, well then we can still fall back on either throwing an exception or asserting.

The solution to this is the `opt::optional` optional type. It's a template class that is parameterized on the type that can possibily not be valid.

By default, the `opt::optional` does not contain a valid value. In this case, the `optional` is _disengaged_ from its value. When the `optional` stores a valid value, it is _engaged_ with a value.

## Usage

To declare an optional value, specify `wp::optional<T>` where `T` is the internal value type. By default, an optional value is _disengaged_. It is _engaged_ when it stores a valid value.

```c++
#include "optional.hpp"
#include <iostream>

...
opt::optional<int> oi;                  // oi is a disengaged int.
opt::optional<int> oj = opt::nullopt;   // oj is disengaged.
oi = 1;                                 // oi is now engaged with the value 1.
oi = opt::nullopt;                      // oi is disengaged again.

if ( oi == oj )                         // Two disengaged optional values are compareable.
{
    std::cout << "Two optional values are equality comparable." << std::endl;
}

if ( oi == opt::nullopt )               // Optional is compareable to opt::nulltop
{
    std::cout << "Optional is disengaged." << std::endl;
}

if ( oi != opt::nullopt )
{
    std::cout << "Optional is engaged." << std::endl;
}

if (oi)                                 // optional supports explicit conversion to bool.
{
    std::cout << "Optional engaged." << std::endl;
}
else
{
    std::cout << "Optional is disengaged." << std::endl;
}

int i = *oi;                            // The * operator is used to obtain the internal value.
                                        // No exception is thrown, but result is UNDEFINED.
i = oi.value();                         // Exception throwing if disengaged.
i = oi.value_or(0);                     // Return the value or another value if disengaged.
```

## Return Optional

You can use an optional value to indicate if an invalid parameter was passed to a function. For example, taking the square root of a negative number is an invalid operation. Instead of returning NaN, one can use an optional value. If a negative number is passed to the `sqrtf` function, a disengaged optional value is returned.

```c++
#include "optional.hpp"
#include <cmath>

// Returns a disengaged optional on failure.
opt::optional<float> sqrtf(float arg)
{
    if ( arg >= 0.0f )
    {
        return std::sqrtf(arg);
    }
    else
    {
        return {};  // Default construct a disengaged optional.
    }
}

...

auto res = sqrtf(-1.0f);

if (res)    // Explict bool conversion indicates if the optional value is engaged or not.
{
    std::cout << "Value is valid: " << *res << std::endl;
}
else
{
    std::cout << "Value is invalid." << std::endl;
}
```

Another example is returning the largest value of a vector of values:

```c++
#include "optional.hpp"
#include <vector>

opt::optional<int> findBiggest(const std::vector<int>& v)
{
    opt::optional<long> biggest;
    for(auto i : v)
    {
        if (!biggest || *biggest < i)
            biggest = i;
    }
}

...

std::vector<int> v;
// Find the largest value of an empty vector??
auto biggest = findBiggest(v);

// Okay, biggest is disengaged.
assert(!biggest);

v = {5, 10, 15, 20, 15};

// Okay, now biggest should contain 20.
biggest = findBiggest(v);

assert(*biggest == 20);
```

Attempting to return the largest value of an emtpy array should result in an error, but using optionals, a disengaged optional can be returned. If the array contains values, the return value is engaged with the largest value.

## Pass Optional Prameters

You can also use otpional as an argument to a function.

```c++
int updateCache( opt::optional<int> newValue = opt::nullopt )
{
    if ( newValue )
    {
        cached = *newValue;
    }
    return cached;
}
```

The `updateCache` method demonstrates that an optional value can be used to conditionally update the state of an object if the optional value is engaged.

## Comparison

`opt::optional` supports the full set of comparison operators as long as the value type does.

```c++
#include "optional.hpp"
#include <cassert>

opt::optional<unsigned> o;      // o is disengaged.
opt::optional<unsigned> o0 = 0; // o0 is engaged with the value 0.
opt::optional<unsigned> o1 = 1; // o1 is engaged with the value 1.

assert(o0 == o0)                // Disengaged optionals compare equal.
assert(o0 != o1);               // Engaged optionals compare their values.
assert(o0 < o1);
assert(o1 > o0);
assert(o0 <= o1);
assert(o1 >= o0);

assert(opt::nullopt != o0);     // nullopt is always less than an engaged value.
assert(opt::nullopt < o0);
assert(o0 > opt::nullopt);

assert(opt::nullopt == o);      // nullopt compares equal to disengaged optional.
assert(o == opt::nullopt);

assert(o0 == 0u);               // Optionals can be compared against their value types.
assert(0u == o0);
assert(o0 < 1u);
assert(o0 <= 1u);
assert(1u > o0);
assert(1u >= o0);
```

## Known Issues

* This library has not been tested with callable types (such as the result of [std::function]).

[std::optional]: https://en.cppreference.com/w/cpp/utility/optional
[std::shared_ptr]: https://en.cppreference.com/w/cpp/memory/shared_ptr
[std::weak_ptr]: https://en.cppreference.com/w/cpp/memory/weak_ptr
[boost::optional]: https://www.boost.org/doc/libs/1_72_0/libs/optional/doc/html/index.html
[std::function]: https://en.cppreference.com/w/cpp/utility/functional/function