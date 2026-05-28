# Wrouter: Symbolic Web Router

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

Routes with trailing slashes are treated the same. The following are equivalent:

 - `/test/`
 - `/test`
