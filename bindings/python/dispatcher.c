#include "wrouter.h"
#include "router.h"
#include "dispatcher.h"
#include <Python.h>
#include <pthread.h>

static void PyDispatcher_dealloc(PyDispatcherObject *self)
{
    if (self->inner) {
        if (self->inner->dispatcher)
            wrouter_dispatcher_free(self->inner->dispatcher);

        PyMem_Free(self->inner);
    }

    Py_XDECREF(self->router_obj);

    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *PyDispatcher_new(PyTypeObject *type, PyObject *args, PyObject *kw)
{
    (void)kw;

    PyObject *router_obj;

    if (!PyArg_ParseTuple(args, "O", &router_obj))
        return NULL;

    PyRouterObject *r = (PyRouterObject *)router_obj;

    PyDispatcherObject *self = (PyDispatcherObject *)type->tp_alloc(type, 0);
    if (!self)
        goto failure;

    self->router_obj = router_obj;
    Py_INCREF(router_obj);

    self->inner = PyMem_Calloc(1, sizeof(PyDispatcher));
    if (!self->inner)
        goto failure;

    self->inner->owner_tid = pthread_self();

    self->inner->dispatcher = wrouter_dispatcher_create(r->inner->router);

    if (!self->inner->dispatcher)
        goto failure;

    return (PyObject *)self;

failure:
    if (self) {
        if (self->inner) {
            if (self->inner->dispatcher)
                wrouter_dispatcher_free(self->inner->dispatcher);

            PyMem_Free(self->inner);
        }

        Py_DECREF(self);
        Py_XDECREF(self->router_obj);
    }

    return PyErr_NoMemory();
}

static PyObject *PyDispatcher_resolve(PyDispatcherObject *self, PyObject *args)
{
    const char *path;

    if (!PyArg_ParseTuple(args, "s", &path))
        return NULL;

    if (!pthread_equal(self->inner->owner_tid, pthread_self())) {
        PyErr_SetString(PyExc_RuntimeError, "Dispatcher is thread-bound.");
        return NULL;
    }

    PyObject *ctx_obj = (PyObject *)wrouter_resolve(self->inner->dispatcher, path);

    if (!ctx_obj)
        ctx_obj = Py_None;

    // Construct parameter dictionary.
    PyObject *params_obj = PyDict_New();
    if (!params_obj) {
        return NULL;
    }

    const wrouter_params_t *params = wrouter_params(self->inner->dispatcher);

    for (uint16_t i = 0; i < params->count; i++) {

        const wrouter_param_t *param = &params->base[i];

        PyObject *k = PyUnicode_FromString(param->name);
        PyObject *v = PyUnicode_FromStringAndSize(param->value, param->length);

        if (!k || !v) {
            Py_XDECREF(k);
            Py_XDECREF(v);
            Py_DECREF(params_obj);
            return NULL;
        }

        PyDict_SetItem(params_obj, k, v);
        Py_DECREF(k);
        Py_DECREF(v);
    }

    return PyTuple_Pack(2, ctx_obj, params_obj);
}

static PyMethodDef PyDispatcher_methods[] = {
    { "resolve", (PyCFunction)PyDispatcher_resolve, METH_VARARGS, NULL }, { NULL }
};

PyTypeObject PyDispatcherType = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "wrouter.Dispatcher",
    .tp_basicsize = sizeof(PyDispatcherObject),
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = PyDispatcher_new,
    .tp_dealloc = (destructor)PyDispatcher_dealloc,
    .tp_methods = PyDispatcher_methods,
};
