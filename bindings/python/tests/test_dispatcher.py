import pytest
import wrouter


def test_resolve_basic():
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
    del builder

    dispatcher = wrouter.Dispatcher(router)

    endpoint, params = dispatcher.resolve("/account")
    assert endpoint == "account.list"
    assert params == {}

    endpoint, params = dispatcher.resolve("/account/create")
    assert endpoint == "account.create"
    assert params == {}

    endpoint, params = dispatcher.resolve("/account/a/1234")
    assert endpoint == "account.view"
    assert(params['account_id'] == "1234")

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
