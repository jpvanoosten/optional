#include <gtest/gtest.h>
#include <iostream>

#include <optional.hpp>

void void_func() noexcept
{
    std::cout << "Function 1" << std::endl;
    return;
}

void void_func2() noexcept
{
    std::cout << "Function 2" << std::endl;
}

int sum(int i, int j)
{
    return i + j;
}

auto lambda = [](int i, int j) { return i + j; };

struct Functor
{
    Functor(int i, int j)
        : m_i(i)
        , m_j(j)
    {}

    int operator()() const noexcept
    {
        return m_i + m_j;
    }

    int m_i, m_j;
};

class Base
{
public:
    Base(int i, int j)
        : m_i(i)
        , m_j(j)
    {}

    int multiply(int i, int j) const noexcept
    {
        return i * j;
    }

    virtual int sum() const noexcept
    {
        return m_i + m_j;
    }

protected:
    int m_i, m_j;
};

class Derived : public Base
{
public:
    Derived(int i, int j)
        : Base(i, j)
    {}

    virtual int sum() const noexcept override
    {
        return Base::sum() + 1;
    }
};

struct PointerToMemberData
{
    PointerToMemberData(int v)
    : value(v)
    {}

    int value;
};

TEST(optional, Test1)
{
    EXPECT_TRUE(true);
}