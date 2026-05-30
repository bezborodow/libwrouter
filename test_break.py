import threading
import wrouter

router = wrouter.Builder().compile()
dispatcher = wrouter.Dispatcher(router)

def worker():
    dispatcher.resolve("test")   # should raise RuntimeError

t = threading.Thread(target=worker)
t.start()
t.join()
