#define PY_SSIZE_T_CLEAN
#include "wrouter.h"
#include "builder.h"
#include "router.h"
#include "dispatcher.h"
#include <Python.h>

static PyMethodDef module_methods[] = { { NULL } };

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

    WrouterRouteError = PyErr_NewException(
        "wrouter.RouteError",
        PyExc_Exception,
        NULL
    );

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
