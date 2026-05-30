#include "pytypes.h"
#include "router.h"
#include <Python.h>

static void PyRouter_dealloc(PyRouterObject *self)
{
    if (self->inner) {
        if (self->inner->router) {
            wrouter_free(self->inner->router);
        }
        PyMem_Free(self->inner);
    }
    Py_TYPE(self)->tp_free((PyObject *)self);
}

PyTypeObject PyRouterType = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "wrouter.Router",
    .tp_basicsize = sizeof(PyRouterObject),
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_dealloc = (destructor)PyRouter_dealloc,
};
