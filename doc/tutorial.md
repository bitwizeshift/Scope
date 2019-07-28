# Tutorial

**{Scope}** is really simple-to-use library with a relatively small
API surface area.

Overall, it only exposes 4 types:

* `scope::scope_exit<Fn>`
* `scope::scope_success<Fn>`
* `scope::scope_fail<Fn>`
* `scope::unique_resource<R,D>`

each of which are described below.

## `scope::scope_exit<Fn>`

The `scope::scope_exit` class (and corresponding `scope::make_scope_exit`
function) are used to to defer execution of a specified function until the
end of a scope. The execution of this function is always guaranteed to occur,
even if the scope ends early due to an exception.

If you are using C++17 or above, you can use the template-deduction for
`scope_exit`, but in C++14 or below you will require `scope::make_scope_exit`
in order to achieve the same effect.

This type is useful for a number of things:

* Doing resource cleanup on several control-flow paths
* Logging on exit
* Computing runtime of a function that has several exit paths
* etc

**Note:** All scope guards MUST be assigned to a variable. The name
of this variable does not matter, but without it there will be no
RAII.

### Doing resource cleanup example

Managing resources that cannot afford to be leaked in any condition,
such as semaphores, are an excellent use-case for `scope_exit`:

```c++
{
  ::sem_lock(sem);

  // Always unlock semaphore at end of scope
  auto scope = ::scope::make_scope_exit([&]{
    ::sem_post(sem);
  });

  // ...

  // multiple return conditions that require unlocking
  if ( ... ) {
    return;
  }

  // ...

  return;
}
```

### Logging on exit example

If you are in a logging-heavy system or function, which is commonly seen
in highly asynchronous environments, it can be helpful to log the same message
at multiple exit points. This is made easy with `scope_exit`:

```c++
{
  auto scope = ::scope::make_scope_exit([&]{
    std::cout << "Leaving function now ..." << std::endl;
  });


  // ...

  // multiple return conditions that require the same log
  if ( ... ) {
    return;
  }

  // ...

}
```

### Computing runtime of a function example

```c++
{
  const auto start_time = std::chrono::steady_clock::now();

  auto scope = ::scope::make_scope_exit([&]{
    const auto end_time = std::chrono::steady_clock::now();
    const auto diff = end_time - start_time;
    do_something_with_calculated_time(diff);
  });


  // ...

  // multiple return conditions that require the same timing
  if ( ... ) {
    return;
  }

  // ...
}
```

## `scope::scope_success<Fn>`

Similar to `scope_exit`, `scope_success` will defer execution until
the end of the scope -- but instead it will only execute if the scope
is being left due to a normal condition, and not due to an exception
being thrown.

This has many potential uses, such as:

* Committing an action for atomicity (e.g. transactions)
* Recording/notifying successful conditions
* etc

**Note:** All scope guards MUST be assigned to a variable. The name
of this variable does not matter, but without it there will be no
RAII.

### Commiting an action atomically example

Sometimes you might have a complicated control flow that should
only ever post a value in successful states, but not in failure
states. This is where `scope_success` comes in:

```c++
{
  // copy the old state of things
  auto state = m_state.copy();

  auto scope = ::scope::make_scope_success([&state, this]{
    this->commit(state);
  });

  // do some potentially throwing stuff... that
  // modifies 'state'
}
```

### Recording/notifying successful conditions example

In some cases, you want to notify that a change has been made
to your code, and this information should be propagated to all
consumers. This could be done via logging, observers, signals,
etc -- but the key point is that someone is listening to this change.

This is where `scope_success` can be useful:

```c++
{
  auto scope = ::scope::make_scope_success([this]{
    this->notify_observers(this->m_some_value);
  });

  // try to change 'm_some_value', throws on error
}
```

## `scope::scope_fail<Fn>`

The `scope_fail` is effectively the inverse of `scope_success` in that
it will always defer execution until the end of the scope, but will only
execute if the scope was left due to an exception being in-flight.

This can be extremely useful for:

* Rolling back changes for atomic-transactions
* Notifying failure conditions
* etc

**Note:** All scope guards MUST be assigned to a variable. The name
of this variable does not matter, but without it there will be no
RAII.

### Rolling back changes example

This can be done in connection with `make_scope_success` to provide
full atomic transactions, where `scope_fail` will roll back changes on
failure, but `scope_success` will commit them!

```c++
{
  auto state = m_state.copy();

  auto scope_fail = ::scope::make_scope_fail([&state, this]{
    this->rollback(state);
  });
  auto scope_success = ::scope::make_scope_success([&state, this]{
    this->commit(state);
  });

  // do some potentially throwing stuff... that

  // modifies 'state'
}
```

### Notifying failure conditions example

It can always be useful to notify that a bad state has happened.
This can be done through logging, observers, signals, etc -- but
the key aspect is that something went bad, and someone else needs
to know about it. This is where `scope_fail` comes in:

```c++
{
  auto scope = ::scope::make_scope_fail([this]{
    this->notify_something_bad_happened();
  });

  // something that may throw on error
}
```

## `scope::unique_resource<R,D>`

`unique_resource` is a more generic version of `unique_ptr`. It has
unique ownership semantics, but can hold any arbitrary type `T` rather
than only pointers. This allows for interfacing directly with types that
distribute resources that must be deleted other calls. This works extremely
well with older C-style APIs that return handles and must be deleted on
cleanup.

`unique_resource` also provides two utility construction functions for deducing
the types, and for ensuring that the constructed value is not a _bad_ value.
For example, POSIX `::open` will return a file descriptor integer `-1` on failure,
which should not be `::close`d.

### Posix File Example

The simple example is interfacing with a Posix-style C API, such as the file API:

```c++
{
  auto file = ::scope::make_unique_resource_checked(
    ::open( ... ), // Creates the resource
    -1,            // The 'invalid' value (ensures no deletion)
    &::close       // function for deleting this
  );

  // do stuff with 'file'

}
```

### SDL2 Example

Realistically, any library that returns handls of any kind can be used by this
utility. For example, if you are using the SDL2 library, this can help manage
any pointers returned from their API calls:

```c++
{
  auto window = ::scope::make_unique_resource_checked(
    ::SDL_CreateWindow( ... ),
    nullptr,
    &::SDL_CloseWindow
  );

  // do stuff with 'window'

}
```

### More complicated examples

What if you have a more complicated example that isn't just a C API using a
function pointer? In that scenario, the deleter can be a lambda or functor
type that does some more complicated actions on callbacks.

Let's use a resource system from an imaginary game-engine as an example.

```c++
{
  auto shader = ::scope::make_unique_resource_checked(
    shader_resource_manager.get_resource(...),
    ShaderResourceManager::BadHandle,
    [&shader_resource_manager](ShaderResourceManager::Handle& handle){
      shader_resource_manager.dispose_resource(handle);
    }
  );

  // do something with 'shader'
}
```

Overall, this can help to reduce a lot of boilerplate that would otherwise
come with creating handle types for complicated systems, since it can now
be boiled down to two things: The resource value, and the system that
disposes it.