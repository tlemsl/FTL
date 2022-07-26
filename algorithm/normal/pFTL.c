#include "list.h"
#include "pFTL.h"
#include "normal.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
pftl_buffer_t pftl_buffer;

extern algorithm __normal;
// Essential Variables
static KEYT* _L2P_table;

static bool _GC_flag;
pthread_mutex_t GC_lock;

static struct mastersegment* _GC_reserve_segment, *_write_segment;

static request* last_request;

void pFTL_init(){
    _L2P_table = (KEYT*)malloc(4UL*RANGE);
    
    pftl_buffer.lpas = (KEYT*)malloc(sizeof(KEYT)*L2PGAP);
    pftl_buffer.data_set = (value_set*)inf_get_valueset(NULL,FS_MALLOC_W,PAGESIZE);
    
   
    //printf("%u", _usable_section);
    memset((void*)_L2P_table, 255, 4UL*RANGE);
    
    //pthread_t GC_thread;
    //pthread_mutex_init(&GC_lock, NULL);
    _write_segment = __normal.bm->get_segment(__normal.bm, BLOCK_ACTIVE);
    _GC_reserve_segment = __normal.bm->get_segment(__normal.bm, BLOCK_RESERVE);
    //pthread_create(&GC_thread, NULL, GC, NULL);

}

KEYT get_key(KEYT lpa){
    return _L2P_table[lpa];
}



void* GC(void* qwer){
    //printf("initialize GC thread\n");
    //while(true){
        
    if(__normal.bm->is_gc_needed(__normal.bm) && __normal.bm->check_full(_write_segment)){
    
        _GC_flag = true;
        printf("start GC!\n");
        request* read_request = NULL, *write_request = NULL;
        normal_params* read_param = NULL, *write_param;
        
        

        __gsegment* gc_target_section = __normal.bm->get_gc_target(__normal.bm);
        _write_segment = _GC_reserve_segment;
        //printf("GC target : %u empty section : %u \n", gc_target_section/_PPS,empty_section);
    

        for(int i=0; i<_PPS; i++){
            read_request = (request*)malloc(sizeof(request));
            read_param = (normal_params*)malloc(sizeof(normal_params));
            read_param->done = false;
            read_request->value = inf_get_valueset(NULL, FS_MALLOC_R, PAGESIZE);
            read_request->param = (void*)read_param;
            read_request->type = GCDR;
            read_request->end_req = GC_end_req;

            KEYT ppa = gc_target_section->seg_idx*_PPS+i;
            read_request->key = ppa;
            normal_get(read_request);
            while(!read_param->done){printf("");}            
            KEYT* temp_lbas=(KEYT*)__normal.bm->get_oob(__normal.bm, ppa);

            for(int pidx=0; pidx<L2PGAP; pidx++){

                KEYT piece_ppa = (ppa<<2) | pidx;
                KEYT lpa = temp_lbas[pidx];
                //printf("GC lpa : %u\n", lpa);
                if(__normal.bm->is_invalid_piece(__normal.bm, piece_ppa)){
                    continue;
                }
                write_request = (request*)malloc(sizeof(request));
                write_param = (normal_params*)malloc(sizeof(normal_params));
                write_param->done = false;
                write_request->type = GCDW;
                write_request->param = (void*)write_param;
                write_request->end_req = GC_end_req;
                write_request->key = lpa;
                write_request->value = inf_get_valueset(NULL, FS_MALLOC_W, PAGESIZE);
                memcpy(write_request->value->value, &read_request->value->value[LPAGESIZE*pidx], LPAGESIZE);
                //pthread_mutex_lock(&GC_lock);
                write(write_request);
//                wait_for_request(write_request);
                __normal.bm->bit_unset(__normal.bm, piece_ppa);
                
                //write_param->done = false;
                //pthread_mutex_unlock(&GC_lock);
            }
            free(read_param);
            inf_free_valueset(read_request->value, FS_MALLOC_R);
            free(read_request);
        }
        //printf("trim!!!!!!!!!!!!!!!!\n");
        //__normal.li->trim_block(gc_target_section->seg_idx*_PPS);
        __normal.bm->trim_segment(__normal.bm,gc_target_section);
        __normal.bm->change_reserve_to_active(__normal.bm, _GC_reserve_segment);
        _GC_reserve_segment = __normal.bm->get_segment(__normal.bm, BLOCK_RESERVE);
        
        //free(read_param);
        //free(read_request);
        //free(write_param);
        //free(write_request);
    
    _GC_flag = false;
    printf("Done GC!\n");
        

    }
    return NULL;
    //}
    
}


void wait_for_request(request* const req){
    int temp=0;
    while(!((normal_params*)(req->param))->done){temp+=1;}
    return;
}



void write(request* const req){
    //printf("LOG : %u\n" ,_log);
    //printf("Usable section : %u\n" ,_usable_section);
    if(!pftl_buffer.buffer_count){
        if(__normal.bm->check_full(_write_segment)) _write_segment = __normal.bm->get_segment(__normal.bm, BLOCK_ACTIVE);
    
        pftl_buffer.ppa = __normal.bm->get_page_addr(_write_segment);
    }
    KEYT lpa = req->key;
    KEYT piece_ppa = (pftl_buffer.ppa<<L2P_offset) | pftl_buffer.buffer_count;
	KEYT reserved_ppa = _L2P_table[lpa];
    _L2P_table[lpa] = piece_ppa;
    if(reserved_ppa!=UINT32_MAX&&req->type!=GCDW) __normal.bm->bit_unset(__normal.bm, reserved_ppa);
   
	pftl_buffer.lpas[pftl_buffer.buffer_count] = lpa;
	memcpy(&pftl_buffer.data_set->value[LPAGESIZE*pftl_buffer.buffer_count], (void*)req->value->value, LPAGESIZE);
	pftl_buffer.buffer_count++;
	//printf("Write Logical : %u Physical : %u\n", req->key, pftl_buffer.ppa);
	if(pftl_buffer.buffer_count == L2PGAP){
		//printf("check\n");
		//normal_params* params=(normal_params*)malloc(sizeof(normal_params));
		//printf("LPA : %d \n", req->key);
		algo_req *my_req=(algo_req*)malloc(sizeof(algo_req));
		my_req->parents=req;
		my_req->end_req=normal_end_req;
		my_req->type=DATAW;
        if(req->type == GCDW) my_req->type = GCDW;
		//my_req->param=(void*)params;
        __normal.bm->set_oob(__normal.bm, (char*)pftl_buffer.lpas,
            sizeof(KEYT)*L2PGAP, pftl_buffer.ppa);
        __normal.bm->bit_set(__normal.bm, piece_ppa);
        memcpy(req->value->value, pftl_buffer.data_set->value, (PAGESIZE>>2)*3);
        
        __normal.li->write(pftl_buffer.ppa,PAGESIZE, req->value,my_req);
		
        pftl_buffer.buffer_count=0;
		return;
	}
    __normal.bm->bit_set(__normal.bm, piece_ppa);
	req->end_req(req);
    return;
}

void read(request* const req){
    //normal_params* params=(normal_params*)malloc(sizeof(normal_params));

	algo_req *my_req=(algo_req*)malloc(sizeof(algo_req));
	my_req->parents=req;
	my_req->end_req=normal_end_req;
	//my_req->param=(void*)params;
    my_req->type=DATAR;
    
	//req->value = (value_set*)inf_get_valueset(NULL, FS_MALLOC_R, PAGESIZE);

    for(int i=0; i<pftl_buffer.buffer_count; i++){
        if(pftl_buffer.lpas[i] == req->key){
            memcpy(req->value->value, &pftl_buffer.data_set->value[i*LPAGESIZE], LPAGESIZE);
            req->end_req(req);
            return;
        }
    }
	
	switch (req->type){
	case GCDR:
		my_req->type=GCDR;
		__normal.li->read(req->key,PAGESIZE,req->value, my_req);
		break;
	
	default:
		my_req->type=DATAR;
		my_req->ppa = get_key(req->key);
		//printf("Read Logical : %u physical : %u\n ", req->key, get_key(req->key)>>2);
		__normal.li->read(get_key(req->key)>>2,PAGESIZE,req->value, my_req);
		break;
	}
	//__normal.li->read(req->key,PAGESIZE,req->value,req->isAsync, my_req);
	
	
	return;
}

bool GC_end_req(request* input){
    normal_params* params=(normal_params*)input->param;
    switch(input->type){
        case GCDR:
            //printf("GC end req type %d, %d\n", input->type,params->done);
            params->done = true;
            //inf_free_valueset(input->value, FS_MALLOC_R);
            break;
        case GCDW:
            //printf("GC end req type %d %u,\n", input->type, input->key);
            
            inf_free_valueset(input->value, FS_MALLOC_W);
            free(input->param);
            free(input);
            //params->done = true;
            break;
    }
    
    //((normal_params*)(input->param))->done = true;
    return  true;
}

