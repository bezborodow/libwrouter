#include "wrouter.h"
#include "dispatcher.h"
#include "params.h"
#include <Python.h>

#if 0
static PyObject *PyParams_subscript(PyObject *self, PyObject *key)
{
    PyErr_SetObject(PyExc_KeyError, key);

    return NULL;

    PyParamsObject *p = (PyParamsObject *)self;

    if (p->snapshot == NULL)
        return PyErr_Format(PyExc_RuntimeError, "Invalid Params object.");

    if (!PyUnicode_Check(key)) {
        PyErr_SetString(PyExc_TypeError, "Key must be str.");
        return NULL;
    }

    const char *name = PyUnicode_AsUTF8(key);
    if (!name)
        return NULL;

    for (uint32_t i = 0; i < p->snapshot->params.count; i++) {
        wrouter_param_nt_t *param = &p->snapshot->params.base[i];

        if (strcmp(param->name, name) == 0)
            return PyUnicode_FromString(param->value);
    }

    PyErr_SetObject(PyExc_KeyError, key);

    return NULL;
}

static Py_ssize_t PyParams_len(PyObject *self)
{
    PyParamsObject *p = (PyParamsObject *)self;

    return p->snapshot->params.count;
}

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

    Py_TYPE(self)->tp_free((PyObject *)self);
}
#endif

static Py_ssize_t PyParams_len(PyObject *self)
{
    (void)self;
    return 0;
}

static PyObject *PyParams_subscript(PyObject *self, PyObject *key)
{
    (void)self;
    PyErr_SetObject(PyExc_KeyError, key);
    return NULL;
}

PyObject *PyParams_FromDispatcher(const wrouter_dispatcher_t *dispatcher)
{
    (void)dispatcher;

    PyParamsObject *obj = PyObject_New(PyParamsObject, &PyParamsType);
    if (!obj)
        return NULL;

    return (PyObject *)obj;
}

static void PyParams_dealloc(PyParamsObject *self)
{
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyMappingMethods PyParams_mapping = {
    .mp_length = PyParams_len,
    .mp_subscript = PyParams_subscript,
};

static PyObject *PyParams_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyParamsObject *self = (PyParamsObject *)type->tp_alloc(type, 0);
    if (!self)
        return NULL;
    return (PyObject *)self;
}

static PyObject *PyParams_iter(PyObject *self)
{
    PyObject *empty = PyTuple_New(0);
    if (!empty)
        return NULL;

    PyObject *it = PyObject_GetIter(empty);
    Py_DECREF(empty);
    return it;
}

static PyObject *PyParams_richcompare(PyObject *a, PyObject *b, int op)
{
    if (op != Py_EQ)
        Py_RETURN_NOTIMPLEMENTED;

    if (PyDict_Check(b))
        return PyBool_FromLong(PyDict_Size(b) == 0);

    Py_RETURN_NOTIMPLEMENTED;
}

// clang-format off
PyTypeObject PyParamsType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "wrouter.Params",
    .tp_basicsize = sizeof(PyParamsObject),
    .tp_dealloc = (destructor)PyParams_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_richcompare = PyParams_richcompare,
    .tp_as_mapping = &PyParams_mapping,
    .tp_new = PyParams_new,
    .tp_iter = PyParams_iter,
};
// clang-format on
