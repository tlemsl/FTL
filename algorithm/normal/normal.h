#ifndef __H_NORMAL__
#define __H_NORMAL__
#include "../../include/container.h"
#include "../../include/settings.h"
typedef struct normal_params{
	request *parents;
	bool done;
}normal_params;

typedef struct normal_cdf_struct{
	uint64_t total_micro;
	uint64_t cnt;
	uint64_t max;
	uint64_t min;
}n_cdf;

uint32_t normal_create (lower_info*, blockmanager * d,algorithm *);
void normal_destroy (lower_info*,  algorithm *);
uint32_t normal_get(request *const);
uint32_t normal_set(request *const);
uint32_t normal_remove(request *const);
void *normal_end_req(algo_req*);
#endif