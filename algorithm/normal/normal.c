#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include "normal.h"
#include "../../bench/bench.h"
#include "pFTL.h"
#define LOWERTYPE 10

extern MeasureTime mt;

struct algorithm __normal={
	.argument_set=NULL,
	.create=normal_create,
	.destroy=normal_destroy,
	.read=normal_get,
	.write=normal_set,
	.remove=normal_remove
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
	printf("Segment count %u\n", TOTALSIZE/_PPS);
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
	my_req->type=DATAR;
	switch (req->type){
	case GCDR:
		__normal.li->read(req->key,PAGESIZE,req->value, my_req);
		break;
	
	default:
		printf("Read Logical : %u physical : %u\n ", req->key, get_key(req->key)>>2);
		__normal.li->read(get_key(req->key)>>2,PAGESIZE,req->value, my_req);	
		break;
	}
	//__normal.li->read(req->key,PAGESIZE,req->value,req->isAsync, my_req);
	
	
	return 1;
}
uint32_t normal_set(request *const req){
	write(req);


	//__normal.li->write(req->key ,PAGESIZE,req->value,req->isAsync,my_req);
	//__normal.li->write(get_key(req->key, DATAW, 0),PAGESIZE,req->value,my_req);
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
	//printf("call %u\n", res->key);

	res->end_req(res);

	free(params);
	free(input);
	return NULL;
}

