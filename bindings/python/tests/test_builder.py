import pytest
import wrouter


def test_invalid_syntax():
    with pytest.raises(TypeError):
        wrouter.Builder(param_syntax = 1000)


def test_bounds_segment_children():
    pass

