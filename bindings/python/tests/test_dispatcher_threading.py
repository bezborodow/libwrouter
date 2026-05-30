import threading
import pytest
import wrouter


def test_dispatcher_thread_safety_or_reject_usage():
    router = wrouter.Builder().compile()
    dispatcher = wrouter.Dispatcher(router)

    exc = {}

    def worker():
        try:
            dispatcher.resolve("test")
        except Exception as e:
            exc["e"] = e

    t = threading.Thread(target=worker)
    t.start()
    t.join()

    assert "e" in exc
    assert isinstance(exc["e"], RuntimeError)
