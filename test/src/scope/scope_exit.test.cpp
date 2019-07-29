#include "scope/scope.hpp"

#include <catch2/catch.hpp>
#include <exception>

#if __cplusplus >= 201703L
TEST_CASE("scope_exit::scope_exit(Fn&&)", "[ctor]")
{
  auto called = false;

  SECTION("Scope exit success")
  {
    SECTION("Release not called")
    {
      {
        auto scope = ::scope::scope_exit([&called] {
          called = true;
        });
        (void) scope;
      }
      REQUIRE(called);
    }

    SECTION("Release called")
    {
      {
        auto scope = ::scope::scope_exit([&called] {
          called = true;
        });
        scope.release();
      }
      REQUIRE(!called);
    }
  }

  SECTION("Scope exit failure")
  {
    SECTION("Release not called")
    {
      try {
        auto scope = ::scope::scope_exit([&called] {
          called = true;
        });
        (void) scope;
        throw std::exception{};
      } catch (...) {}
      REQUIRE(called);
    }

    SECTION("Release called")
    {
      try {
        auto scope = ::scope::scope_exit([&called] {
          called = true;
        });
        scope.release();
        throw std::exception{};
      } catch (...) {}
      REQUIRE(!called);
    }
  }
}
#endif

TEST_CASE("make_scope_exit(Fn&&)", "utilities")
{
  auto called = false;

  SECTION("Scope exit success")
  {
    SECTION("Release not called")
    {
      {
        auto scope = ::scope::make_scope_exit([&called] {
          called = true;
        });
        (void) scope;
      }
      REQUIRE(called);
    }

    SECTION("Release called")
    {
      {
        auto scope = ::scope::make_scope_exit([&called] {
          called = true;
        });
        scope.release();
      }
      REQUIRE(!called);
    }
  }

  SECTION("Scope exit failure")
  {
    SECTION("Release not called")
    {
      try {
        auto scope = ::scope::make_scope_exit([&called] {
          called = true;
        });
        (void) scope;
        throw std::exception{};
      } catch (...) {}
      REQUIRE(called);
    }

    SECTION("Release called")
    {
      try {
        auto scope = ::scope::make_scope_exit([&called] {
          called = true;
        });
        scope.release();
        throw std::exception{};
      } catch (...) {}
      REQUIRE(!called);
    }
  }
}