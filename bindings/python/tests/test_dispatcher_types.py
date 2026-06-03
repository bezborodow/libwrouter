import pytest
from wrouter import Dispatcher


def test_invalid_router():
    class NotARouter:
        pass

    not_a_router = NotARouter()

    with pytest.raises(TypeError):
        Dispatcher(None)

    with pytest.raises(TypeError):
        Dispatcher(1000)

    with pytest.raises(TypeError):
        Dispatcher("foo")

    with pytest.raises(TypeError):
        Dispatcher(NotARouter)

    with pytest.raises(TypeError):
        Dispatcher(object)

    with pytest.raises(TypeError):
        Dispatcher(NotARouter)
