#define PY_SSIZE_T_CLEAN
#include "wrouter.h"
#include "builder.h"
#include "router.h"
#include "dispatcher.h"
#include <Python.h>

static PyObject *py_build_router(PyObject *self, PyObject *args);

// clang-format off
static PyMethodDef module_methods[] = {
    {
        "build_router",
        py_build_router,
        METH_VARARGS,
        "Build a router from (pattern, route_ctx) tuples"
    },
    {NULL, NULL, 0, NULL}
};
// clang-format on

static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT, "wrouter", NULL, -1, module_methods, NULL, NULL, NULL, NULL
};

PyObject *WrouterRouteError = NULL;

static PyObject *PyParamSyntax_COLON;
static PyObject *PyParamSyntax_BRACE;
static PyObject *PyParamSyntax_ANGLE;

static PyObject *PyParamSyntax_New(wrouter_param_syntax_t v)
{
    PyParamSyntaxObject *obj = PyObject_New(PyParamSyntaxObject, &PyParamSyntaxType);

    if (!obj)
        return NULL;

    obj->value = v;
    return (PyObject *)obj;
}

static PyObject *py_build_router(PyObject *self, PyObject *args)
{
    (void)self;

    wrouter_error_t err = WROUTER_OK;

    PyObject *routes;
    if (!PyArg_ParseTuple(args, "O", &routes))
        return NULL;

    if (!PyList_Check(routes))
        return PyErr_Format(PyExc_TypeError, "routes must be a list");

    wrouter_options_t opts = { 0 };
    wrouter_builder_t *builder = wrouter_builder_create(opts);
    if (!builder)
        return PyErr_NoMemory();

    Py_ssize_t n = PyList_Size(routes);

    for (Py_ssize_t i = 0; i < n; i++) {
        PyObject *item = PyList_GetItem(routes, i);

        const char *pattern;
        PyObject *route_ctx;

        if (!PyArg_ParseTuple(item, "sO", &pattern, &route_ctx)) {
            wrouter_builder_free(builder);
            return PyErr_Format(PyExc_TypeError, "Route must be (pattern, route_ctx)");
        }

        err = wrouter_add_context(builder, pattern, route_ctx);

        if (err == WROUTER_ERR_NO_MEMORY) {
            wrouter_builder_free(builder);
            return PyErr_NoMemory();
        }

        if (err != WROUTER_OK) {
            wrouter_builder_free(builder);
            PyErr_SetString(WrouterRouteError, wrouter_strerror(err));
            return NULL;
        }
    }

    wrouter_t *router = wrouter_compile(builder, &err);
    wrouter_builder_free(builder);

    if (err == WROUTER_ERR_NO_MEMORY)
        return PyErr_NoMemory();

    if (err != WROUTER_OK) {
        PyErr_SetString(WrouterRouteError, wrouter_strerror(err));
        return NULL;
    }

    return PyRouter_FromRouter(router);
}

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

    WrouterRouteError = PyErr_NewException("wrouter.RouteError", PyExc_Exception, NULL);

    PyParamSyntax_COLON = PyParamSyntax_New(WROUTER_SYNTAX_COLON);
    PyParamSyntax_BRACE = PyParamSyntax_New(WROUTER_SYNTAX_BRACE);
    PyParamSyntax_ANGLE = PyParamSyntax_New(WROUTER_SYNTAX_ANGLE);

    if (!PyParamSyntax_COLON || !PyParamSyntax_BRACE || !PyParamSyntax_ANGLE)
        goto failure;

    Py_INCREF(PyParamSyntax_COLON);
    Py_INCREF(PyParamSyntax_BRACE);
    Py_INCREF(PyParamSyntax_ANGLE);

    PyModule_AddObject(m, "RouteError", WrouterRouteError);

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
