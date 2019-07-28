
#include <scope/scope.hpp>

namespace {
  void example_deleter(int&){}
}

int main()
{
  {
    auto scope = ::scope::make_scope_exit([]{

    });
    (void) scope;
  }
  {
    auto scope = ::scope::make_scope_fail([]{

    });
    (void) scope;
  }
  {
    auto scope = ::scope::make_scope_success([]{

    });
    (void) scope;
  }
  {
    auto resource = ::scope::make_unique_resource(int{5}, &::example_deleter);

    (void) resource;
  }

  return 0;
}