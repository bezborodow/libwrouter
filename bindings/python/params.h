#pragma once
#include "wrouter.h"
#include <Python.h>

typedef struct {
    PyObject_HEAD
    wrouter_params_snapshot_t *snapshot;
} PyParamsObject;

extern PyTypeObject PyParamsType;

PyObject *PyParams_FromDispatcher(const wrouter_dispatcher_t *dispatcher);
