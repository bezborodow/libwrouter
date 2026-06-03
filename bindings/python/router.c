#include "router.h"
#include "wrouter.h"
#include <Python.h>

PyObject *PyRouter_FromRouter(wrouter_t *router)
{
    PyRouterObject *obj = PyObject_New(PyRouterObject, &PyRouterType);
    if (!obj)
        return NULL;

    obj->inner = PyMem_Calloc(1, sizeof(PyRouter));
    if (!obj->inner) {
        Py_DECREF(obj);
        return PyErr_NoMemory();
    }

    obj->inner->router = router;

    return (PyObject *)obj;
}

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
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE,
    .tp_dealloc = (destructor)PyRouter_dealloc,
};
