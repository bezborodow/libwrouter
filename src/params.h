#pragma once
#include "wrouter.h"

#define WILDCARD_PARAM "_"

int params_alloc(wrouter_params_t *params, size_t max_params);
void params_free(wrouter_params_t *params);
int params_nt_alloc(wrouter_params_nt_t *params, size_t n);
void params_nt_free(wrouter_params_nt_t *params);
wrouter_param_t *param_next(wrouter_params_t *params);
