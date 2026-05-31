# Wrouter: Symbolic Web Router

**Work-in-progress!**

## Usage

### Python

```python
import wrouter

routes = [
    ("/account", "account.list"),
    ("/account/create", "account.create"),
    ("/account/a/:account_id", "account.view")
]

builder = wrouter.Builder()

for pattern, endpoint in routes:
    builder.add(pattern, endpoint)

router = builder.compile()
dispatcher = wrouter.Dispatcher(router)

endpoint, params = dispatcher.resolve("/account/a/1234")

print(f"{endpoint} {params['account_id']}")
```

### C

```c
#include <stdio.h>
#include <stdint.h>
#include <wrouter.h>

static void route_hello(void *dispatch_ctx, const void *route_ctx, const wrouter_params_t *params)
{
    const char *addressee = params->base[0].value;
    uint16_t addressee_len = params->base[0].length;

    printf("Hello, %.*s!\n", addressee_len, addressee);
}

int main(void)
{
    wrouter_options_t options = { 0 };

    wrouter_builder_t *builder = wrouter_builder_create(options);
    wrouter_add_handler(builder, "/hello/:addressee", route_hello);

    wrouter_t *router = wrouter_compile(builder);
    wrouter_builder_free(builder);

    wrouter_dispatcher_t *dispatcher = wrouter_dispatcher_create(router);

    wrouter_dispatch(dispatcher, "/hello/world", NULL);

    wrouter_dispatcher_free(dispatcher);
    wrouter_free(router);

    return 0;
}
```

Compile with:

```bash
gcc -o hello hello.c -lwrouter
```

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

The router has no concept of HTTP methods. Therefore, a router must be
instantiated for each method supported by the application, including a separate
router for WebSockets, if desired.

The router is immutable, and is therefore thread-safe, and may be shared
between threads. The dispatcher is mutable and must not be shared between
threads.

## Constraints

A URL is divided into segments by the `/` character.

Wildcards may only appear at the end of a route. Wildcards are evaluated as a
fallback.  A wildcard will consume all segments following it.

 - `/accounts/user/*/view` (invalid)
 - `/accounts/downloads/*`
 - `/*`

A segment may only be a parameter or a literal.

 - `/board/<board_id>/ticket/<ticket_id>`
 - `/board/<board_id>/ticket<ticket_id>` (invalid)

Routes must not conflict with parameters and literals at the same level.
These two routes are valid but incompatible:

 - `/project/list`
 - `/project/<project_id>`

Wildcards are acceptable within at the same level at the end if they are
opposing a literal, not a parameter.

Compatible:

 - `/project/list`
 - `/project/*`

Incompatible:

 - `/project/<project_id>`
 - `/project/*`

Routes with trailing slashes are (currently) treated the same. This will change
in the future, as this library is a work-in-progress. The following are
equivalent:

 - `/test/`
 - `/test`

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
