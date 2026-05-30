#pragma once
#include <Python.h>

typedef struct {
    wrouter_dispatcher_t *dispatcher;
    pthread_t owner_tid;
} PyDispatcher;

typedef struct {
    PyObject_HEAD PyDispatcher *inner;
} PyDispatcherObject;

typedef struct {
    PyObject_HEAD
    wrouter_param_t *items;
    uint32_t count;
} PyParamsObject;

extern PyTypeObject PyDispatcherType;
