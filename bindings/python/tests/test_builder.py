import pytest
import wrouter
import random
import string


def test_invalid_syntax():
    with pytest.raises(TypeError):
        wrouter.Builder(param_syntax = 1000)


def test_bounds_segment_children():
    builder = wrouter.Builder()

    with pytest.raises(wrouter.RouteError):
        print("Building...");
        for i in range(1024):
            builder.add(f"/a{i}", i)
