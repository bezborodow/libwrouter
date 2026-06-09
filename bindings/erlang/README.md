# Wrouter Erlang Bindings

Erlang NIF bindings for libwrouter.

## Installation

When published to Hex, add `wrouter` to `rebar.config`:

```erlang
{deps, [{wrouter, "1.0.0"}]}.
```

The package includes a native NIF and builds it with `make`, so users need
Erlang/OTP, rebar3, make, and a C compiler available at compile time.

## Usage

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

## Development

The primary project build uses Meson. The Hex/rebar3 package uses a small
Makefile shim because that is the usual Erlang NIF packaging path and keeps
downstream build requirements minimal.

To build and test the Erlang package locally:

```sh
rebar3 compile
rebar3 eunit
```

To publish, install `rebar3_hex` in your global rebar3 config:

```erlang
{plugins, [rebar3_hex]}.
```

Then authenticate and publish:

```sh
rebar3 hex user auth
rebar3 hex build
rebar3 hex publish
```
