import pytest
import wrouter


def test_invalid_syntax():
    with pytest.raises(TypeError):
        wrouter.Builder(param_syntax = 1000)


def test_out_of_range_segment_children():
    builder = wrouter.Builder()

    last_ok = None

    with pytest.raises(wrouter.RouteError):
        for i in range(1024):
            builder.add(f"/foo/a{i}", i)
            last_ok = i

    assert last_ok == 255

    # The builder is corrupted now.
    with pytest.raises(wrouter.RouteError):
        builder.add("/bar", None)

    # The builder is corrupted now.
    with pytest.raises(wrouter.RouteError):
        builder.compile()


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
