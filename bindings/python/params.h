#pragma once
#include "wrouter.h"
#include <Python.h>

#define PYPARAMS 0

#if PYPARAMS
typedef struct {
    PyObject_HEAD
} PyParamsObject;

extern PyTypeObject PyParamsType;

PyObject *PyParams_FromDispatcher(const wrouter_dispatcher_t *dispatcher);

#endif
