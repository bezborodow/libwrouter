import pytest
import wrouter


def test_invalid_syntax():
    with pytest.raises(TypeError):
        builder = wrouter.Builder(param_syntax = 1000)
