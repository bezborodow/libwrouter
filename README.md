# Wrouter: Symbolic Web Router

URL router.

## Usage

### Python

```python
from wrouter import build_router, Dispatcher


routes = [
    ("/account", "account.list"),
    ("/account/create", "account.create"),
    ("/account/a/:account_id", "account.view")
]

dispatcher = Dispatcher(build_router(routes))

endpoint, params = dispatcher.resolve("/account/a/1234")

print(f"{endpoint} {params['account_id']}")
```

Will print: `account.view 1234`.

Install from [PyPI](https://pypi.org/project/wrouter/0.1.0/) with `pip install wrouter`.

### Erlang

```erlang
Routes = [
    {<<"/account">>, account_list},
    {<<"/account/:id">>, account_view},
    {<<"/posts/:id">>, fun handle_post/2}
],

{ok, Router} = wrouter:new(Routes),
{ok, Handler, Params} = wrouter:resolve(Router, <<"/account/1234">>).
```

`Handler` is the route context term supplied when building the router. It can
be an atom, tuple, map, function, or any other Erlang term. Parameters are
returned as a map of binaries.

Install via [Hex](https://hex.pm/packages/wrouter) with:

```erlang
{deps, [{wrouter, "1.0.1"}]}.
```

### C++

```
#include "wrouter.hpp"

#include <iostream>
#include <string>

struct Response {
    std::string body;
};

int main()
{
    wrouter::Builder<Response> builder;

    builder.add("/hello/:name", [](Response& response, wrouter::Params params) {
        response.body = "Hello, " + params["name"] + "!";
    });

    auto router = builder.consume();
    wrouter::Dispatcher dispatcher(router);

    Response response;
    dispatcher.dispatch("/hello/world", response);

    std::cout << response.body << "\n";
}
```

### C

```c
#include <stdio.h>
#include <wrouter.h>

static void route_hello(void *dispatch_ctx, const void *route_ctx, const wrouter_params_t *params)
{
    const wrouter_param_t *addressee = wrouter_param(params, "addressee");

    if (addressee)
        printf("Hello, %.*s!\n", addressee->length, addressee->value);
}

int main(void)
{
    wrouter_error_t err;
    wrouter_options_t options = { 0 };
    wrouter_builder_t *builder = NULL;
    wrouter_t *router = NULL;
    wrouter_dispatcher_t *dispatcher = NULL;

    builder = wrouter_builder_create(options);
    if (builder == NULL)
        goto failure;

    err = wrouter_add_handler(builder, "/hello/:addressee", route_hello);
    if (err)
        goto failure;

    router = wrouter_consume(&builder, &err);
    if (err)
        goto failure;

    dispatcher = wrouter_dispatcher_create(router);
    if (dispatcher == NULL)
        goto failure;

    wrouter_dispatch(dispatcher, "/hello/world", NULL);

    wrouter_dispatcher_destroy(&dispatcher);
    wrouter_destroy(&router);

    return 0;

failure:
    fprintf(stderr, "%s\n", wrouter_strerror(err));
    wrouter_builder_destroy(&builder);
    wrouter_dispatcher_destroy(&dispatcher);
    wrouter_destroy(&router);
    return 1;
}
```

Compile with:

```bash
gcc -o hello hello.c -lwrouter
```

Will print: `Hello, world!`

## Concepts

This project is designed as a router for use in a Web application server that
is assumed to be behind an HTTP proxy. As such, it makes no attempt to handle
hostnames, subdomains, or aliases. The router does not handle HTTP parsing or
request handling, and as such should be used in conjunction with other
libraries.

The router is intended to be fast, cache-efficient, and deterministic. It is
therefore deliberately restrictive in what forms of routes can be accepted into
the routing graph (see constraints below). This results in a graph that is
simple and easy to traverse efficiently. The strict routing graph prevents
ambiguity in route resolution, avoiding the need for prioritisation or
resolving the specificity of conflicting routes.

The router has no concept of HTTP methods such as GET and POST. Therefore, a
router must be instantiated for each method supported by the application,
including a separate router for WebSockets, if desired.

The router is immutable, and is therefore thread-safe, and may be shared
between threads. The dispatcher is mutable and must not be shared between
threads.

## Constraints

A URL is divided into segments by the `/` character. A segment maybe either be
a literal string, parameter, or wildcard. Parameters are denoted by a colon
(default), angle, or brace; e.g., `:param`, `<param>`, or `{param}`,
respectively.

Wildcards (`*`) may only appear at the end of a route. Wildcards are evaluated
as a fallback. The most specific wildcard will match.  A wildcard will consume
all segments following it.

 - `/accounts/user/*/view` (invalid!)
 - `/accounts/downloads/*`
 - `/*`

A segment may only be a parameter or a literal.

 - `/board/:board_id/ticket:ticket_id` (valid, but `ticket:ticket_id` will be parsed as a literal!)
 - `/board/:board_id/ticket/:ticket_id`

Routes must not conflict with parameters and literals at the same level.

 - `/project/list` and - `/project/:project_id` (incompatible!)

Wildcards are acceptable within at the same level at the end if they are
opposing a literal, not a parameter.

 - `/project/list` and `/project/*`
 - `/project/:project_id` and `/project/*` (incompatible!)

Parameters at the same level must share the same name.

 - `/repos/:user/:repo` and `/repos/:user/:repo/tree/:branch/*`
 - `/project/:id` and `/project/:project_id/accounts/:account_id` (incompatible!)

Routes with trailing slashes are distinct. The following are not equivalent:

 - `/test`
 - `/test/`

If it is desired to redirect from `/test` to `/test/`, this must be performed
explicitly by adding both routes, of which the former will redirect to the
latter.

Flexibility is not a goal of this project.

Where these constraints are severely limiting, the HTTP proxy should rewrite
URLs for the application server to consume. For example, `/new` could be
rewritten as `/repo/new`, while `/:user/:repo` could be rewritten as
`/repos/:user/:repo`. A small rewrite engine might be included in the future.

## Limitations

This is not an HTTP parser; as such, request paths containing query parameters
such as `/foo/bar?query=hi` will fail to match.

## Building

Compile:

```bash
make
```

Run tests:

```bash
make test
```

Install:

```bash
sudo make install
```
