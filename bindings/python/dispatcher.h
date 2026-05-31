#pragma once
#include "wrouter.h"
#include <pthread.h>
#include <Python.h>

typedef struct {
    wrouter_dispatcher_t *dispatcher;
    pthread_t owner_tid;
} PyDispatcher;

typedef struct {
    PyObject_HEAD PyDispatcher *inner;
    PyObject *router_obj;
} PyDispatcherObject;

extern PyTypeObject PyDispatcherType;
