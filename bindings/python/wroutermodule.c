#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "wrouter.h"

typedef struct {
    PyObject_HEAD wrouter_param_syntax_t value;
} PyParamSyntaxObject;

static PyTypeObject PyParamSyntaxType = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "wrouter.ParamSyntax",
    .tp_basicsize = sizeof(PyParamSyntaxObject),
    .tp_flags = Py_TPFLAGS_DEFAULT,
};
static PyObject *PyParamSyntax_COLON;
static PyObject *PyParamSyntax_BRACE;
static PyObject *PyParamSyntax_ANGLE;

static PyTypeObject PyBuilderType;
static PyTypeObject PyRouterType;
static PyTypeObject PyDispatcherType;

typedef struct {
    wrouter_builder_t *builder;
    pthread_t owner_tid;
} PyBuilder;

typedef struct {
    PyObject_HEAD PyBuilder *inner;
} PyBuilderObject;

typedef struct {
    wrouter_t *router;
} PyRouter;

typedef struct {
    PyObject_HEAD PyRouter *inner;
} PyRouterObject;

typedef struct {
    wrouter_dispatcher_t *dispatcher;
    pthread_t owner_tid;
} PyDispatcher;

typedef struct {
    PyObject_HEAD PyDispatcher *inner;
} PyDispatcherObject;

typedef struct {
    PyObject *value; /* str or arbitrary object */
    int is_string;
} PyRouteCtx;

static PyObject *PyParamSyntax_New(wrouter_param_syntax_t v)
{
    PyParamSyntaxObject *obj = PyObject_New(PyParamSyntaxObject, &PyParamSyntaxType);

    if (!obj)
        return NULL;

    obj->value = v;
    return (PyObject *)obj;
}

static int parse_syntax(PyObject *obj, wrouter_param_syntax_t *out)
{
    if (!PyObject_TypeCheck(obj, &PyParamSyntaxType)) {
        PyErr_SetString(PyExc_TypeError, "ParamSyntax required");
        return -1;
    }

    *out = ((PyParamSyntaxObject *)obj)->value;
    return 0;
}

/* ---------------------------
   Builder
   --------------------------- */

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
    PyObject *syntax_obj = NULL;
    PyBuilderObject *self = NULL;
    wrouter_options_t opts = { 0 };

    static char *kwlist[] = { "param_syntax", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kw, "|O", kwlist, &syntax_obj))
        return NULL;

    self = (PyBuilderObject *)type->tp_alloc(type, 0);
    if (!self)
        return NULL;

    self->inner = PyMem_Calloc(1, sizeof(PyBuilder));
    if (!self->inner)
        goto failure;

    if (syntax_obj) {
        if (parse_syntax(syntax_obj, &opts.param_syntax) < 0)
            goto failure;
    }

    self->inner->owner_tid = pthread_self();
    self->inner->builder = wrouter_builder_create(opts);
    if (!self->inner->builder)
        goto failure;

    return (PyObject *)self;

failure:
    Py_XDECREF(self);
    if (self && self->inner)
        PyMem_Free(self->inner);

    return PyErr_NoMemory();
}

/* add route with ctx only */
static PyObject *PyBuilder_add(PyBuilderObject *self, PyObject *args)
{
    const char *pattern;
    PyObject *ctx;

    if (!PyArg_ParseTuple(args, "sO", &pattern, &ctx))
        return NULL;

    if (!pthread_equal(self->inner->owner_tid, pthread_self())) {
        PyErr_SetString(PyExc_RuntimeError, "Builder is thread-bound.");
        return NULL;
    }

    PyRouteCtx *rc = PyMem_Malloc(sizeof(PyRouteCtx));
    if (!rc)
        return PyErr_NoMemory();

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

static PyObject *PyBuilder_compile(PyBuilderObject *self, PyObject *args)
{
    (void)args;
    PyRouterObject *obj = PyObject_New(PyRouterObject, &PyRouterType);
    if (!obj)
        return NULL;

    if (!pthread_equal(self->inner->owner_tid, pthread_self())) {
        PyErr_SetString(PyExc_RuntimeError, "Builder is thread-bound.");
        return NULL;
    }

    obj->inner = PyMem_Calloc(1, sizeof(PyRouter));
    if (!obj->inner)
        return PyErr_NoMemory();

    obj->inner->router = wrouter_compile(self->inner->builder);
    if (!obj->inner->router)
        return PyErr_NoMemory();

    return (PyObject *)obj;
}

/* ---------------------------
   Router
   --------------------------- */

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

/* ---------------------------
   Dispatcher
   --------------------------- */

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
    if (!self)
        return NULL;

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
    }
    return PyErr_NoMemory();
}

/* resolve only */
static PyObject *PyDispatcher_resolve(PyDispatcherObject *self, PyObject *args)
{
    const char *path;

    if (!PyArg_ParseTuple(args, "s", &path))
        return NULL;

    if (!pthread_equal(self->inner->owner_tid, pthread_self())) {
        PyErr_SetString(PyExc_RuntimeError, "Dispatcher is thread-bound.");
        return NULL;
    }

    const void *ctx = wrouter_resolve(self->inner->dispatcher, path);

    if (!ctx)
        Py_RETURN_NONE;

    const PyRouteCtx *rc = (const PyRouteCtx *)ctx;

    if (rc->is_string)
        return PyUnicode_FromString(PyUnicode_AsUTF8(rc->value));

    Py_INCREF(rc->value);
    return rc->value;
}

/* ---------------------------
   Type definitions
   --------------------------- */

static PyMethodDef PyBuilder_methods[] = {
    { "add", (PyCFunction)PyBuilder_add, METH_VARARGS, NULL },
    { "compile", (PyCFunction)PyBuilder_compile, METH_NOARGS, NULL },
    { NULL, NULL, 0, NULL }
};

static PyTypeObject PyBuilderType = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "wrouter.Builder",
    .tp_basicsize = sizeof(PyBuilderObject),
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = PyBuilder_new,
    .tp_dealloc = (destructor)PyBuilder_dealloc,
    .tp_methods = PyBuilder_methods,
};

static PyMethodDef PyDispatcher_methods[] = {
    { "resolve", (PyCFunction)PyDispatcher_resolve, METH_VARARGS, NULL }, { NULL }
};

static PyTypeObject PyDispatcherType = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "wrouter.Dispatcher",
    .tp_basicsize = sizeof(PyDispatcherObject),
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = PyDispatcher_new,
    .tp_dealloc = (destructor)PyDispatcher_dealloc,
    .tp_methods = PyDispatcher_methods,
};

/* Router is opaque in Python (no methods needed) */

static PyTypeObject PyRouterType = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "wrouter.Router",
    .tp_basicsize = sizeof(PyRouterObject),
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_dealloc = (destructor)PyRouter_dealloc,
};

/* ---------------------------
   Module
   --------------------------- */

static PyMethodDef module_methods[] = { { NULL } };

static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT, "wrouter", NULL, -1, module_methods, NULL, NULL, NULL, NULL
};

PyMODINIT_FUNC PyInit_wrouter(void)
{
    PyObject *m = NULL;
    PyObject *enum_mod = NULL;
    PyObject *int_enum = NULL;
    PyObject *ParamSyntax = NULL;
    PyObject *args = NULL;

    if (PyType_Ready(&PyBuilderType) < 0)
        return NULL;
    if (PyType_Ready(&PyRouterType) < 0)
        return NULL;
    if (PyType_Ready(&PyDispatcherType) < 0)
        return NULL;

    m = PyModule_Create(&moduledef);
    if (!m)
        goto failure;

    if (PyType_Ready(&PyParamSyntaxType) < 0)
        goto failure;

    PyParamSyntax_COLON = PyParamSyntax_New(WROUTER_SYNTAX_COLON);
    PyParamSyntax_BRACE = PyParamSyntax_New(WROUTER_SYNTAX_BRACE);
    PyParamSyntax_ANGLE = PyParamSyntax_New(WROUTER_SYNTAX_ANGLE);

    if (!PyParamSyntax_COLON || !PyParamSyntax_BRACE || !PyParamSyntax_ANGLE)
        goto failure;

    Py_INCREF(PyParamSyntax_COLON);
    Py_INCREF(PyParamSyntax_BRACE);
    Py_INCREF(PyParamSyntax_ANGLE);

    PyModule_AddObject(m, "COLON", PyParamSyntax_COLON);
    PyModule_AddObject(m, "BRACE", PyParamSyntax_BRACE);
    PyModule_AddObject(m, "ANGLE", PyParamSyntax_ANGLE);

    Py_INCREF(&PyParamSyntaxType);
    PyModule_AddObject(m, "ParamSyntax", (PyObject *)&PyParamSyntaxType);

    Py_INCREF(&PyBuilderType);
    PyModule_AddObject(m, "Builder", (PyObject *)&PyBuilderType);

    Py_INCREF(&PyRouterType);
    PyModule_AddObject(m, "Router", (PyObject *)&PyRouterType);

    Py_INCREF(&PyDispatcherType);
    PyModule_AddObject(m, "Dispatcher", (PyObject *)&PyDispatcherType);

    return m;

failure:
    Py_XDECREF(args);
    Py_XDECREF(int_enum);
    Py_XDECREF(enum_mod);
    Py_XDECREF(ParamSyntax);
    Py_DECREF(m);
    return NULL;
}
