import threading
import pytest
import wrouter


def test_builder_thread_safety_or_reject_usage_compile():
    builder = wrouter.Builder();

    exc = {}

    def worker():
        try:
            builder.compile()
        except Exception as e:
            exc["e"] = e

    t = threading.Thread(target=worker)
    t.start()
    t.join()

    assert "e" in exc
    assert isinstance(exc["e"], RuntimeError)


def test_builder_thread_safety_or_reject_usage_add_route():
    builder = wrouter.Builder();

    exc = {}

    def worker():
        try:
            builder.add("/", "foo")
        except Exception as e:
            exc["e"] = e

    t = threading.Thread(target=worker)
    t.start()
    t.join()

    assert "e" in exc
    assert isinstance(exc["e"], RuntimeError)
