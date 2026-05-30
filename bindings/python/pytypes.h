#pragma once
#include <Python.h>
#include "wrouter.h"

typedef struct {
    wrouter_builder_t *builder;
    pthread_t owner_tid;
} PyBuilder;

typedef struct {
    PyObject_HEAD PyBuilder *inner;
} PyBuilderObject;

typedef struct {
    wrouter_t *router;
} PyRouter;

typedef struct {
    PyObject_HEAD PyRouter *inner;
} PyRouterObject;

typedef struct {
    wrouter_dispatcher_t *dispatcher;
    pthread_t owner_tid;
} PyDispatcher;

typedef struct {
    PyObject_HEAD PyDispatcher *inner;
} PyDispatcherObject;

typedef struct {
    PyObject_HEAD wrouter_param_syntax_t value;
} PyParamSyntaxObject;

typedef struct {
    PyObject *value; /* str or arbitrary object */
    int is_string;
} PyRouteCtx;
