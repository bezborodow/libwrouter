#pragma once
#include "wrouter.h"
#include <Python.h>

typedef struct {
    wrouter_t *router;
} PyRouter;

typedef struct {
    PyObject_HEAD
    PyRouter *inner;
} PyRouterObject;

extern PyTypeObject PyRouterType;

PyObject *PyRouter_FromRouter(wrouter_t *router);
