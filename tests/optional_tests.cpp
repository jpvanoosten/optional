#include <gtest/gtest.h>
#include <iostream>

#include <optional.hpp>

using namespace opt;

enum class state
{
    DefaultConstructed,
    ValueCopyConstructed,
    ValueMoveConstructed,
    CopyConstructed,
    MoveConstructed,
    CopyAssigned,
    MoveAssigned,
    ValueCopyAssigned,
    ValueMoveAssigned,
    MovedFrom,
    ValueConstructed
};

struct oracle_val
{
    state s;
    int i;
    oracle_val(int _i = 0) noexcept
        : s(state::ValueConstructed)
        , i(_i)
    {}
};

struct oracle
{
    state s;
    oracle_val v;

    oracle() noexcept
        : s(state::DefaultConstructed)
    {}

    oracle(const oracle_val& _v) noexcept
        : s(state::ValueCopyConstructed)
        , v(_v)
    {}

    oracle(oracle_val&& _v) noexcept
        : s(state::ValueMoveConstructed)
        , v(std::move(_v))
    {
        _v.s = state::MovedFrom;
    }

    oracle(const oracle& o) noexcept
        : s(state::CopyConstructed)
        , v(o.v)
    {}

    oracle(oracle&& o) noexcept
        : s(state::MoveConstructed)
        , v(std::move(o.v))
    {
        o.s = state::MovedFrom;
    }

    oracle& operator=(const oracle_val& _v) noexcept
    {
        s = state::ValueCopyConstructed;
        v = _v;

        return *this;
    }

    oracle& operator=(oracle_val&& _v) noexcept
    {
        s = state::ValueMoveConstructed;
        v = std::move(_v);
        _v.s = state::MovedFrom;

        return *this;
    }

    oracle& operator=(const oracle& o) noexcept
    {
        s = state::CopyConstructed;
        v = o.v;

        return *this;
    }

    oracle& operator=(oracle&& o) noexcept
    {
        s = state::MoveConstructed;
        v = std::move(o.v);
        o.s = state::MovedFrom;

        return *this;
    }
};

bool operator==(const oracle& a, const oracle& b)
{
    return a.v.i == b.v.i;
}

bool operator!=(const oracle& a, const oracle& b)
{
    return a.v.i != b.v.i;
}


TEST(optional, Disengaged)
{
    optional<int> o1;
    EXPECT_FALSE(o1);
    EXPECT_TRUE(!o1);
}

TEST(optional, DisengagedNullopt)
{
    optional<int> o1 = nullopt;
    EXPECT_FALSE(o1);

    optional<int> o2 = o1;
    EXPECT_FALSE(o2);

    EXPECT_EQ(o1, nullopt);
    EXPECT_EQ(o1, optional<int>());
    EXPECT_TRUE(!o1);
    EXPECT_EQ(bool(o1), false);

    EXPECT_EQ(o2, nullopt);
    EXPECT_EQ(o2, optional<int>());
    EXPECT_TRUE(!o2);
    EXPECT_EQ(bool(o2), false);

    EXPECT_EQ(o1, o2);
}

TEST(optional, ValueCtor)
{
    oracle_val v;
    optional<oracle> oo1(v);

    EXPECT_TRUE(oo1);
    EXPECT_NE(oo1, nullopt);
    EXPECT_NE(oo1, optional<oracle>());
    EXPECT_EQ(oo1, optional<oracle>(v));
    EXPECT_TRUE(!!oo1);
    EXPECT_TRUE(bool(oo1));
    EXPECT_EQ(oo1->s, state::MoveConstructed);
    EXPECT_EQ(v.s, state::ValueConstructed);

    optional<oracle> oo2(std::move(v));

    EXPECT_NE(oo2, nullopt);
    EXPECT_NE(oo2, optional<oracle>());
    EXPECT_EQ(oo2, oo1);
    EXPECT_TRUE(!!oo2);
    EXPECT_TRUE(bool(oo2));
    EXPECT_EQ(oo2->s, state::MoveConstructed);
    EXPECT_EQ(v.s, state::MovedFrom);
}

TEST(optional, InPlaceCtor)
{
    oracle_val v;
    optional<oracle> oo1{ in_place, v };
    EXPECT_NE(oo1, nullopt);
    EXPECT_NE(oo1, optional<oracle>());
    EXPECT_EQ(oo1, optional<oracle>(v));
    EXPECT_TRUE(!!oo1);
    EXPECT_TRUE(bool(oo1));
    EXPECT_EQ(oo1->s, state::ValueCopyConstructed);
    EXPECT_EQ(v.s, state::ValueConstructed);

    optional<oracle> oo2{ in_place, std::move(v) };
    EXPECT_NE(oo2, nullopt);
    EXPECT_NE(oo2, optional<oracle>());
    EXPECT_EQ(oo2, oo1);
    EXPECT_TRUE(!!oo2);
    EXPECT_TRUE(bool(oo2));
    EXPECT_EQ(oo2->s, state::ValueMoveConstructed);
    EXPECT_EQ(v.s, state::MovedFrom);
}

TEST(optional, InPlaceCondCtor)
{
    oracle_val v;
    optional<oracle> oo1{ in_place_if, false, v };
    EXPECT_EQ(oo1, nullopt);
    EXPECT_EQ(oo1, optional<oracle>());
    EXPECT_NE(oo1, optional<oracle>(v));
    EXPECT_FALSE(!!oo1);
    EXPECT_FALSE(bool(oo1));

    optional<oracle> oo2{ in_place_if, true, std::move(v) };
    EXPECT_NE(oo2, nullopt);
    EXPECT_NE(oo2, optional<oracle>());
    EXPECT_NE(oo2, oo1);
    EXPECT_TRUE(!!oo2);
    EXPECT_TRUE(bool(oo2));
    EXPECT_EQ(oo2->s, state::ValueMoveConstructed);
    EXPECT_EQ(v.s, state::MovedFrom);
}

TEST(optional, Assignment)
{
    optional<int> oi;
    oi = optional<int>(1);

    EXPECT_EQ(*oi, 1);

    oi = nullopt;
    EXPECT_FALSE(oi);
    EXPECT_TRUE(!oi);

    oi = 2;
    EXPECT_EQ(*oi, 2);

    oi = {};
    EXPECT_FALSE(oi);
    EXPECT_TRUE(!oi);
}

template <class T>
struct MoveAware
{
    T val;
    bool moved;
    MoveAware(T val) : val(val), moved(false) {}
    MoveAware(MoveAware const&) = delete;
    MoveAware(MoveAware&& rhs) : val(rhs.val), moved(rhs.moved) {
        rhs.moved = true;
    }
    MoveAware& operator=(MoveAware const&) = delete;
    MoveAware& operator=(MoveAware&& rhs) {
        val = (rhs.val);
        moved = (rhs.moved);
        rhs.moved = true;
        return *this;
    }
};

TEST(optional, MoveAware)
{
    optional<MoveAware<int>> oi{ 1 }, oj{ 2 };
    EXPECT_TRUE(oi);
    EXPECT_FALSE(oi->moved);
    EXPECT_TRUE(oj);
    EXPECT_FALSE(oi->moved);

    optional<MoveAware<int>> ok = std::move(oi);
    EXPECT_TRUE(ok);
    EXPECT_FALSE(ok->moved);
    EXPECT_TRUE(oi);
    EXPECT_TRUE(oi->moved);

    ok = std::move(oj);
    EXPECT_TRUE(ok);
    EXPECT_FALSE(ok->moved);
    EXPECT_TRUE(oj);
    EXPECT_TRUE(oj->moved);
}

TEST(optional, MoveConstruct)
{
    // Move construct a disengaged optional.
    {
        optional<int> oi;
        optional<int> oj = std::move(oi);
        EXPECT_FALSE(oj);
        EXPECT_EQ(oi, oj);
        EXPECT_EQ(oj, oi);
        EXPECT_EQ(oi, nullopt);
        EXPECT_EQ(oj, nullopt);
    }

    // Note: Move constructing an engaged optional does not 
    // disengage the other. Only the internal value is moved.
    // @see https://en.cppreference.com/w/cpp/utility/optional/optional
    {
        optional<int> oi = 1;
        optional<int> oj = std::move(oi);
        EXPECT_TRUE(oj);
        EXPECT_EQ(oi, oj);
        EXPECT_EQ(oj, oi);
        EXPECT_EQ(*oj, 1);
    }
}

TEST(optional, OptionalOptional)
{
    optional<optional<int>> ooi = nullopt;
    EXPECT_EQ(ooi, nullopt);
    EXPECT_FALSE(ooi);

    {
        optional<optional<int>> ooj{ in_place };
        EXPECT_NE(ooj, nullopt);
        EXPECT_TRUE(ooj);
        EXPECT_EQ(*ooj, nullopt);
    }

    {
        optional<optional<int>> ooj{ in_place, nullopt };
        EXPECT_NE(ooj, nullopt);
        EXPECT_TRUE(ooj);
        EXPECT_EQ(*ooj, nullopt);
    }

    {
        optional<optional<int>> ooj{ optional<int>{} };
        EXPECT_NE(ooj, nullopt);
        EXPECT_TRUE(ooj);
        EXPECT_EQ(*ooj, nullopt);
        EXPECT_TRUE(!*ooj);
    }
}

// Guard is a non-copyable, and non-movable object.
struct Guard
{
    std::string val;

    // Default constructible.
    Guard()
        : val{}
    {}

    // Explicit parameterized constructor.
    explicit Guard(std::string s, int = 0)
        : val(s)
    {}

    // Copy constructor is deleted.
    Guard(const Guard&) = delete;
    // Move constructor is deleted.
    Guard(Guard&&) = delete;
    // Copy assignment operator is deleted.
    void operator=(const Guard&) = delete;
    // Move assignment operator is deleted.
    void operator=(Guard&&) = delete;
};

TEST(optional, Guard)
{
    optional<Guard> oga;                    // Disengaged
    optional<Guard> ogb(in_place, "Test");  // Initialize the guard "in-place"
    EXPECT_FALSE(oga);
    EXPECT_TRUE(ogb);
    EXPECT_EQ(ogb->val, "Test");

    optional<Guard> ogc(in_place);          // Default constructs the guard.
    EXPECT_TRUE(ogc);
    EXPECT_EQ(ogc->val, "");

    oga.emplace("Test");                    // Initialize the contained value with "Test".
    EXPECT_TRUE(oga);
    EXPECT_EQ(oga->val, "Test");

    oga.emplace();                          // Default constructs the guard.
    EXPECT_TRUE(oga);
    EXPECT_EQ(oga->val, "");

    oga = nullopt;                          // Disengage the optional guard.
    EXPECT_FALSE(oga);
    EXPECT_TRUE(!oga);
    EXPECT_EQ(oga, nullopt);
}

TEST(optional, OptionalConst)
{
    const optional<int> oi = 4;
    EXPECT_TRUE(oi);
    EXPECT_EQ(*oi, 4);

    // This should fail since we can't assign a value to a const int&
    // *oi = 1;
}

TEST(optional, OptionalRef)
{
    int i = 1;
    int j = 2;

    optional<int&> oi;                      // Disengaged optional to an int&
    EXPECT_FALSE(oi);

    optional<int&> oj = j;                  // oj contains a reference to j.
    EXPECT_TRUE(oj);
    EXPECT_EQ(*oj, j);
    EXPECT_EQ(*oj, 2);

    *oj = 3;                                // j = 3.
    EXPECT_EQ(j, 3);

    // oi = j;                              // Error: refs can't be assigned after definition.
    oi = { j };                               // OK: oi is now engaged with a reference to j.
    oi = oj;                                // OK: oi is now engaged with a reference to j.
    EXPECT_EQ(*oi, j);

    oi.emplace(i);
    EXPECT_EQ(*oi, i);

    oj.emplace(j);
    EXPECT_EQ(*oj, j);

    oi = nullopt;                           // OK, oi is now disengaged.
    EXPECT_FALSE(oi);
}

template <typename T>
T getValue(optional<T> newVal = nullopt, optional<T&> storeHere = nullopt)
{
    T cached{};

    if (newVal) {
        cached = *newVal;

        if (storeHere) {
            *storeHere = *newVal; // LEGAL: assigning T to T
        }
    }
    return cached;
}

TEST(optional, OptionalArg)
{
    int i = 5;
    i = getValue<int>(i, i);
    EXPECT_EQ(i, 5);
    i = getValue<int>(i);
    EXPECT_EQ(i, 5);
    i = getValue<int>();
    EXPECT_EQ(i, 0);
}

// A type that takes singular ownership of it's internal state.
// This type is neither copy constructable nor copy assignable.
struct Owner
{
    int i;

    Owner() = delete;
    Owner(int _i)
        : i(_i)
    {}

    Owner(Owner&& o)
        : i(o.i)
    {
        o.i = 0;
    }

    // No copy-constructor.
    Owner(const Owner& d) = delete;
    // No copy-assignment.
    Owner& operator=(const Owner&) = delete;

    Owner& operator=(Owner&& o)
    {
        i = o.i;
        o.i = 0;

        return *this;
    }
};

bool operator==(const Owner& lhs, const Owner& rhs)
{
    return lhs.i == rhs.i;
}

bool operator!=(const Owner& lhs, const Owner& rhs)
{
    return lhs.i != rhs.i;
}

std::tuple<Owner, Owner, Owner> getOwners()
{
    return std::tuple<Owner, Owner, Owner>(Owner{1}, Owner{2}, Owner{3});
}

TEST(optional, NoCopy)
{
    optional<Owner> i, j, k;    // OK, i, j, k are disengaged.
    EXPECT_FALSE(i);
    EXPECT_FALSE(j);
    EXPECT_FALSE(k);

    std::tie(i, j, k) = getOwners();

    EXPECT_EQ(*i, Owner(1));
    EXPECT_EQ(*j, Owner(2));
    EXPECT_EQ(*k, Owner(3));
}

TEST(optional, Relational)
{
    unsigned int i0 = 0;
    unsigned int i1 = 1;

    // Declare an optional unsigned integer type.
    using ouint = optional<unsigned int>;
    using oruint = optional<unsigned int&>;

    EXPECT_EQ(ouint(), ouint());            // Two disengaged optionals compare equal.
    EXPECT_LT(ouint(), ouint(0));           // Disengaged is always less than an engaged optional.
    EXPECT_GT(ouint(0), ouint());
    EXPECT_LT(ouint(0), ouint(1));          // Two engaged optionals will compare their internal values.
    EXPECT_GT(ouint(1), ouint(0));
    EXPECT_EQ(ouint(0), ouint(0));
    EXPECT_FALSE(ouint() < ouint());        // Two disengaged optionals are neither less than nor greater than.
    EXPECT_FALSE(ouint() > ouint());
    EXPECT_TRUE(ouint() <= ouint());
    EXPECT_TRUE(ouint() >= ouint());

    EXPECT_NE(ouint(), ouint(0));           // Test inequality.
    EXPECT_NE(ouint(0), ouint());
    EXPECT_NE(ouint(0), ouint(1));
    EXPECT_NE(ouint(1), ouint(0));

    EXPECT_EQ(ouint(), nullopt);            // Disengaged optional compare equal to nullopt.
    EXPECT_EQ(nullopt, ouint());

    EXPECT_NE(ouint(0), nullopt);           // Engaged optional compare not equal to nullopt.
    EXPECT_NE(nullopt, ouint(0));

    EXPECT_FALSE(ouint(0) < nullopt);       // nullopt is always less than an engaged optional.
    EXPECT_TRUE(nullopt < ouint(0));
    EXPECT_TRUE(ouint(0) > nullopt);        // Engaged optional is always greater than nullopt.
    EXPECT_FALSE(nullopt > ouint(0));

    EXPECT_LE(nullopt, ouint(0));           // nullopt is always less-than or equal to an engaged optional.
    EXPECT_GE(ouint(0), nullopt);
    EXPECT_FALSE(ouint(0) <= nullopt);
    EXPECT_TRUE(ouint(0) >= nullopt);
    EXPECT_TRUE(nullopt <= ouint(0));
    EXPECT_FALSE(nullopt >= ouint(0));

    EXPECT_EQ(ouint(0), 0u);                 // Compare with values.
    EXPECT_EQ(0u, ouint(0));
    EXPECT_NE(ouint(0), 1u);
    EXPECT_NE(1u, ouint(0));
    EXPECT_LT(ouint(0), 1u);
    EXPECT_GT(1u, ouint(0));
    EXPECT_GT(ouint(1), 0u);
    EXPECT_LT(0u, ouint(1));
    EXPECT_GE(ouint(1), 0u);
    EXPECT_LE(0u, ouint(1));
    EXPECT_GE(1u, ouint(0));
    EXPECT_LE(ouint(0), 1u);

    // Optional reference to a value.
    oruint oi0 = i0;
    oruint oi1 = i1;

    EXPECT_EQ(oi0, i0);                     // Compare with optional reference types.
    EXPECT_EQ(i0, oi0);
    EXPECT_NE(oi0, i1);
    EXPECT_NE(i1, oi0);
    EXPECT_LT(oi0, i1);
    EXPECT_GT(i1, oi0);
    EXPECT_GT(oi1, i0);
    EXPECT_LT(i0, oi1);
    EXPECT_GE(oi1, i1);
    EXPECT_LE(i1, oi1);
    EXPECT_GE(oi1, i0);
    EXPECT_LE(i0, oi1);


}
// TODO: Emplace and construct with initializer lists.