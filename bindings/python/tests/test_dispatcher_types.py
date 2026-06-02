import pytest
import wrouter


def test_invalid_router():
    with pytest.raises(TypeError):
        wrouter.Router(None)
    with pytest.raises(TypeError):
        wrouter.Router(1000)
    with pytest.raises(TypeError):
        wrouter.Router("foo")
