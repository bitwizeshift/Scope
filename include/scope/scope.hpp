/****************************************************************************
 * \file scope.hpp
 *
 * This header contains the definition for scope utilities, as defined in the
 * p0052r6 proposal paper:
 * http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0052r6.pdf
 *
 * This file is written in C++11, with compatible extensions for C++17
 * through the constructor deduction guides.
 ****************************************************************************/

//          Copyright Matthew Rodusek 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)
#ifndef SCOPE_SCOPE_HPP
#define SCOPE_SCOPE_HPP

#include <type_traits> // std::decay, std::is_*
#include <functional>  // std::reference_wrapper, std::ref
#include <utility>     // std::forward, std::move
#include <exception>   // std::uncaught_exception(s)
#include <limits>      // std::numeric_limits

//! \def SCOPE_NODISCARD
//!
//! \brief A macro which expands into the attribute for warning on a
//!        discarded value.
//!
//! This is used within Scope to ensure that a user does not create a
//! temporary scope guard instance without holding onto it for the
//! remainder of the scope.
#if __cplusplus >= 201703L
# define SCOPE_NODISCARD [[nodiscard]]
#elif defined(__GNUC__) || defined(__CLANG__)
# define SCOPE_NODISCARD __attribute__ ((warn_unused_result))
#elif defined(_MSC_VER) && (_MSC_VER >= 1700)
# define SCOPE_NODISCARD _Check_return_
#else
# define SCOPE_NODISCARD
#endif

// The large #if/endif block below, and the definition of
// scope::detail::uncaught_exception_count is adapted from boost:
// https://beta.boost.org/doc/libs/develop/boost/core/uncaught_exceptions.hpp

//             Copyright Andrey Semashev 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#if (defined(__cpp_lib_uncaught_exceptions) && __cpp_lib_uncaught_exceptions >= 201411) || \
    defined(_MSC_VER) && _MSC_VER >= 1900
# define SCOPE_HAS_UNCAUGHT_EXCEPTIONS
#endif

#if !defined(SCOPE_HAS_UNCAUGHT_EXCEPTIONS)

// cxxabi.h availability macro
# if defined(__has_include)
#   if __has_include(<cxxabi.h>)
#     define SCOPE_HAS_CXXABI_H
#   endif
# elif defined(__GLIBCXX__) || defined(__GLIBCPP__)
#   define SCOPE_HAS_CXXABI_H
# endif

# if defined(SCOPE_HAS_CXXABI_H)
// MinGW GCC 4.4 seem to not work the same way the newer GCC versions do. As
// a result, __cxa_get_globals based implementation will always return 0.
// Just disable it for now and fall back to std::uncaught_exception().
#   if !(defined(__MINGW32__) && (defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__) < 405))
#     include <cxxabi.h>
#     include <cstring>
#     define SCOPE_HAS_CXA_GET_GLOBALS

// At least on MinGW and Linux, only GCC since 4.7 declares __cxa_get_globals()
// in cxxabi.h. Older versions of GCC do not expose this function but it's
// there.
// On OpenBSD, it seems, the declaration is also missing.
// Note that at least on FreeBSD 11, cxxabi.h declares __cxa_get_globals with
// a different exception specification, so we can't declare the function
// unconditionally. On Linux with clang and libc++ and on OS X, there is a
// version of cxxabi.h from libc++abi that doesn't declare __cxa_get_globals,
// but provides __cxa_uncaught_exceptions.
// The function only appeared in version _LIBCPPABI_VERSION >= 1002 of the
// library. Unfortunately, there are linking errors about undefined reference
// to __cxa_uncaught_exceptions on Ubuntu Trusty and OS X, so we avoid using
// it and forward-declare __cxa_get_globals instead. On QNX SDP 7.0 (QCC 5.4.0),
// there are multiple cxxabi.h, one from glibcxx from gcc and another from
// libc++abi from LLVM. Which one is included will be determined by the qcc
// command line arguments (-V and/or -Y;
// http://www.qnx.com/developers/docs/7.0.0/#com.qnx.doc.neutrino.utilities/topic/q/qcc.html
// ).
// The LLVM libc++abi is missing the declaration of __cxa_get_globals but it is
// also patched by QNX developers to not define _LIBCPPABI_VERSION. Older QNX
// SDP versions, up to and including 6.6, don't provide LLVM and libc++abi.
// See https://github.com/boostorg/core/issues/59.
#     if !defined(__FreeBSD__) && \
    ( \
        (defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__) < 407) || \
        defined(__OpenBSD__) || \
        (defined(__QNXNTO__) && !defined(__GLIBCXX__) && !defined(__GLIBCPP__)) || \
        defined(_LIBCPPABI_VERSION) \
    )
namespace __cxxabiv1 {
  struct __cxa_eh_globals;
#       if defined(__OpenBSD__)
  extern "C" __cxa_eh_globals* __cxa_get_globals();
#       else
  extern "C" __cxa_eh_globals* __cxa_get_globals() noexcept __attribute__((__const__));
#       endif
} // namespace __cxxabiv1
#     endif
#   endif // !(defined(__MINGW32__) && (defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__) < 405))
# endif // defined(SCOPE_HAS_CXXABI_H)

# if defined(_MSC_VER) && _MSC_VER >= 1400
#   include <cstring>
#   define SCOPE_HAS_GETPTD
namespace scope {
  namespace detail {
    extern "C" void* _getptd();
  } // namespace detail
} // namespace scope
# endif // defined(_MSC_VER) && _MSC_VER >= 1400
#endif // !defined(SCOPE_HAS_UNCAUGHT_EXCEPTIONS)

#if !defined(SCOPE_HAS_UNCAUGHT_EXCEPTIONS) && \
    !defined(SCOPE_HAS_CXA_GET_GLOBALS) && \
    !defined(SCOPE_HAS_GETPTD)
//! This macro is defined when `uncaught_exceptions` is not guaranteed to
//! return values greater than 1 if multiple exceptions are pending
# define SCOPE_UNCAUGHT_EXCEPTIONS_EMULATED
#endif

namespace scope {

  //==========================================================================
  // implementation utilities
  //==========================================================================

  namespace detail {

    inline int uncaught_exception_count()
    {
#if defined(SCOPE_HAS_UNCAUGHT_EXCEPTIONS)
      // C++17 implementation
      return std::uncaught_exceptions();
#elif defined(SCOPE_HAS_CXA_GET_GLOBALS)
      // Tested on {clang 3.2,GCC 3.5.6,GCC 4.1.2,GCC 4.4.6,GCC 4.4.7}x{x32,x64}
      using byte = unsigned char;
      auto count = int{};
      const auto* ptr = reinterpret_cast<const byte*>(::abi::__cxa_get_globals()) + sizeof(void*);

      // __cxa_eh_globals::uncaughtExceptions, x32 offset - 0x4, x64 - 0x8
      std::memcpy(&count, ptr, sizeof(count));
      return count;
#elif defined(SCOPE_HAS_GETPTD)
      // MSVC specific. Tested on {MSVC2005SP1,MSVC2008SP1,MSVC2010SP1,MSVC2012}x{x32,x64}.
      using byte = unsigned char;
      auto count = int{};

      const auto offset = (sizeof(void*) == 8u ? 0x100 : 0x90);
      const auto* ptr = static_cast<const byte*>(::scope::detail::_getptd()) + offset;

      // _tiddata::_ProcessingThrow, x32 offset - 0x90, x64 - 0x100
      std::memcpy(&count, ptr, sizeof(count));
      return count;
#else
      // Portable C++03 implementation. Does not allow to detect multiple
      // nested exceptions.

      // This is a buggy fallback since it will only work with 1 exception
      // in-flight, but we don't have any other options without exploiting
      // internal compiler features.
      return static_cast<int>(std::uncaught_exception());
#endif
    }

    //========================================================================
    // internal class : on_exit_policy
    //========================================================================

    class on_exit_policy
    {
    public:
      on_exit_policy() noexcept
        : m_should_execute{true}
      {

      }
      on_exit_policy(on_exit_policy&&) = default;
      on_exit_policy(const on_exit_policy&) = default;
      on_exit_policy& operator=(on_exit_policy&&) = default;
      on_exit_policy& operator=(const on_exit_policy&) = default;

      void release() noexcept
      {
        m_should_execute = false;
      }

      bool should_execute() const noexcept
      {
        return m_should_execute;
      }

    private:

      bool m_should_execute;
    };

    //========================================================================
    // internal class : on_fail_policy
    //========================================================================

    class on_fail_policy
    {
    public:
      on_fail_policy() noexcept
        : m_exception_count{uncaught_exception_count()}
      {

      }
      on_fail_policy(on_fail_policy&&) = default;
      on_fail_policy(const on_fail_policy&) = default;
      on_fail_policy& operator=(on_fail_policy&&) = default;
      on_fail_policy& operator=(const on_fail_policy&) = default;

      void release() noexcept
      {
        m_exception_count = std::numeric_limits<int>::max();
      }

      bool should_execute() const noexcept
      {
        return m_exception_count < uncaught_exception_count();
      }

    private:

      int m_exception_count;
    };

    //========================================================================
    // internal class : on_success_policy
    //========================================================================

    class on_success_policy
    {
    public:
      on_success_policy() noexcept
        : m_exception_count{uncaught_exception_count()}
      {

      }

      on_success_policy(on_success_policy&&) = default;
      on_success_policy(const on_success_policy&) = default;
      on_success_policy& operator=(on_success_policy&&) = default;
      on_success_policy& operator=(const on_success_policy&) = default;

      void release() noexcept
      {
        m_exception_count = -1;
      }

      bool should_execute() const noexcept
      {
        return m_exception_count == uncaught_exception_count();
      }

    private:

      int m_exception_count;
    };

    //========================================================================
    // internal non-member functions
    //========================================================================

    template<typename T>
    std::conditional<
      std::is_nothrow_move_assignable<T>::value,
      T&&,
      T const&
    >
    move_if_noexcept_assignable(T& x) noexcept
    {
      return std::move(x);
    }

    template<typename T>
    const T& as_const(T& t) noexcept { return t; }

    //========================================================================
    // internal class : storage
    //========================================================================

    template <typename T>
    class storage
    {
    public:

      template<typename TT, typename Guard>
      explicit storage(TT&& t, Guard&& guard)
        : m_value(std::forward<TT>(t))
      {
        guard.release();
      }
      explicit storage(const T& t)
          : m_value(t)
      {

      }
      explicit storage(T&& t)
          : m_value(std::move_if_noexcept(t))
      {

      }
      T& ref() noexcept
      {
        return m_value;
      }
      const T& cref() const noexcept
      {
        return m_value;
      }
      T&& rref() noexcept
      {
        return std::move(m_value);
      }

      void set(const T& t)
      {
        m_value = t;
      }
      void set(T&& t)
      {
        m_value = move_if_noexcept_assignable(t);
      }

    private:

      T m_value;
    };

    template <typename T>
    class storage<T&>
    {
    public:

      template<typename TT, typename Guard>
      explicit storage(TT&& t, Guard&& guard)
        : m_value(t)
      {
        guard.release();
      }
      template<typename TT>
      explicit storage(TT&& t)
          : m_value(std::forward<TT>(t))
      {

      }

      T& ref() noexcept
      {
        return m_value.get();
      }
      const T& cref() const noexcept
      {
        return m_value.get();
      }
      T& rref() noexcept
      {
        return m_value.get();
      }

      void set(T& t)
      {
        m_value = t;
      }

    private:

      std::reference_wrapper<T> m_value;
    };

    //========================================================================
    // internal class : basic_scope_guard
    //========================================================================

    template <typename Fn, typename ExitPolicy>
    class basic_scope_guard : public ExitPolicy
    {
    public:
      template <typename ExitFunction>
      explicit basic_scope_guard(ExitFunction&& fn)
        noexcept(std::is_nothrow_move_constructible<Fn>::value)
        : ExitPolicy{},
          m_function{std::forward<ExitFunction>(fn)}
      {

      }

      basic_scope_guard(basic_scope_guard&& other)
        noexcept(std::is_nothrow_move_constructible<Fn>::value)
        : ExitPolicy{other},
          m_function{other.m_function.rref(), other}
      {

      }

      ~basic_scope_guard()
        noexcept(noexcept(std::declval<Fn&>()()))
      {
        if (should_execute()) {
          m_function.ref()();
        }
      }

      basic_scope_guard(const basic_scope_guard&) = delete;
      basic_scope_guard& operator=(const basic_scope_guard&) = delete;
      basic_scope_guard& operator=(basic_scope_guard&&) = delete;

      //----------------------------------------------------------------------
      // Modifiers
      //----------------------------------------------------------------------
    public:

      using ExitPolicy::release;

      //----------------------------------------------------------------------
      // Observers
      //----------------------------------------------------------------------
    public:

      using ExitPolicy::should_execute;

    private:

      storage<Fn> m_function;
    };

  } // namespace detail

  //==========================================================================
  // class : scope_exit<Fn>
  //==========================================================================

  ////////////////////////////////////////////////////////////////////////////
  /// \brief An exit handler for handling both sucess an error cases
  ///
  /// This will always execute the stored function unless 'release' has been
  /// called.
  ///
  /// When working in C++17 and above, you can use the automatic class
  /// type-deduction in order to create an instance of this:
  ///
  /// \code
  /// auto guard = scope::scope_exit{[&]{
  ///   ...
  /// }};
  /// \endcode
  ///
  /// When working in any C++11 or C++14, convenience functions are also
  /// added to perform this deduction for you:
  ///
  /// \code
  /// auto guard = scope::make_scope_exit([&]{
  ///   ...
  /// });
  /// \endcode
  ///
  /// \tparam Fn the function type for the scope to execute
  ////////////////////////////////////////////////////////////////////////////
  template <typename Fn>
  class scope_exit
    : private detail::basic_scope_guard<Fn,detail::on_exit_policy>
  {
    using base_type = detail::basic_scope_guard<Fn,detail::on_exit_policy>;

  public:

    using base_type::base_type;

    using base_type::release;

    using base_type::should_execute;
  };

#if __cplusplus >= 201703L
  template <typename Fn>
  scope_exit(Fn) -> scope_exit<Fn>;
#endif

  //==========================================================================
  // non-member functions : class : scope_exit<Fn>
  //==========================================================================

  //--------------------------------------------------------------------------
  // Utilities
  //--------------------------------------------------------------------------

  /// \brief A convenience function for pre-C++17 compilers that deduces
  ///        the underlying function type of the \c scope_exit
  ///
  /// It is recommended to always store the result of this into an \c auto
  /// variable:
  ///
  /// \code
  /// auto guard = scope::make_scope_exit([&]{
  ///   ...
  /// });
  /// \endcode
  ///
  /// \param fn the function to create
  /// \return an instance of the scope_exit
  template <typename Fn>
  SCOPE_NODISCARD
  scope_exit<typename std::decay<Fn>::type>
    make_scope_exit(Fn&& fn);

  //==========================================================================
  // class : scope_success<Fn>
  //==========================================================================

  ////////////////////////////////////////////////////////////////////////////
  /// \brief An exit handler for handling non-throwing cases
  ///
  /// This will execute as long as an exception has not been thrown in the
  /// same frame the scope was created in. You can manually disengage
  /// executing this handler by calling \c release.
  ///
  /// When working in C++17 and above, you can use the automatic class
  /// type-deduction in order to create an instance of this:
  ///
  /// \code
  /// auto guard = scope::scope_success{[&]{
  ///   ...
  /// }};
  /// \endcode
  ///
  /// When working in any C++11 or C++14, convenience functions are also
  /// added to perform this deduction for you:
  ///
  /// \code
  /// auto guard = scope::make_scope_success([&]{
  ///   ...
  /// });
  /// \endcode
  ///
  /// \tparam Fn the function type for the scope to execute
  ////////////////////////////////////////////////////////////////////////////
  template <typename Fn>
  class scope_success
    : private detail::basic_scope_guard<Fn,detail::on_success_policy>
  {
    using base_type = detail::basic_scope_guard<Fn,detail::on_success_policy>;

  public:

    using base_type::base_type;

    using base_type::release;

    using base_type::should_execute;
  };

#if __cplusplus >= 201703L
  template <typename Fn>
  scope_success(Fn) -> scope_success<Fn>;
#endif

  //==========================================================================
  // non-member functions : class : scope_success<Fn>
  //==========================================================================

  //--------------------------------------------------------------------------
  // Utilities
  //--------------------------------------------------------------------------

  /// \brief A convenience function for pre-C++17 compilers that deduces
  ///        the underlying function type of the \c scope_success
  ///
  /// It is recommended to always store the result of this into an \c auto
  /// variable:
  ///
  /// \code
  /// auto guard = scope::make_scope_success([&]{
  ///   ...
  /// });
  /// \endcode
  ///
  /// \param fn the function to create
  /// \return an instance of the scope_success
  template <typename Fn>
  SCOPE_NODISCARD
  scope_success<typename std::decay<Fn>::type>
    make_scope_success(Fn&& fn);

  //==========================================================================
  // class : scope_fail<Fn>
  //==========================================================================

  ////////////////////////////////////////////////////////////////////////////
  /// \brief An exit handler for handling throwing cases
  ///
  /// This will execute when an exception has been thrown in the same frame
  /// the scope was created in. You can manually disengage executing this
  /// handler by calling \c release.
  ///
  /// When working in C++17 and above, you can use the automatic class
  /// type-deduction in order to create an instance of this:
  ///
  /// \code
  /// auto guard = scope::scope_fail{[&]{
  ///   ...
  /// }};
  /// \endcode
  ///
  /// When working in any C++11 or C++14, convenience functions are also
  /// added to perform this deduction for you:
  ///
  /// \code
  /// auto guard = scope::make_scope_fail([&]{
  ///   ...
  /// });
  /// \endcode
  ///
  /// \tparam Fn the function type for the scope to execute
  ////////////////////////////////////////////////////////////////////////////
  template <typename Fn>
  class scope_fail
    : private detail::basic_scope_guard<Fn,detail::on_fail_policy>
  {
    using base_type = detail::basic_scope_guard<Fn,detail::on_fail_policy>;

  public:

    using base_type::base_type;

    using base_type::release;

    using base_type::should_execute;
  };

#if __cplusplus >= 201703L
  template <typename Fn>
  scope_fail(Fn) -> scope_fail<Fn>;
#endif

  //==========================================================================
  // non-member functions : class : scope_fail<Fn>
  //==========================================================================

  //--------------------------------------------------------------------------
  // Utilities
  //--------------------------------------------------------------------------

  /// \brief A convenience function for pre-C++17 compilers that deduces
  ///        the underlying function type of the \c scope_fail
  ///
  /// It is recommended to always store the result of this into an \c auto
  /// variable:
  ///
  /// \code
  /// auto guard = scope::make_scope_fail([&]{
  ///   ...
  /// });
  /// \endcode
  ///
  /// \param fn the function to create
  /// \return an instance of the scope_fail
  template <typename Fn>
  SCOPE_NODISCARD
  scope_fail<typename std::decay<Fn>::type>
    make_scope_fail(Fn&& fn);

  //==========================================================================
  // class : unique_resource<R,D>
  //==========================================================================

  ////////////////////////////////////////////////////////////////////////////
  /// \brief A resource with unique-ownership semantics
  ///
  /// This wraps and owns the specified resource R and deletes it at the
  /// end of its lifetime with D.
  ///
  /// This utility is extremely useful for wrapping old C-style functions
  /// that return raw resources like pointers, or integer handles, since
  /// you can couple the destruction of the resource safely inside this
  /// class. For example:
  ///
  /// \code
  /// auto resource = scope::unique_resource{::open(...), &::close};
  /// \endcode
  ///
  /// Or if you are pre-C++17, you can use:
  ///
  /// \code
  /// auto resource = scope::make_unique_resource(::open(...), &::close);
  /// \endcode
  ///
  /// \tparam R The resource type
  /// \tparam D The deleter type
  ////////////////////////////////////////////////////////////////////////////
  template <typename R, typename D>
  class unique_resource
  {
    static_assert(
      (std::is_move_constructible<R>::value &&
       std::is_nothrow_move_constructible<R>::value) ||
      std::is_copy_constructible<R>::value,
      "unique_resource must be nothrow_move_constructible or copy_constructible"
    );
    static_assert(
      (std::is_move_constructible<R>::value &&
       std::is_nothrow_move_constructible<D>::value) ||
       std::is_copy_constructible<D>::value,
      "deleter must be nothrow_move_constructible or copy_constructible"
    );

    template <typename T, typename TT>
    using is_nothrow_move_or_copy_constructible_from
      = std::integral_constant<bool,(std::is_reference<TT>::value ||
         !std::is_nothrow_constructible<T,TT>::value)
       ? std::is_constructible<T, TT const &>::value
       : std::is_constructible<T, TT>::value>;

    template <typename T, typename U, typename TT, typename UU>
    using enable_constructor_t = typename std::enable_if<
      is_nothrow_move_or_copy_constructible_from<T,TT>::value &&
      is_nothrow_move_or_copy_constructible_from<U,UU>::value
    >::type;

    //------------------------------------------------------------------------
    // Constructors / Destructor / Assignment
    //------------------------------------------------------------------------
  public:

    using resource_type = typename std::remove_reference<R>::type;
    using deleter_type  = typename std::remove_reference<D>::type;

    // Deleted default-constructor
    unique_resource() = delete;

    /// \brief Constructs a unique_resource from a given \p r resource and the
    ///        \p deleter
    ///
    /// \param r the resource to own uniquely
    /// \param d the deleter to destroy the resource
    template <typename RR, typename DD,
              typename = enable_constructor_t<R,D,RR,DD>>
    explicit unique_resource(RR&& r, DD&& d)
      noexcept(std::is_nothrow_constructible<R, RR>::value &&
               std::is_nothrow_constructible<D, DD>::value);

    /// \brief Constructs a unique_resource by moving the contents of an
    ///        existing resource
    ///
    /// The contents of the moved resource are now owned by this resource
    ///
    /// \param other the othe resource to move
    unique_resource(unique_resource&& other)
      noexcept(std::is_nothrow_move_constructible<R>::value &&
               std::is_nothrow_move_constructible<D>::value);

    // Deleted copy constructor
    unique_resource(const unique_resource& other) = delete;

    //------------------------------------------------------------------------

    /// \brief Equivalent to invoking \c reset()
    ~unique_resource();

    //------------------------------------------------------------------------

    /// \brief
    ///
    /// \param other
    /// \return \c reference to (*this)
    unique_resource& operator=(unique_resource&& other)
      noexcept(std::is_nothrow_assignable<R&, R>::value &&
               std::is_nothrow_assignable<D&, D>::value);

    // Deleted copy assignment
    unique_resource& operator=(const unique_resource& other) = delete;

    //------------------------------------------------------------------------
    // Modifiers
    //------------------------------------------------------------------------
  public:

    /// \brief Resets the resource managed by this unique_resource
    ///
    /// This will invoke the stored deleter on the resource, if the resource
    /// has not already executed.
    void reset() noexcept;

    /// \brief Resets the resource managed by this unique_resource to the
    ///        newly specified \p r resource
    ///
    /// The old resource is deleted with the stored deleter and replaced by
    /// the new one.
    ///
    /// \param r the new resource to store
    template <typename RR>
    void reset(RR&& r);

    /// \brief Releases the underlying stored resource from being managed by
    ///        this unique_resource class
    ///
    /// \note by calling this function, the deleter of this class is no
    ///       longer going to be invoked -- making it the responsibility of
    ///       the caller to take care of this instead.
    void release() noexcept;

    //------------------------------------------------------------------------
    // Observers
    //------------------------------------------------------------------------
  public:

    /// \brief Gets a reference to the underlying resource
    ///
    /// \return a reference to the underlying resource
    const resource_type& get() const noexcept;

    /// \brief Gets a reference to the underlying deleter
    ///
    /// \return a reference to the underlying deleter
    const deleter_type& get_deleter() const noexcept;

    template<typename RR=R>
    typename std::enable_if<std::is_pointer<RR>::value,typename std::remove_pointer<R>::type&>::type
      operator*() const noexcept;

    template<typename RR=R>
    typename std::enable_if<std::is_pointer<RR>::value,R>::type
      operator->() const noexcept;

    //------------------------------------------------------------------------
    // Private Members
    //------------------------------------------------------------------------
  private:

    detail::storage<R> m_resource;
    detail::storage<D> m_deleter;
    bool m_execute_on_destruction;

    //------------------------------------------------------------------------
    // Private Member Functions
    //------------------------------------------------------------------------
  private:

    template <typename RR, typename DD>
    unique_resource(RR&& r, DD&& d, bool execute_on_destruction);

    void do_assign(std::true_type, std::true_type, unique_resource&& other);
    void do_assign(std::false_type, std::true_type, unique_resource&& other);
    void do_assign(std::true_type, std::false_type, unique_resource&& other);
    void do_assign(std::false_type, std::false_type, unique_resource&& other);

    template <typename T, typename U, typename S>
    friend unique_resource<typename std::decay<T>::type, typename std::decay<U>::type>
    make_unique_resource_checked(T&&, const S&, U&&);
//      noexcept(std::is_nothrow_constructible<typename std::decay<T>::type, T>::value &&
//               std::is_nothrow_constructible<typename std::decay<U>::type, U>::value);

  };

#if __cplusplus >= 201703L

  // Template deduction guidelines
  template<typename R, typename D>
  unique_resource(R, D) -> unique_resource<R, D>;

#endif /* __cplusplus >= 201703L */

  //==========================================================================
  // non-member functions : class : unique_resource<R,D>
  //==========================================================================

  //--------------------------------------------------------------------------
  // Utilities
  //--------------------------------------------------------------------------

  /// \brief
  ///
  /// Example:
  ///
  /// \code
  /// auto file = make_unique_resource_checked(
  ///   ::fopen("potentially_nonexistent_file.txt", "r"),
  ///   nullptr,
  ///   &::fclose
  /// );
  /// \endcode
  ///
  /// \param resource The resource to manager
  /// \param invalid The invalid resource to not delete
  /// \param d The deleter for the resource
  /// \return the constructed unique_resource
  template <typename R, typename D, typename S=R>
  SCOPE_NODISCARD
  unique_resource<typename std::decay<R>::type, typename std::decay<D>::type>
  make_unique_resource_checked(R&& resource, const S& invalid, D&& d);
//    noexcept(std::is_nothrow_constructible<typename std::decay<R>::type, R>::value &&
//             std::is_nothrow_constructible<typename std::decay<D>::type, D>::value);

  /// \brief Makes a unique_resource from the specified \p resource and
  ///        deleter \p d
  ///
  /// This function exists to allow for the unique_resource to be
  /// type-reduced.
  ///
  /// Example:
  ///
  /// \code
  /// auto file = make_unique_resource(
  ///   ::fopen("potentially_nonexistent_file.txt", "r"),
  ///   &::fclose
  /// );
  /// \endcode
  ///
  /// \param resource The resource to manager
  /// \param d The deleter for the resource
  /// \return the constructed unique_resource
  template <typename R, typename D>
  SCOPE_NODISCARD
  unique_resource<typename std::decay<R>::type, typename std::decay<D>::type>
  make_unique_resource(R&& resource, D&& d)
    noexcept(std::is_nothrow_constructible<typename std::decay<R>::type, R>::value &&
             std::is_nothrow_constructible<typename std::decay<D>::type, D>::value);

} // namespace scope

template <typename Fn>
scope::scope_exit<typename std::decay<Fn>::type>
  scope::make_scope_exit(Fn&& fn)
{
  using result_type = scope_exit<typename std::decay<Fn>::type>;

  return result_type{std::forward<Fn>(fn)};
}

template <typename Fn>
scope::scope_success<typename std::decay<Fn>::type>
  scope::make_scope_success(Fn&& fn)
{
  using result_type = scope_success<typename std::decay<Fn>::type>;

  return result_type{std::forward<Fn>(fn)};
}

template <typename Fn>
scope::scope_fail<typename std::decay<Fn>::type>
  scope::make_scope_fail(Fn&& fn)
{
  using result_type = scope_fail<typename std::decay<Fn>::type>;

  return result_type{std::forward<Fn>(fn)};
}

//============================================================================
// definition : class : unique_resource
//============================================================================

//----------------------------------------------------------------------------
// Constructors / Destructor / Assignment
//----------------------------------------------------------------------------

template <typename R, typename D>
template <typename RR, typename DD, typename>
inline scope::unique_resource<R,D>::unique_resource(RR&& r, DD&& d)
  noexcept(std::is_nothrow_constructible<R, RR>::value &&
           std::is_nothrow_constructible<D, DD>::value)
  : unique_resource{std::forward<RR>(r), std::forward<DD>(d), true}
{

}

template <typename R, typename D>
inline scope::unique_resource<R,D>::unique_resource(unique_resource&& other)
  noexcept(std::is_nothrow_move_constructible<R>::value &&
            std::is_nothrow_move_constructible<D>::value)
  : m_resource{other.m_resource.rref(), make_scope_exit([]{})},
    m_deleter{other.m_deleter.rref(), make_scope_exit([&other,this]
    {
      if (other.m_execute_on_destruction) {
        auto& deleter = m_deleter.ref();
        deleter(m_resource.ref());
        other.release();
      }
    })},
    m_execute_on_destruction{other.m_execute_on_destruction}
{
  other.m_execute_on_destruction = false;
}
//----------------------------------------------------------------------------

template <typename R, typename D>
inline scope::unique_resource<R,D>::~unique_resource()
{
  reset();
}

//----------------------------------------------------------------------------

template <typename R, typename D>
inline scope::unique_resource<R,D>&
  scope::unique_resource<R,D>::operator=(unique_resource&& other)
  noexcept(std::is_nothrow_assignable<R&, R>::value &&
            std::is_nothrow_assignable<D&, D>::value)
{
  if (&other == this ) return (*this);

  do_assign(
    std::is_nothrow_move_assignable<R>{},
    std::is_nothrow_move_assignable<D>{},
    std::move(other)
  );
  m_execute_on_destruction = other.m_execute_on_destruction;
  other.m_execute_on_destruction = false;
  return (*this);
}

//----------------------------------------------------------------------------
// Modifiers
//----------------------------------------------------------------------------

template <typename R, typename D>
inline void scope::unique_resource<R,D>::reset()
  noexcept
{
  if (m_execute_on_destruction) {
    m_execute_on_destruction = false;

    auto& deleter = m_deleter.ref();
    deleter( m_resource.ref() );
  }
}

template <typename R, typename D>
template <typename RR>
inline void scope::unique_resource<R,D>::reset(RR&& r)
{
  auto guard = make_scope_fail([&r, this]{
    auto& deleter = m_deleter.ref();
    deleter(r);
  });
  (void) guard;

  reset();
  m_resource.set( std::forward<RR>(r) );
  m_execute_on_destruction = true;
}


template <typename R, typename D>
inline void scope::unique_resource<R,D>::release()
  noexcept
{
  m_execute_on_destruction = false;
}

//----------------------------------------------------------------------------
// Observers
//----------------------------------------------------------------------------

template <typename R, typename D>
inline const typename scope::unique_resource<R,D>::resource_type&
  scope::unique_resource<R,D>::get()
  const noexcept
{
  return m_resource.cref();
}

template <typename R, typename D>
inline const typename scope::unique_resource<R,D>::deleter_type&
  scope::unique_resource<R,D>::get_deleter()
  const noexcept
{
  return m_deleter.cref();
}

template <typename R, typename D>
template<typename RR>
inline typename std::enable_if<std::is_pointer<RR>::value,typename std::remove_pointer<R>::type&>::type
  scope::unique_resource<R,D>::operator*()
  const noexcept
{
  return (*get());
}

template <typename R, typename D>
template<typename RR>
inline typename std::enable_if<std::is_pointer<RR>::value,R>::type
  scope::unique_resource<R,D>::operator->()
  const noexcept
{
  return get();
}


//----------------------------------------------------------------------------
// Private Member Functions
//----------------------------------------------------------------------------

template <typename R, typename D>
template <typename RR, typename DD>
inline scope::unique_resource<R,D>::unique_resource(RR&& r,
                                                    DD&& d,
                                                    bool execute_on_destruction)
  : m_resource{std::forward<RR>(r), make_scope_exit([&d,&r]
    {
      d(r);
    })},
    m_deleter{std::forward<DD>(d), make_scope_exit([&d,this]{
      d(m_resource.ref());
    })},
    m_execute_on_destruction{execute_on_destruction}
{

}

//----------------------------------------------------------------------------

template <typename R, typename D>
inline void scope::unique_resource<R,D>::do_assign(std::true_type,
                                                   std::true_type,
                                                   unique_resource&& other)
{
  m_resource = std::move(other.resource);
  m_deleter = std::move(other.deleter);
}

template <typename R, typename D>
inline void scope::unique_resource<R,D>::do_assign(std::false_type,
                                                   std::true_type,
                                                   unique_resource&& other)
{
  m_resource = detail::as_const(other.resource);
  m_deleter = std::move(other.deleter);
}

template <typename R, typename D>
inline void scope::unique_resource<R,D>::do_assign(std::true_type,
                                                   std::false_type,
                                                   unique_resource&& other)
{
  m_resource = std::move(other.resource);
  m_deleter = detail::as_const(other.deleter);
}

template <typename R, typename D>
inline void scope::unique_resource<R,D>::do_assign(std::false_type,
                                                   std::false_type,
                                                   unique_resource&& other)
{
  m_resource = detail::as_const(other.resource);
  m_deleter = detail::as_const(other.deleter);
}

//============================================================================
// definitions : non-member functions : class : unique_resource
//============================================================================

//----------------------------------------------------------------------------
// Utilities
//----------------------------------------------------------------------------

template <typename R, typename D, typename S>
inline scope::unique_resource<typename std::decay<R>::type, typename std::decay<D>::type>
  scope::make_unique_resource_checked(R&& resource, const S& invalid, D&& d)
//  noexcept(std::is_nothrow_constructible<typename std::decay<R>::type, R>::value &&
//           std::is_nothrow_constructible<typename std::decay<D>::type, D>::value)
{
  using resource_type = typename std::decay<R>::type;
  using deleter_type  = typename std::decay<D>::type;
  using result_type   = scope::unique_resource<resource_type, deleter_type>;

  const auto execute_on_destruction = !(resource == invalid);

  return result_type{
    std::forward<R>(resource),
    std::forward<D>(d),
    execute_on_destruction
  };
}

template <typename R, typename D>
inline scope::unique_resource<typename std::decay<R>::type, typename std::decay<D>::type>
  scope::make_unique_resource(R&& resource, D&& d)
  noexcept(std::is_nothrow_constructible<typename std::decay<R>::type, R>::value &&
           std::is_nothrow_constructible<typename std::decay<D>::type, D>::value)
{
  using resource_type = typename std::decay<R>::type;
  using deleter_type  = typename std::decay<D>::type;
  using result_type   = scope::unique_resource<resource_type, deleter_type>;

  return result_type{
    std::forward<R>(resource),
    std::forward<D>(d)
  };
}

#endif /* SCOPE_SCOPE_HPP */