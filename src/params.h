#pragma once
#include "wrouter.h"

int params_alloc(wrouter_params_t *params, size_t max_params);
void params_free(wrouter_params_t *params);
int params_nt_alloc(wrouter_params_nt_t *params, size_t n);
void params_nt_free(wrouter_params_nt_t *params);
