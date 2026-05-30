#include "wrouter.h"
#include "builder.h"
#include "router.h"
#include <Python.h>

static int parse_syntax(PyObject *obj, wrouter_param_syntax_t *out)
{
    if (!PyObject_TypeCheck(obj, &PyParamSyntaxType)) {
        PyErr_SetString(PyExc_TypeError, "ParamSyntax required");
        return -1;
    }

    *out = ((PyParamSyntaxObject *)obj)->value;
    return 0;
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

void PyBuilder_dealloc(PyBuilderObject *self)
{
    if (self->inner) {
        if (self->inner->builder) {
            wrouter_builder_free(self->inner->builder);
        }
        PyMem_Free(self->inner);
    }
    Py_TYPE(self)->tp_free((PyObject *)self);
}

PyObject *PyBuilder_new(PyTypeObject *type, PyObject *args, PyObject *kw)
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

static PyMethodDef PyBuilder_methods[] = {
    { "add", (PyCFunction)PyBuilder_add, METH_VARARGS, NULL },
    { "compile", (PyCFunction)PyBuilder_compile, METH_NOARGS, NULL },
    { NULL, NULL, 0, NULL }
};

PyTypeObject PyParamSyntaxType = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "wrouter.ParamSyntax",
    .tp_basicsize = sizeof(PyParamSyntaxObject),
    .tp_flags = Py_TPFLAGS_DEFAULT,
};

PyTypeObject PyBuilderType = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "wrouter.Builder",
    .tp_basicsize = sizeof(PyBuilderObject),
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = PyBuilder_new,
    .tp_dealloc = (destructor)PyBuilder_dealloc,
    .tp_methods = PyBuilder_methods,
};
