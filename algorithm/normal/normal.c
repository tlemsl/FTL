#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include "normal.h"
#include "../../bench/bench.h"
#include "pFTL.h"
#define LOWERTYPE 10

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
	normal_params* params=(normal_params*)malloc(sizeof(normal_params));

	algo_req *my_req=(algo_req*)malloc(sizeof(algo_req));
	my_req->parents=req;
	my_req->end_req=normal_end_req;
	my_req->param=(void*)params;
	normal_cnt++;
	
	switch (req->type){
	case GCDR:
		my_req->type=GCDR;
		__normal.li->read(req->key,PAGESIZE,req->value, my_req);
		break;
	
	default:
		my_req->type=DATAR;
		printf("Read Logical : %u physical : %u\n ", req->key, get_key(req->key)>>2);
		__normal.li->read(get_key(req->key)>>2,PAGESIZE,req->value, my_req);
		uint8_t offset = get_key(req->key)&(L2PGAP-1);
		//memcpy(&req->value->value[0], &req->value->value[offset*LPAGESIZE], LPAGESIZE );
		break;
	}
	//__normal.li->read(req->key,PAGESIZE,req->value,req->isAsync, my_req);
	
	
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
	printf("call %u\n", res->key);
	
	if(res->key == 0){
		FILE* temp = fopen("readresult.txt", "w");
		for(int i=0; i<LPAGESIZE;i++){
                fprintf(temp,"%d\n", ((KEYT*)res->value->value)[i]);
        }
		fflush(temp);
        fclose(temp);
	}
	//printf("\n")
	res->end_req(res);
	

	free(params);
	free(input);
	return NULL;
}


