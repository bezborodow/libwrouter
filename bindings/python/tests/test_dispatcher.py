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
])
def test_incompatible_routes(route1, route2):
    builder = wrouter.Builder()

    builder.add(route1, "foo")
    with pytest.raises(RuntimeError):
        builder.add(route2, "foo")


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
])
def test_invalid_routes(route):
    builder = wrouter.Builder()

    with pytest.raises(RuntimeError):
        builder.add(route, "foo")


def test_wildcards():
    routes = [
        ("/", "root"),
        ("/*", "root.wildcard"),
        ("/account/create", "account.create"),
        ("/account/a/:account_id", "account.view"),
        ("/account/a/:account_id/documents/*", "account.documents"),
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
    assert params == {}

    context, params = dispatcher.resolve("/random/thing")
    assert context == "root.wildcard"
    assert params == {}

    context, params = dispatcher.resolve("/account/create")
    assert context == "account.create"
    assert params == {}

    context, params = dispatcher.resolve("/account/a/1234")
    assert context == "account.view"
    assert params['account_id'] == "1234"

    context, params = dispatcher.resolve("/account/a/1234/documents/document.pdf")
    assert context == "account.documents"
    assert params['account_id'] == "1234"

    context, params = dispatcher.resolve("/account/a/1234/documents/")
    assert context == None
    assert params == {}


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
        ("/downloads/*", "/downloads/documents/schematic.pdf", {}),
        ("/downloads/", "/downloads/", {}),
        ("/", "/", {}),
        ("/*", "/hello", {}),
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
            {"account_id": "200", "account_contact_id": "300"}
        ),
        ("/projects", "/projects", {}),
        ("/projects/create", "/projects/create", {}),
        ("/project/<project_id>", "/project/400", {"project_id": "400"}),
        ("/project/<project_id>/edit", "/project/400/edit", {"project_id": "400"}),
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
