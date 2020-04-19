#pragma once

/*
 *  Copyright(c) 2020 Jeremiah van Oosten
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files(the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions :
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 *  IN THE SOFTWARE.
 */

 /**
  *  @file optional.hpp
  *  @date April 19, 2020
  *  @author Jeremiah van Oosten
  *
  *  @brief C++11 version of C++17's std::optional type based on boost::optional.
  */

#include <exception>    // for std::exception
#include <type_traits>
#include <utility>      // for std::move

  // Check for inline variable support (requires C++17)
#if defined(__cpp_inline_variables) && __cpp_inline_variables >= 201606L
#define INLINE_VAR inline constexpr
#else
#define INLINE_VAR constexpr
#endif

namespace opt
{
    // Since C++17
    // @see https://en.cppreference.com/w/cpp/utility/optional/bad_optional_access
    class bad_optional_access : public std::exception
    {
    public:
        bad_optional_access()
            : std::exception("Attempted to access the value of an uninitialized optional object.")
        {}
    };

    // Since C++17
    // @see https://en.cppreference.com/w/cpp/utility/optional/nullopt_t
    struct nullopt_t
    {
        struct init_tag {};
        explicit constexpr nullopt_t(init_tag) {}
    };

    // Since C++17
    // @see https://en.cppreference.com/w/cpp/utility/optional/nullopt
    INLINE_VAR nullopt_t nullopt(nullopt_t::init_tag());

    // Forward declare optional template class.
    template<class T> class optional;
    template<class T> class optional<T&>;

    // This forward is needed to refer to namespace scope swap from the member swap
    template<class T> struct optional_swap_should_use_default_constructor;
    template<class T> void swap(optional<T>&, optional<T>&);
    template<class T> void swap(optional<T&>&, optional<T&>&) noexcept;


    namespace detail
    {
        // A tag for in-place initialization of contained value.
        struct in_place_init_t
        {
            struct init_tag {};
            explicit in_place_init_t(init_tag) {}
        };

        INLINE_VAR in_place_init_t in_place_init(in_place_init_t::init_tag());

        // A tag for conditional in-place initialization of contained value.
        struct in_place_init_if_t
        {
            struct init_tag {};
            explicit in_place_init_if_t(init_tag) {}
        };

        INLINE_VAR in_place_init_if_t in_place_init_if(in_place_init_if_t::init_tag());

        struct init_value_tag {};

        struct optional_tag {};

        template<typename T>
        class optional_base : public optional_tag
        {
        private:
            using storage_type = typename std::aligned_storage<sizeof(T), alignof(T)>::type;
            using this_type = optional_base<T>;

            bool m_initialized;
            storage_type m_storage;

        protected:
            using value_type = T;
            using reference_type = T&;
            using reference_const_type = T const&;
            using rval_reference_type = T&&;
            using reference_type_of_temporary_wrapper = T&&;
            using pointer_type = T*;
            using pointer_const_type = T const*;
            using argument_type = T const&;

            // Creates an optional<T> uninitialized
            optional_base() noexcept
                : m_initialized(false)
            {}

            // Creates an optional<T> uninitialized
            optional_base(nullopt_t) noexcept
                : m_initialized(false)
            {}

            // Creates an optional<T> initialized with 'val'.
            // Can throw if T::T(T const&) does
            optional_base(init_value_tag, argument_type val)
                : m_initialized(false)
            {
                construct(val);
            }

            // move-construct an optional<T> initialized from an rvalue-ref to 'val'.
            // Can throw if T::T(T&&) does
            optional_base(init_value_tag, rval_reference_type val)
                : m_initialized(false)
            {
                construct(std::move(val));
            }

            // Creates an optional<T> initialized with 'val' IFF cond is true, 
            // otherwise creates an uninitialized optional<T>.
            // Can throw if T::T(T const&) does
            optional_base(bool cond, argument_type val)
                : m_initialized(false)
            {
                if (cond)
                    construct(val);
            }

            // Creates an optional<T> initialized with 'move(val)' IFF cond is true, 
            // otherwise creates an uninitialized optional<T>.
            // Can throw if T::T(T &&) does
            optional_base(bool cond, rval_reference_type val)
                : m_initialized(false)
            {
                if (cond)
                    construct(std::move(val));
            }

            // Creates a deep copy of another optional<T>
            // Can throw if T::T(T const&) does
            optional_base(optional_base const& rhs)
                : m_initialized(false)
            {
                if (rhs.is_initialized())
                    construct(rhs.get_impl());
            }

            // Creates a deep move of another optional<T>
            // Can throw if T::T(T&&) does
            optional_base(optional_base&& rhs)
                noexcept((std::is_nothrow_move_constructible<T>::value))
                : m_initialized(false)
            {
                if (rhs.is_initialized())
                    construct(std::move(rhs.get_impl()));
            }

            // This is used for both converting and in-place constructions.
            // Derived classes use the 'tag' to select the appropriate
            // implementation (the correct 'construct()' overload)
            template<class Expr, class PtrExpr>
            explicit optional_base(Expr&& expr, PtrExpr const* tag)
                : m_initialized(false)
            {
                construct(std::forward<Expr>(expr), tag);
            }

            optional_base& operator=(optional_base const& rhs)
            {
                this->assign(rhs);

                return *this;
            }

            optional_base& operator=(optional_base&& rhs)
                noexcept((std::is_nothrow_move_constructible<T>::value&&
                    std::is_nothrow_move_assignable<T>::value))
            {
                // This is the original cast from the boost C++ library.
                // Note: Shouldn't this be this->assign(std::move(rhs)) ?
                //this->assign(std::move(rhs));
                this->assign(static_cast<optional_base&&>(rhs));

                return *this;
            }

            // No-throw (assuming T::~T() doesn't)
            ~optional_base()
            {
                destroy();
            }

            // Assigns from another optional<T> (deep-copies the rhs value)
            void assign(optional_base const& rhs)
            {
                if (is_initialized())
                {
                    if (rhs.is_initialized())
                        assign_value(rhs.get_impl());
                    else
                        destroy();
                }
                else
                {
                    if (rhs.is_initialized())
                        construct(rhs.get_impl());
                }
            }

            // Assigns from another optional<T> (deep-moves the rhs value)
            void assign(optional_base&& rhs)
            {
                if (is_initialized())
                {
                    if (rhs.is_initialized())
                        assign_value(std::move(rhs.get_impl()));
                    else
                        destroy();
                }
                else
                {
                    if (rhs.is_initialized())
                        construct(std::move(rhs.get_impl()));
                }
            }

            // Assigns from another _convertible_ optional<U> (deep-copies the rhs value)
            template<class U>
            void assign(opt::optional<U> const& rhs)
            {
                if (is_initialized())
                {
                    if (rhs.is_initialized())
                        assign_value(rhs.get());
                    else
                        destroy();
                }
                else
                {
                    if (rhs.is_initialized())
                        construct(rhs.get());
                }
            }

            // move-assigns from another _convertible_ optional<U> (deep-moves from the rhs value)
            template<class U>
            void assign(optional<U>&& rhs)
            {
                using ref_type = typename optional<U>::rval_reference_type;

                if (is_initialized())
                {
                    if (rhs.is_initialized())
                        assign_value(static_cast<ref_type>(rhs.get()));
                    else destroy();
                }
                else
                {
                    if (rhs.is_initialized())
                        construct(static_cast<ref_type>(rhs.get()));
                }
            }

            // Assigns from a T (deep-copies the rhs value)
            void assign(argument_type val)
            {
                if (is_initialized())
                    assign_value(val);
                else
                    construct(val);
            }

            // Assigns from a T (deep-moves the rhs value)
            void assign(rval_reference_type val)
            {
                if (is_initialized())
                    assign_value(std::move(val));
                else
                    construct(std::move(val));
            }

            // Assigns from "none", destroying the current value, if any, leaving this UNINITIALIZED
            // No-throw (assuming T::~T() doesn't)
            void assign(nullopt_t) noexcept
            {
                destroy();
            }

        public:

            // Destroys the current value, if any, leaving this UNINITIALIZED
            // No-throw (assuming T::~T() doesn't)
            void reset() noexcept
            {
                destroy();
            }

            // Returns a pointer to the value if this is initialized, otherwise,
            // returns NULL.
            // No-throw
            pointer_const_type get_ptr() const
            {
                return m_initialized ? get_ptr_impl() : nullptr;
            }

            pointer_type get_ptr()
            {
                return m_initialized ? get_ptr_impl() : nullptr;
            }

            bool is_initialized() const noexcept
            {
                return m_initialized;
            }

        protected:

            void construct(argument_type val)
            {
                ::new(&m_storage) value_type(val);
                m_initialized = true;
            }

            void construct(rval_reference_type val)
            {
                ::new(&m_storage) value_type(std::move(val));
                m_initialized = true;
            }

            // Constructs in-place
            // upon exception *this is always uninitialized
            template<class... Args>
            void construct(in_place_init_t, Args&&... args)
            {
                ::new (&m_storage) value_type(std::forward<Args>(args)...);
                m_initialized = true;
            }

            template<class... Args>
            void emplace_assign(Args&&... args)
            {
                destroy();
                construct(in_place_init, std::forward<Args>(args)...);
            }

            template<class... Args>
            explicit optional_base(in_place_init_t, Args&&... args)
                : m_initialized(false)
            {
                construct(in_place_init, std::forward<Args>(args)...);
            }

            template<class... Args>
            explicit optional_base(in_place_init_if_t, bool cond, Args&&... args)
                : m_initialized(false)
            {
                if (cond)
                    construct(in_place_init, std::forward<Args>(args)...);
            }

            // Constructs using any expression implicitly convertible to the single argument
            // of a one-argument T constructor.
            // Converting constructions of optional<T> from optional<U> uses this function with
            // 'Expr' being of type 'U' and relying on a converting constructor of T from U.
            template<class Expr>
            void construct(Expr&& expr, void const*)
            {
                new (&m_storage) value_type(std::forward<Expr>(expr));
                m_initialized = true;
            }

            // Assigns using a form any expression implicitly convertible to the single argument
            // of a T's assignment operator.
            // Converting assignments of optional<T> from optional<U> uses this function with
            // 'Expr' being of type 'U' and relying on a converting assignment of T from U.
            template<class Expr>
            void assign_expr_to_initialized(Expr&& expr, void const*)
            {
                assign_value(std::forward<Expr>(expr));
            }

            void assign_value(argument_type val)
            {
                get_impl() = val;
            }

            void assign_value(rval_reference_type val)
            {
                get_impl() = static_cast<rval_reference_type>(val);
            }

            void destroy()
            {
                if (m_initialized)
                    destroy_impl();
            }

            reference_const_type get_impl() const
            {
                return reinterpret_cast<reference_const_type>(m_storage);
            }

            reference_type get_impl()
            {
                return reinterpret_cast<reference_type>(m_storage);
            }

            pointer_const_type get_ptr_impl() const
            {
                return reinterpret_cast<pointer_const_type>(&m_storage);
            }

            pointer_type get_ptr_impl() 
            { 
                return reinterpret_cast<pointer_type>(&m_storage); 
            }

        private:
            void destroy_impl() 
            { 
                get_impl().~T();
                m_initialized = false;
            }
        };

    }
}

