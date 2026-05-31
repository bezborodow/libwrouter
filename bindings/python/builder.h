#pragma once
#include "wrouter.h"
#include <pthread.h>
#include <Python.h>

typedef struct {
    PyObject_HEAD
    wrouter_param_syntax_t value;
} PyParamSyntaxObject;

typedef struct {
    wrouter_builder_t *builder;
    pthread_t owner_tid;
} PyBuilder;

typedef struct {
    PyObject_HEAD
    PyBuilder *inner;
} PyBuilderObject;

extern PyTypeObject PyParamSyntaxType;
extern PyTypeObject PyBuilderType;
