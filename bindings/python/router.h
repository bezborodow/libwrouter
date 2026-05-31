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

typedef struct {
    PyObject *value;
    int is_string;
} PyRouteCtx;

extern PyTypeObject PyRouterType;
