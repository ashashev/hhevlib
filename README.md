# The hhev library

I will keep here my useful headers :)

## params_setters.hpp

Guess you have a class with some setters. For example, the "HTTPServerParams" class has some methods:

 * `SetMaxQueued`
 * `SetMaxThreads`

If you want to set the params you should write something like following:

```cpp
Poco::Net::HTTPServerParams::Ptr params(new Poco::Net::HTTPServerParams());
params->SetMaxQueued(64);
params->SetMaxThreads(10);
//... use params ...
```

When you do it once, it's ok, but if you need it more you can write make-function and use it:

```cpp
auto params = makeHTTPServerParams(maxQueued = 64, maxThreads = 10);
//... use params ...
```

You can write also:

```cpp
auto params1 = makeHTTPServerParams(maxThreads = 10, maxQueued = 64);
auto params2 = makeHTTPServerParams(maxThreads = 10);
auto params3 = makeHTTPServerParams(maxQueued = 64);
```

The `params_setters.hpp` helps you to write this function:

```cpp
const auto maxQueued = params_setters::makeSetterFactory(
        &Poco::Net::HTTPServerParams::setMaxQueued);
const auto maxThreads = params_setters::makeSetterFactory(
        &Poco::Net::HTTPServerParams::setMaxThreads);

template<typename ... Setters>
Poco::Net::HTTPServerParams::Ptr makeHTTPServerParams(Setters&& ... setters)
{
    Poco::Net::HTTPServerParams::Ptr params(new Poco::Net::HTTPServerParams());
    params_setters::applySetters(params.get(), std::forward<Setters>(setters)...);
    return params;
}
```

