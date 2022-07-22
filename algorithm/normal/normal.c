#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include "normal.h"
#include "../../bench/bench.h"
#include "pFTL.h"
//#define LOWERTYPE 10

extern MeasureTime mt;
extern pthread_mutex_t GC_lock;

struct algorithm __normal={
        .argument_set = NULL,
        .create = normal_create,
        .destroy = normal_destroy,
        .read = normal_get,
        .write = normal_set,
        .flush = NULL,
        .remove=normal_remove,
        .test = NULL,
        .print_log = NULL,
        .empty_cache = NULL,
        .dump_prepare=NULL,
        .dump = NULL
};

n_cdf _cdf[LOWERTYPE];

//char temp[PAGESIZE];

void normal_cdf_print(){

}
uint32_t normal_create (lower_info* li,blockmanager *a, algorithm *algo){
	algo->li=li;
	algo->bm = a;
	//memset(temp,'x',PAGESIZE);
	for(int i=0; i<LOWERTYPE; i++){
		_cdf[i].min=UINT_MAX;
	}
	pFTL_init();
	return 1;
}
void normal_destroy (lower_info* li, algorithm *algo){
	normal_cdf_print();
	return;
}

int normal_cnt;
uint32_t normal_get(request *const req){
	read(req);
	
	return 1;
}
uint32_t normal_set(request *const req){
	GC(NULL);
	//pthread_mutex_lock(&GC_lock);
	write(req);
	//wait_for_gc();


	//__normal.li->write(req->key ,PAGESIZE,req->value,req->isAsync,my_req);
	//__normal.li->write(get_key(req->key, DATAW, 0),PAGESIZE,req->value,my_req);
	//pthread_mutex_unlock(&GC_lock);
	//wait_for_gc();
	return 0;
}
uint32_t normal_remove(request *const req){
	//__normal.li->trim_block(req->key, false);
	__normal.li->trim_block(req->key);
	
	return 1;
}
void *normal_end_req(algo_req* input){
	normal_params* params=(normal_params*)input->param;
	//bool check=false;
	//int cnt=0;
	request *res=input->parents;
	if(input->type == DATAR){
		uint8_t read_offset = input->ppa & (L2PGAP-1);
		memcpy(&res->value->value[0], &res->value->value[read_offset*LPAGESIZE], LPAGESIZE);	
	}

	
	res->end_req(res);

	//if(params->value) printf("chekc2\n");//inf_free_valueset(params->value, FS_MALLOC_W);

	free(params);
	free(input);
	return NULL;
}


