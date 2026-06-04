import pytest
import wrouter


def test_invalid_syntax():
    with pytest.raises(TypeError):
        wrouter.Builder(param_syntax = 1000)


def test_out_of_range_segment_children():
    builder = wrouter.Builder()

    last_ok_index = None

    with pytest.raises(wrouter.RouteError):
        for i in range(4 * 1024):
            builder.add(f"/foo/a{i}", i)
            last_ok_index = i

    accepted_count = last_ok_index + 1
    assert last_ok_index == 254
    assert accepted_count == 255

    # The builder is corrupted now.
    with pytest.raises(wrouter.RouteError):
        builder.add("/bar", None)

    # The builder is corrupted now.
    with pytest.raises(wrouter.RouteError):
        builder.compile()

    # Try again, but stop before failure.
    del builder
    builder = wrouter.Builder()
    for i in range(accepted_count):
        builder.add(f"/foo/a{i}", i)

    # Compile and resolve all accepted routes.
    router = builder.compile()
    dispatcher = wrouter.Dispatcher(router)
    for i in range(accepted_count):
        assert dispatcher.resolve(f"/foo/a{i}") == (i, {})


def test_out_of_range_graph_size():

    # Choose a number that will exceed the limits.
    NI = 2
    NJ = 55
    NK = 100

    # Build.
    builder = wrouter.Builder()
    for i in range(NI):
        for j in range(NJ):
            for k in range(NK):
                builder.add(f"/a{i}/b{j}/c{k}", None)

    # Compile.
    with pytest.raises(wrouter.RouteError):
        builder.compile()
