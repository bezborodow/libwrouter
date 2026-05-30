#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "wrouter.h"

static PyTypeObject PyBuilderType;
static PyTypeObject PyRouterType;
static PyTypeObject PyDispatcherType;


typedef struct {
    PyObject *value;   /* str or arbitrary object */
    int is_string;
} PyRouteCtx;

/* ---------------------------
   Builder
   --------------------------- */

typedef struct {
    wrouter_builder_t *builder;
} PyBuilder;

typedef struct {
    PyObject_HEAD
    PyBuilder *inner;
} PyBuilderObject;

static void PyBuilder_dealloc(PyBuilderObject *self)
{
    if (self->inner) {
        if (self->inner->builder) {
            wrouter_builder_free(self->inner->builder);
        }
        PyMem_Free(self->inner);
    }
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *PyBuilder_new(PyTypeObject *type, PyObject *args, PyObject *kw)
{
    (void)args;
    (void)kw;

    PyBuilderObject *self = (PyBuilderObject *)type->tp_alloc(type, 0);
    if (!self) return NULL;

    self->inner = PyMem_Calloc(1, sizeof(PyBuilder));
    if (!self->inner) return PyErr_NoMemory();

    wrouter_options_t opts = {0};
    self->inner->builder = wrouter_builder_create(opts);

    if (!self->inner->builder) {
        PyMem_Free(self->inner);
        return PyErr_NoMemory();
    }

    return (PyObject *)self;
}

/* add route with ctx only */
static PyObject *PyBuilder_add(PyBuilderObject *self, PyObject *args)
{
    const char *pattern;
    PyObject *ctx;

    if (!PyArg_ParseTuple(args, "sO", &pattern, &ctx))
        return NULL;

    PyRouteCtx *rc = PyMem_Malloc(sizeof(PyRouteCtx));
    if (!rc) return PyErr_NoMemory();

    rc->is_string = PyUnicode_Check(ctx);
    rc->value = ctx;
    Py_INCREF(ctx);

    if (wrouter_add_context(self->inner->builder, pattern, rc) != 0) {
        Py_DECREF(ctx);
        PyMem_Free(rc);
        Py_RETURN_NONE;
    }

    Py_RETURN_NONE;
}

static PyObject *PyBuilder_compile(PyBuilderObject *self, PyObject *args);

/* ---------------------------
   Router
   --------------------------- */

typedef struct {
    wrouter_t *router;
} PyRouter;

typedef struct {
    PyObject_HEAD
    PyRouter *inner;
} PyRouterObject;

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

static PyObject *PyBuilder_compile(PyBuilderObject *self, PyObject *args)
{
    (void)args;
    PyRouterObject *obj = PyObject_New(PyRouterObject, &PyRouterType);
    if (!obj) return NULL;

    obj->inner = PyMem_Calloc(1, sizeof(PyRouter));
    if (!obj->inner) return PyErr_NoMemory();

    obj->inner->router = wrouter_compile(self->inner->builder);
    if (!obj->inner->router) return PyErr_NoMemory();

    return (PyObject *)obj;
}

/* ---------------------------
   Dispatcher
   --------------------------- */

typedef struct {
    wrouter_dispatcher_t *dispatcher;
} PyDispatcher;

typedef struct {
    PyObject_HEAD
    PyDispatcher *inner;
} PyDispatcherObject;

static void PyDispatcher_dealloc(PyDispatcherObject *self)
{
    if (self->inner) {
        if (self->inner->dispatcher) {
            wrouter_dispatcher_free(self->inner->dispatcher);
        }
        PyMem_Free(self->inner);
    }
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
    if (!self) return NULL;

    self->inner = PyMem_Calloc(1, sizeof(PyDispatcher));
    if (!self->inner) return PyErr_NoMemory();

    self->inner->dispatcher =
        wrouter_dispatcher_create(r->inner->router);

    if (!self->inner->dispatcher)
        return PyErr_NoMemory();

    return (PyObject *)self;
}

/* resolve only */
static PyObject *PyDispatcher_resolve(PyDispatcherObject *self, PyObject *args)
{
    const char *path;

    if (!PyArg_ParseTuple(args, "s", &path))
        return NULL;

    const void *ctx = wrouter_resolve(self->inner->dispatcher, path);

    if (!ctx) {
        Py_RETURN_NONE;
    }

    const PyRouteCtx *rc = (const PyRouteCtx *)ctx;

    if (rc->is_string) {
        return PyUnicode_FromString(PyUnicode_AsUTF8(rc->value));
    }

    Py_INCREF(rc->value);
    return rc->value;
}

/* ---------------------------
   Type definitions
   --------------------------- */

static PyMethodDef PyBuilder_methods[] = {
    {"add", (PyCFunction)PyBuilder_add, METH_VARARGS, NULL},
    {"compile", (PyCFunction)PyBuilder_compile, METH_NOARGS, NULL},
    {NULL, NULL, 0, NULL}
};

static PyTypeObject PyBuilderType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "wrouter.Builder",
    .tp_basicsize = sizeof(PyBuilderObject),
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = PyBuilder_new,
    .tp_dealloc = (destructor)PyBuilder_dealloc,
    .tp_methods = PyBuilder_methods,
};

static PyMethodDef PyDispatcher_methods[] = {
    {"resolve", (PyCFunction)PyDispatcher_resolve, METH_VARARGS, NULL},
    {NULL}
};

static PyTypeObject PyDispatcherType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "wrouter.Dispatcher",
    .tp_basicsize = sizeof(PyDispatcherObject),
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = PyDispatcher_new,
    .tp_dealloc = (destructor)PyDispatcher_dealloc,
    .tp_methods = PyDispatcher_methods,
};

/* Router is opaque in Python (no methods needed) */

static PyTypeObject PyRouterType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "wrouter.Router",
    .tp_basicsize = sizeof(PyRouterObject),
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_dealloc = (destructor)PyRouter_dealloc,
};

/* ---------------------------
   Module
   --------------------------- */

static PyMethodDef module_methods[] = {
    {NULL}
};

static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
    "wrouter",
    NULL,
    -1,
    module_methods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC PyInit_wrouter(void)
{
    if (PyType_Ready(&PyBuilderType) < 0) return NULL;
    if (PyType_Ready(&PyRouterType) < 0) return NULL;
    if (PyType_Ready(&PyDispatcherType) < 0) return NULL;

    PyObject *m = PyModule_Create(&moduledef);
    if (!m) return NULL;

    Py_INCREF(&PyBuilderType);
    PyModule_AddObject(m, "Builder", (PyObject *)&PyBuilderType);

    Py_INCREF(&PyRouterType);
    PyModule_AddObject(m, "Router", (PyObject *)&PyRouterType);

    Py_INCREF(&PyDispatcherType);
    PyModule_AddObject(m, "Dispatcher", (PyObject *)&PyDispatcherType);

    return m;
}
