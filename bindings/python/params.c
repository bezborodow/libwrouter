#include "wrouter.h"
#include "dispatcher.h"
#include "params.h"
#include <Python.h>

PyObject *PyParams_FromDispatcher(const wrouter_dispatcher_t *dispatcher)
{
    // Deep copy of parameters from the dispatcher.
    const wrouter_params_t *params = wrouter_params(dispatcher);
    wrouter_params_snapshot_t *snapshot = wrouter_params_copy(params);

    if (snapshot == NULL)
        return PyErr_NoMemory();

    PyParamsObject *obj = PyObject_New(PyParamsObject, &PyParamsType);

    if (!obj)
        goto failure;

    obj->snapshot = snapshot;

    return (PyObject *)obj;

failure:
    wrouter_snapshot_free(snapshot);
    return NULL;
}

static void PyParams_dealloc(PyParamsObject *self)
{
    wrouter_snapshot_free(self->snapshot);
    PyObject_Del(self);
}

PyTypeObject PyParamsType = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "wrouter.Params",
    .tp_basicsize = sizeof(PyParamsObject),
    .tp_dealloc = (destructor)PyParams_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT,
};
