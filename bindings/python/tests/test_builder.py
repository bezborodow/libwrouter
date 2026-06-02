import pytest
import wrouter
import random
import string
import time


def test_invalid_syntax():
    with pytest.raises(TypeError):
        wrouter.Builder(param_syntax = 1000)


def test_bounds_segment_children():
    builder = wrouter.Builder()

    with pytest.raises(wrouter.RouteError):
        print("Building...");
        for i in range(1024):
            builder.add(f"/a{i}", i)


def test_many():
    NI = 3
    NJ = 7
    NK = 255

    builder = wrouter.Builder()

    # Building.
    t0 = time.perf_counter()
    for i in range(NI):
        for j in range(NJ):
            for k in range(NK):
                builder.add(f"/a{i}/b{j}/foo/:p{j}/c{k}", f"{i}_{j}_{k}")

    # Compiling.
    t1 = time.perf_counter()
    router = builder.compile()

    # Dispatching.
    t2 = time.perf_counter()
    dispatcher = wrouter.Dispatcher(router)

    for i in range(NI):
        for j in range(NJ):
            for k in range(NK):
                context, params = dispatcher.resolve(f"/a{i}/b{j}/foo/p/c{k}")

                assert context == f"{i}_{j}_{k}"
                assert params[f"p{j}"] == "p"

    t3 = time.perf_counter()

    if False:
        build_time = t1 - t0
        compile_time = t2 - t1
        dispatch_time = t3 - t2
        total_time = t3 - t0
        routes = NI * NJ * NK

        print("\n=== Stats ===")
        print(f"Routes       : {routes:,}")
        print(f"Total time   : {total_time:.6f} s")
        print()
        print(f"Build        : {build_time:.6f} s   {routes / build_time:,.0f} routes/s")
        print(f"Compile      : {compile_time:.6f} s   {routes / compile_time:,.0f} routes/s")
        print(f"Dispatch     : {dispatch_time:.6f} s   {routes / dispatch_time:,.0f} resolves/s")


