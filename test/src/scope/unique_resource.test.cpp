#include "scope/scope.hpp"

#include <catch2/catch.hpp>
#include <map>

namespace {

  void example_deleter( int ){}

  template <typename T>
  struct mock_resource {
    T value;
  };

  template <typename T>
  bool operator==(const mock_resource<T>& lhs, const mock_resource<T>& rhs)
  {
    return lhs.value == rhs.value;
  }
  template <typename T>
  bool operator<(const mock_resource<T>& lhs, const mock_resource<T>& rhs)
  {
    return lhs.value < rhs.value;
  }

  // Maps type to number of times deleter has been called
  template <typename T>
  using deleter_map = std::map<mock_resource<T>,int>;

  template <typename T>
  struct mock_deleter {
    ::deleter_map<int>* map;
    void operator()(mock_resource<T>& r) noexcept(false)
    {
      (*map)[r]++;
    }
  };

  using example_deleter_type = decltype(&::example_deleter);
}

TEST_CASE("unique_resource::unique_resource(R&&, D&&)", "[ctor]")
{
  SECTION("R and D are value types")
  {
    using resource_type = ::mock_resource<int>;
    using deleter_type  = ::mock_deleter<int>;
    using result_type = ::scope::unique_resource<resource_type,deleter_type>;

    const auto value = 42;
    auto map = ::deleter_map<int>{};

    auto resource = resource_type{value};
    auto deleter  = deleter_type{&map};

    auto r = result_type{resource, deleter};

    SECTION("Contains copy of resource value")
    {
      REQUIRE(r.get().value == resource.value);
    }
    SECTION("Contains copy of deleter")
    {
      REQUIRE(r.get_deleter().map == deleter.map);
    }
  }

  SECTION("R is reference and D is value type")
  {
    using resource_type = ::mock_resource<int>;
    using deleter_type  = ::mock_deleter<int>;
    using result_type = ::scope::unique_resource<resource_type&,deleter_type>;

    const auto value = 42;
    auto map = ::deleter_map<int>{};
    auto resource = resource_type{value};
    auto deleter  = deleter_type{&map};

    auto r = result_type{resource, deleter};

    SECTION("Contains reference to resource")
    {
      const auto& reference = r.get();
      REQUIRE(&reference.value == &resource.value);
    }
    SECTION("Contains copy of deleter")
    {
      REQUIRE(r.get_deleter().map == deleter.map);
    }
  }

  SECTION("R is value type and D is reference")
  {
    using resource_type = ::mock_resource<int>;
    using deleter_type  = ::mock_deleter<int>;
    using result_type = ::scope::unique_resource<resource_type,deleter_type&>;

    const auto value = 42;
    auto map = ::deleter_map<int>{};
    auto resource = resource_type{value};
    auto deleter  = deleter_type{&map};

    auto r = result_type{resource, deleter};

    SECTION("Contains copy of resource")
    {
      REQUIRE(r.get().value == resource.value);
    }
    SECTION("Contains reference to deleter")
    {
      REQUIRE(&r.get_deleter().map == &deleter.map);
    }
  }

  SECTION("R and D are reference types")
  {
    using resource_type = ::mock_resource<int>;
    using deleter_type  = ::mock_deleter<int>;
    using result_type = ::scope::unique_resource<resource_type&,deleter_type&>;

    const auto value = 42;

    auto map = ::deleter_map<int>{};
    auto resource = resource_type{value};
    auto deleter  = deleter_type{&map};

    auto r = result_type{resource, deleter};

    SECTION("Contains reference to resource")
    {
      REQUIRE(&r.get().value == &resource.value);
    }
    SECTION("Contains reference to deleter")
    {
      REQUIRE(&r.get_deleter().map == &deleter.map);
    }
  }
}

TEST_CASE("unique_resource::reset()", "[modifiers]")
{
  using resource_type = ::mock_resource<int>;
  using deleter_type  = ::mock_deleter<int>;

  SECTION("Resource is reset without new value")
  {
    const auto value = 42;
    auto map = ::deleter_map<int>{};
    auto resource = resource_type{value};
    auto deleter  = deleter_type{&map};

    {
      auto r = ::scope::make_unique_resource(
        resource,
        deleter
      );

      r.reset(); // should call delete
    }

    SECTION("Calls delete once")
    {
      REQUIRE(map[resource] == 1);
    }
    SECTION("Calls delete on only the one resource")
    {
      REQUIRE(map.size() == 1u);
    }
  }
}

TEST_CASE("unique_resource::reset(RR&&)", "[modifiers]")
{
  using resource_type = ::mock_resource<int>;
  using deleter_type  = ::mock_deleter<int>;

  SECTION("Resource is reset with new value")
  {
    const auto value = 42;
    const auto new_value = value + 1;

    auto map = ::deleter_map<int>{};
    auto resource = resource_type{value};
    auto new_resource = resource_type{new_value};
    auto deleter = deleter_type{&map};
    {
      auto r = ::scope::make_unique_resource(
        resource,
        deleter
      );

      r.reset(new_resource); // should call delete
    } // should call delete again

    SECTION("Calls delete on old resource once")
    {
      REQUIRE(map[resource] == 1);
    }
    SECTION("Calls delete on new resource once")
    {
      REQUIRE(map[new_resource] == 1);
    }
    SECTION("Calls deleter on only the two resources")
    {
      REQUIRE(map.size() == 2u);
    }
  }
}

TEST_CASE("unique_resource::release()", "[modifiers]")
{
  using resource_type = ::mock_resource<int>;
  using deleter_type  = ::mock_deleter<int>;

  SECTION("Not released")
  {
    const auto value = 42;

    auto map = ::deleter_map<int>{};
    auto resource = resource_type{value};
    auto deleter  = deleter_type{&map};

    {
      auto r = ::scope::make_unique_resource(
        resource,
        deleter
      );
      (void) r;
    } // should call delete on scope exit

    SECTION("Calls deleter on resource")
    {
      REQUIRE(map[resource] == 1);
    }
    SECTION("Calls deleter only once")
    {
      REQUIRE(map.size() == 1u);
    }
  }

  SECTION("Released")
  {
    const auto value = 42;

    auto map = ::deleter_map<int>{};
    auto resource = resource_type{value};
    auto deleter  = deleter_type{&map};

    {
      auto r = ::scope::make_unique_resource(
        resource,
        deleter
      );
      r.release();
    } // should not call delete on scope exit

    SECTION("Does not call deleter on any resource")
    {
      REQUIRE(map.empty());
    }
  }
}

TEST_CASE("make_unique_resource(R&&, D&&)")
{
  const auto value   = 5;
  const auto deleter = &::example_deleter;

  auto resource = ::scope::make_unique_resource(
    value,
    deleter
  );

  SECTION("Constructed resource contains value")
  {
    REQUIRE(resource.get() == value);
  }

  SECTION("Constructed resource contains deleter")
  {
    REQUIRE(resource.get_deleter() == deleter);
  }
}

TEST_CASE("make_unique_resource_checked(R&&, const S&, D&&)")
{
  using resource_type = ::mock_resource<int>;
  using deleter_type  = ::mock_deleter<int>;

  const auto invalid_value = -1;
  const auto invalid_resource = resource_type{invalid_value};

  SECTION("Constructed with invalid value")
  {
    auto map = ::deleter_map<int>{};
    auto resource = invalid_resource;
    auto deleter  = deleter_type{&map};
    {

      auto r = ::scope::make_unique_resource_checked(
        resource,
        invalid_resource,
        deleter
      );
      (void) r;
    } // should not call delete on scope exit

    SECTION("Destructor does not delete any resource")
    {
      REQUIRE(map.empty());
    }
  }

  SECTION("Constructed with valid value")
  {
    const auto value = 42;
    auto map = ::deleter_map<int>{};
    auto resource = resource_type{value};
    auto deleter  = deleter_type{&map};
    {

      auto r = ::scope::make_unique_resource_checked(
        resource,
        invalid_resource,
        deleter
      );
      (void) r;
    } // should call delete on scope exit
    SECTION("Destructor calls delete on resource")
    {
      REQUIRE(map[resource] == 1);
    }
    SECTION("Delete only one resource once")
    {
      REQUIRE(map.size() == 1u);
    }
  }
}