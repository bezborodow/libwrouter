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

    endpoint = dispatcher.resolve("/account")
    assert endpoint == "account.list"

    endpoint = dispatcher.resolve("/account/create")
    assert endpoint == "account.create"

    endpoint = dispatcher.resolve("/account/a/1234")
    assert endpoint == "account.view"


@pytest.mark.skip(reason="Known segfault: dispatcher holds router pointer.")
def test_dispatcher_after_router_delete():

    builder = wrouter.Builder()

    router = builder.compile()
    del builder

    dispatcher = wrouter.Dispatcher(router)

    # TODO This will cause a segfault.
    del router

    dispatcher.resolve("/account")


def test_resolve_cases():
    cases = [
        ("/downloads/*", "/downloads/documents/schematic.pdf", None),
        ("/downloads/", "/downloads/", None),
        ("/", "/", None),
        ("/*", "/hello", None),
        ("/accounts", "/accounts", None),
        ("/accounts/create", "/accounts/create", None),
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
        ("/projects", "/projects", None),
        ("/projects/create", "/projects/create", None),
        ("/project/<project_id>", "/project/400", {"project_id": "400"}),
        ("/project/<project_id>/edit", "/project/400/edit", {"project_id": "400"}),
    ]

    builder = wrouter.Builder(param_syntax = wrouter.ANGLE)

    for pattern, _, _ in cases:
        builder.add(pattern, pattern)  # ctx = pattern string

    router = builder.compile()
    del builder

    dispatcher = wrouter.Dispatcher(router)
    #dispatcher = router.dispatcher()

    for pattern, request, expected_params in cases:
        ctx = dispatcher.resolve(request)

        assert ctx == pattern
