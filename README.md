# The hhev library

I will keep here my useful headers :)

## params\_setters.hpp

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

## pipeline.hpp

This header can be useful when you need to make some verifications, transformations and want to return something or do something in the end. For example, you want to handle an HTTP request with the Poco library. The simple way to do it is writing few "if" construction:

```cpp
void handler(HTTPServerRequest& request, HTTPServerResponse& response)
{
    if (request.getMethod() != "GET")
    {
        std::string err = "The " +request.getMethod() + " method is now allowed.";
        response.setStatus(HTTPResponse::HTTP_METHOD_NOT_ALLOWED);
        response.setContentType("text/plain");
        response.setContentLength(err.size());
        response.send() << err;
    }

    auto query_params = Poco::URI(request.getURI()).getQueryParameters();
    auto it = std::find_if(query_params.begin(), query_params.end(),
            [](auto& pair) {return pair.firt == "page";});
    if (it == query_params.end())
    {
        std::string err =  "The \"page\" parameter isn't found.";
        response.setStatus(HTTPResponse::HTTP_FOUND);
        response.setContentType("text/plain");
        response.setContentLength(err.size());
        response.send() << err;
    }

    Poco::JSON::Object json;
    // ... filling json ...

    std::ostringstream oss;
    json.stringify(oss);
    response.setStatus(HTTPResponse::HTTP_OK);
    response.setContentType("application/json");
    response.setContentLength(oss.tellp());
    response.send() << oss.str();
}
```

If verification fails, you should send a response with some description. At the end of the function, you send the answer. It's OK if you have one handler, but it looks ugly if you should repeat it in some handlers.

Can we write it better? Of course, yes! You can throw an exception with error, and fill the response into the catch block,  or you can make some checkers and do set parameters of the response into them. In the handler, you can wrap the response object and the wrapper will call the send method from the destructor.

With "pipeline" you can write it like this:

```cpp
struct Context
{
    Context(HTTPServerRequest& req, HTTPServerResponse& res)
        : request(req)
        , response(res)
    {}

    HTTPServerRequest& request;
    HTTPServerResponse& response;
    std::map<std::string, std::string> params;
    std::string body;
    std::string content_type;
};

auto method_is(const std::string& method)
{
    return [method](Context& c) -> bool {
        if (c.request.getMethod() != method)
        {
            c.response.setStatus(HTTPResponse::HTTP_METHOD_NOT_ALLOWED);
            c.content_type = "text/plain";
            c.body = "The " + c.request.getMethod() + " method is now allowed.";
            return false;
        }
        return true;
    };
}

auto query_contains(const std::string& name)
{
    return [name](Context& c) -> bool {
        auto query_params = Poco::URI(c.request.getURI()).getQueryParameters();
        auto it = std::find_if(query_params.begin(), query_params.end(),
                [](auto& pair) {return pair.firt == "page";});

        if (it == query_params.end())
        {
            c.response.setStatus(HTTPResponse::HTTP_FOUND);
            c.body =  "The \"page\" parameter isn't found.";
            c.content_type = "text/plain";
        }

        c.params.emplace(it->first, it->second);
        return true;
    };
}

const auto send_response = auxiliary::pipeline::finish([](Context& c) -> void {
    c.response.setContentType(c.content_type);
    c.response.setContentLength(c.body.size());
    c.response.send() << c.body;
});

void handler(HTTPServerRequest& request, HTTPServerResponse& response)
{
    Context c {request, response};
    pipeline::start(c) | method_is("GET") | query_contains("page")
        | [](Context& c) -> bool {

        Poco::JSON::Object json;
        // ... filling json ...

        std::ostringstream oss;
        json.stringify(oss);
        c.response.setStatus(HTTPResponse::HTTP_OK);
        c.content_type = "application/json";
        c.body = oss.str();

        return true;
    } | send_response;
}
```

Now the handler looks a bit clearer.