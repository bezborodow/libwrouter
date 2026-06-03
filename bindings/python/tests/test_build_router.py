import pytest
from wrouter import build_router, Dispatcher, RouteError


def test_build_router():
    routes = [
        ("/account", "account.list"),
        ("/account/", "account.list.trailing"),
        ("/account/create", "account.create"),
        ("/account/a/:account_id", "account.view"),
    ]

    dispatcher = Dispatcher(build_router(routes))
    
    route_ctx, params = dispatcher.resolve("/account/create")
    assert route_ctx == "account.create"
    assert params == {}

    route_ctx, params = dispatcher.resolve("/account/a/1234")
    assert route_ctx == "account.view"
    assert params['account_id'] == "1234"


def test_invalid_build():
    routes = [
        ("/////", "invalid"),
    ]

    with pytest.raises(RouteError):
        dispatcher = Dispatcher(build_router(routes))
