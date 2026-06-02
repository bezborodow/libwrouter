import pytest
import wrouter
import random
import string


def test_invalid_syntax():
    with pytest.raises(TypeError):
        wrouter.Builder(param_syntax = 1000)


def test_bounds_segment_children():
    builder = wrouter.Builder()

    print("Building...");
    for i in range(1024):
        s = ''.join(random.choices(string.ascii_lowercase, k=3))
        builder.add(f"/{s}-{i}", i)

    print("Compiling...");
    router = builder.compile()
    dispatcher = wrouter.Dispatcher(router)


    assert dispatcher.resolve("/literal-100") == (10023, {})
