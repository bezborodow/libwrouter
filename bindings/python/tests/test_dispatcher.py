import pytest
import wrouter


def test_resolve_basic():
    routes = [
        ("/account", "account.list"),
        ("/account/", "account.list.trailing"),
        ("/account/create", "account.create"),
        ("/account/a/:account_id", "account.view"),
    ]

    builder = wrouter.Builder()

    for pattern, endpoint in routes:
        builder.add(pattern, endpoint)

    router = builder.compile()
    del builder

    dispatcher = wrouter.Dispatcher(router)

    endpoint, params = dispatcher.resolve("/account")
    assert endpoint == "account.list"
    assert params == {}

    endpoint, params = dispatcher.resolve("/account/create")
    assert endpoint == "account.create"
    assert params == {}

    # Test parameters.
    endpoint, params = dispatcher.resolve("/account/a/1234")
    assert endpoint == "account.view"
    assert params['account_id'] == "1234"

    # Test not found.
    endpoint, params = dispatcher.resolve("/not/found")
    assert endpoint == None
    assert params == {}

    # Trailing-slashes are not equivalent.
    endpoint, params = dispatcher.resolve("/account/create/")
    assert endpoint == None
    assert params == {}

    # Test root '/' is not found if it is not explicitly defined without a fallback.
    endpoint, params = dispatcher.resolve("/")
    assert endpoint == None
    assert params == {}

    # Test illegal format.
    endpoint, params = dispatcher.resolve("")
    assert endpoint == None
    assert params == {}

    endpoint, params = dispatcher.resolve("//")
    assert endpoint == None
    assert params == {}


@pytest.mark.parametrize("route1, route2", [
    ("/", "/"),
    ("/api", "/api"),
    ("/dup", "/dup"),
    ("/dup/:param", "/dup/:param"),
    ("/dup/:param", "/dup/:key"),
    ("/dup/:param", "/dup/:key"),
    ("/dup/:param", "/dup/*"),
    ("/project/:id", "/project/:project_id/accounts/:account_id"),
    ("/project/list", "/project/:project_id"),
])
def test_incompatible_routes(route1, route2):
    builder = wrouter.Builder()

    builder.add(route1, "foo")
    with pytest.raises(wrouter.RouteError):
        builder.add(route2, "foo")

    # Try again in opposite order!!:
    builder = wrouter.Builder()

    builder.add(route2, "foo")
    with pytest.raises(wrouter.RouteError):
        builder.add(route1, "foo")


@pytest.mark.parametrize("route", [
    (""),
    ("*"),
    ("//"),
    ("/*/"),
    ("/:_"),
    ("/::"),
    ("/:1"),
    ("account"),
    ("/account/*/"),
    ("/*/something"),
    ("/account/*/edit"),
    ("/account/*/edit/"),
    ("/account/:_/edit"),
    ("/account/*/edit/"),
    ("/accounts/user/*/view"),
])
def test_invalid_routes(route):
    builder = wrouter.Builder()

    with pytest.raises(wrouter.RouteError):
        builder.add(route, "foo")


def test_unexpected_parameter_usage():
    # Expect that segment "ticket:ticket_id" will resolve to a literal string.
    builder = wrouter.Builder()
    builder.add("/board/:board_id/ticket:ticket_id", "board.ticket"),
    router = builder.compile()
    dispatcher = wrouter.Dispatcher(router)
    context, params = dispatcher.resolve("/board/1234/ticket:1234")
    assert context == None
    assert params == {}
    context, params = dispatcher.resolve("/board/1234/ticket:ticket_id")
    assert context == "board.ticket"
    assert params['board_id'] == "1234"


def test_wildcards():
    routes = [
        ("/", "root"),
        ("/*", "root.wildcard"),
        ("/account/create", "account.create"),
        ("/account/a/:account_id", "account.view"),
        ("/account/a/:account_id/documents/*", "account.documents"),
        ("/project/list", "project.list"),
        ("/project/*", "project.wildcard"),
        ("/repos/:user/:repo", "repo"),
        ("/repos/:user/:repo/tree/:branch", "repo.tree"),
        ("/repos/:user/:repo/tree/:branch/*", "repo.tree.path"),
    ]
    builder = wrouter.Builder()

    for pattern, context in routes:
        builder.add(pattern, context)

    router = builder.compile()
    del builder

    dispatcher = wrouter.Dispatcher(router)

    context, params = dispatcher.resolve("/")
    assert context == "root"
    assert params == {}

    context, params = dispatcher.resolve("/random")
    assert context == "root.wildcard"
    assert params == {"_": "random"}

    context, params = dispatcher.resolve("/random/thing")
    assert context == "root.wildcard"
    assert params == {"_": "random/thing"}

    context, params = dispatcher.resolve("/account/create")
    assert context == "account.create"
    assert params == {}

    context, params = dispatcher.resolve("/account/a/1234")
    assert context == "account.view"
    assert params['account_id'] == "1234"
    assert len(params) == 1

    context, params = dispatcher.resolve("/account/a/1234/documents/document.pdf")
    assert context == "account.documents"
    assert params['account_id'] == "1234"
    assert params['_'] == "document.pdf"
    assert len(params) == 2

    context, params = dispatcher.resolve("/account/a/1234/documents/")
    assert context == None
    assert params == {}

    context, params = dispatcher.resolve("/project/list")
    assert context == "project.list"
    assert params == {}

    context, params = dispatcher.resolve("/project/random")
    assert context == "project.wildcard"
    assert params == {"_": "random"}

    context, params = dispatcher.resolve("/repos/bezborodow/libwrouter")
    assert context == "repo"
    assert params['user'] == "bezborodow"
    assert params['repo'] == "libwrouter"
    assert len(params) == 2

    context, params = dispatcher.resolve("/repos/bezborodow/libwrouter/tree/master")
    assert context == "repo.tree"
    assert params['user'] == "bezborodow"
    assert params['repo'] == "libwrouter"
    assert params['branch'] == "master"
    assert len(params) == 3

    context, params = dispatcher.resolve("/repos/bezborodow/libwrouter/tree/master/bindings/python")
    assert context == "repo.tree.path"
    assert params['user'] == "bezborodow"
    assert params['repo'] == "libwrouter"
    assert params['branch'] == "master"
    assert params['_'] == "bindings/python"
    assert len(params) == 4


def test_dispatcher_after_router_delete():

    builder = wrouter.Builder()

    router = builder.compile()
    del builder

    dispatcher = wrouter.Dispatcher(router)

    # This will cause a segfault if reference counting is incorrect!!  However,
    # deleting the router here, will have no effect if correct.
    del router

    # BOOM?
    dispatcher.resolve("/account")


def test_resolve_cases():
    cases = [
        ("/downloads/*", "/downloads/documents/schematic.pdf", {"_": "documents/schematic.pdf"}),
        ("/downloads/", "/downloads/", {}),
        ("/", "/", {}),
        ("/*", "/hello", {"_": "hello"}),
        ("/accounts", "/accounts", {}),
        ("/accounts/create", "/accounts/create", {}),
        ("/account/<account_id>", "/account/100", {"account_id": "100"}),
        ("/account/<account_id>/edit", "/account/100/edit", {"account_id": "100"}),
        ("/account/<account_id>/projects", "/account/100/projects", {"account_id": "100"}),
        ("/account/<account_id>/contacts", "/account/100/contacts", {"account_id": "100"}),
        (
            "/account/<account_id>/contact/<account_contact_id>",
            "/account/200/contact/300",
            {"account_id": "200", "account_contact_id": "300"}
        ),
        (
            "/account/<account_id>/contact/<account_contact_id>/credentials/*",
            "/account/200/contact/300/credentials/letter_of_endorsement.pdf",
            {"account_id": "200", "account_contact_id": "300", "_": "letter_of_endorsement.pdf"}
        ),
        ("/projects", "/projects", {}),
        ("/projects/create", "/projects/create", {}),
        ("/project/<project_id>", "/project/400", {"project_id": "400"}),
        ("/project/<project_id>/", "/project/400/", {"project_id": "400"}),
        ("/project/<project_id>/edit", "/project/400/edit", {"project_id": "400"}),
        ("/project/<project_id>/edit/", "/project/9900/edit/", {"project_id": "9900"}),
    ]

    builder = wrouter.Builder(param_syntax = wrouter.ANGLE)

    for pattern, _, _ in cases:
        builder.add(pattern, pattern)  # ctx = pattern string

    router = builder.compile()
    del builder

    dispatcher = wrouter.Dispatcher(router)
    #dispatcher = router.dispatcher() TODO

    for pattern, request, expected_params in cases:
        ctx, params = dispatcher.resolve(request)

        assert ctx == pattern
        assert params == expected_params


def test_dispatcher_context_function():
    handler = lambda: None

    builder = wrouter.Builder()
    builder.add("/function", handler)

    router = builder.compile()

    dispatcher = wrouter.Dispatcher(router)

    resolved_handler, _ = dispatcher.resolve("/function")
    assert callable(resolved_handler)
    assert resolved_handler is handler


def test_none_is_a_valid_context():
    builder = wrouter.Builder()
    builder.add("/function", None)

    router = builder.compile()

    dispatcher = wrouter.Dispatcher(router)

    context, _ = dispatcher.resolve("/function")
    assert context is None


def test_many():
    NI = 3
    NJ = 7
    NK = 255

    builder = wrouter.Builder()

    # Building.
    t0 = time.perf_counter()
    for i in range(NI):
        for j in range(NJ):
            for k in range(NK):
                builder.add(f"/a{i}/b{j}/foo/:p{j}/c{k}", f"{i}_{j}_{k}")

    # Compiling.
    t1 = time.perf_counter()
    router = builder.compile()

    # Dispatching.
    t2 = time.perf_counter()
    dispatcher = wrouter.Dispatcher(router)

    for i in range(NI):
        for j in range(NJ):
            for k in range(NK):
                context, params = dispatcher.resolve(f"/a{i}/b{j}/foo/p/c{k}")

                assert context == f"{i}_{j}_{k}"
                assert params[f"p{j}"] == "p"

    t3 = time.perf_counter()

    if False:
        build_time = t1 - t0
        compile_time = t2 - t1
        dispatch_time = t3 - t2
        total_time = t3 - t0
        routes = NI * NJ * NK

        print("\n=== Stats ===")
        print(f"Routes       : {routes:,}")
        print(f"Total time   : {total_time:.6f} s")
        print()
        print(f"Build        : {build_time:.6f} s   {routes / build_time:,.0f} routes/s")
        print(f"Compile      : {compile_time:.6f} s   {routes / compile_time:,.0f} routes/s")
        print(f"Dispatch     : {dispatch_time:.6f} s   {routes / dispatch_time:,.0f} resolves/s")


