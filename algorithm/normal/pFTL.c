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
static KEYT* _P2L_table;
static bool* _validation_table;
static bool* _section_validation_table;
static uint32_t _log = 0, _garbage_cnt = 0, _usable_section = _NOS;
static bool _GC_flag;
pthread_mutex_t GC_lock;
// Validation Parameters
static uint32_t _user_write_reqest, _lower_write_request_cnt;
static struct mastersegment* GC_reserve_segment;


void pFTL_init(){
    printf("%u %u %u\n",4UL*RANGE, 4UL*L2PGAP*_NOP, sizeof(bool)*L2PGAP*_NOP);
    _L2P_table = (KEYT*)malloc(4UL*RANGE);
    _P2L_table = (KEYT*)malloc(4UL*L2PGAP*_NOP);
    _validation_table = (bool*)malloc(sizeof(bool)*L2PGAP*_NOP);

    _section_validation_table = (bool*)malloc(sizeof(bool)*_NOS);
    
    pftl_buffer.lpas = (KEYT*)malloc(sizeof(KEYT)*L2PGAP);
    pftl_buffer.data_set = inf_get_valueset(NULL,FS_MALLOC_W,PAGESIZE);

   
    //printf("%u", _usable_section);
    memset((void*)_L2P_table, 255, 4UL*RANGE);
    memset((void*)_P2L_table, 255, 4UL*L2PGAP*_NOP); 
    memset((void*)_validation_table, 0, sizeof(bool)*L2PGAP*_NOP);
    memset((void*)_section_validation_table, 0, sizeof(bool)*_NOS);

    pthread_t GC_thread;
    pthread_mutex_init(&GC_lock, NULL);
    GC_reserve_segment = __normal.bm->get_segment(__normal.bm, BLOCK_ACTIVE);
    printf("%u!!!!!!\n\n\n", GC_reserve_segment->seg_idx);
    printf("%u!!!!!!\n\n\n", __normal.bm->get_page_addr(GC_reserve_segment));
    printf("%u!!!!!!\n\n\n", __normal.bm->get_page_addr(GC_reserve_segment));
    printf("%u!!!!!!\n\n\n", __normal.bm->get_page_addr(GC_reserve_segment));
    _log = _PPS;
    //pthread_create(&GC_thread, NULL, GC, NULL);

}

KEYT get_key(KEYT lpa){
    return _L2P_table[lpa];
}

void* GC(void* qwer){
    //printf("initialize GC thread\n");
    //while(true){
        
        if(_usable_section==1){
            
            while (!done_GC()){
                _GC_flag = true;
                //printf("start GC!\n");
                request* read_request = NULL, *write_request = NULL;
                normal_params* read_param = NULL, *write_param;
                read_request = (request*)malloc(sizeof(request));
                write_request = (request*)malloc(sizeof(request));
                read_param = (normal_params*)malloc(sizeof(normal_params));
                write_param = (normal_params*)malloc(sizeof(normal_params));

                KEYT gc_target_section = get_GC_target();
                KEYT empty_section = _log/_PPS;
                //printf("GC target : %u empty section : %u \n", gc_target_section/_PPS,empty_section);
                read_param->done = false;
                write_param->done = false;

                read_request->value = inf_get_valueset(NULL, FS_MALLOC_R, PAGESIZE);
                read_request->param = (void*)read_param;
                read_request->type = GCDR;
                read_request->end_req = GC_end_req;

                write_request->type = GCDW;
                write_request->param = (void*)write_param;
                //write_request->value = (value_set*)malloc(sizeof(value_set));
                write_request->value = inf_get_valueset(NULL, FS_MALLOC_W, LPAGESIZE);
                write_request->end_req = GC_end_req;


                int cnt=0;
                for(int i=0; i<_PPS; i++){
                    read_request->key = gc_target_section+i;
                    normal_get(read_request);
                    wait_for_request(read_request);
                    request temp_request;
                    temp_request.value;
                    for(int pidx=0; pidx<L2PGAP; pidx++){
                        
                        KEYT ppa = ((gc_target_section+i)<<2) | pidx;
                        KEYT lpa = _P2L_table[ppa];
                        if(!_validation_table[ppa]){
                            cnt++;
                            continue;
                        }
                        
                        write_request->key = lpa;
                        //memcpy((void*)write_request->value, (void*)(read_request->value+(LPAGESIZE*pidx)), 1);
                        //pthread_mutex_lock(&GC_lock);
                        write(write_request);
                        wait_for_request(write_request);
                        _validation_table[ppa] = false;
                        write_param->done = false;
                        //pthread_mutex_unlock(&GC_lock);
                    }
                    read_param->done=false;
                }
                //printf("trim!!!!!!!!!!!!!!!!\n");
                __normal.li->trim_block(gc_target_section);

                _section_validation_table[gc_target_section/_PPS]=false;
                _usable_section++;

                free(read_param);
                free(read_request);
                free(write_param);
                free(write_request);
            }
            _GC_flag = false;
            //printf("Done GC!\n");
            

       }
       

        
    //}
    
}
/*
void GC_read(request* const req){
	algo_req *my_req=(algo_req*)malloc(sizeof(algo_req));
    my_req->parents=req;
	my_req->end_req=GC_end_req;
	my_req->param=(void*)req->param;
	my_req->type=DATAR;
    __normal.li->read(get_key(req->key)>>2,PAGESIZE,req->value, my_req);
	return;
}*/

void wait_for_request(request* const req){
    while(!((normal_params*)(req->param))->done){}
    return;
}

inline void log_up(){
    _log++;
    if(!(_log%_PPS)){
        if(((int)_log/_PPS)==_NOS || _section_validation_table[(int)_log/_PPS]){
            //printf("check!!!!\n");
            _log=get_empty_section();
        }
    }    
}

KEYT get_empty_section(){
    for(int i=0; i<_NOS; i++){
        if(!_section_validation_table[i]){
            return i*_PPS;
        }
    }

    printf("No Empty Section!!!!!\n");
    abort();
    return UINT32_MAX;
}

KEYT get_GC_target(){
    uint32_t max=0, id=UINT32_MAX;
    for(int i=0; i<_NOS; i++){
        if(_section_validation_table[i] && (i!=(int)_log/_PPS )){
            u_int32_t size = _PPS*L2PGAP;
            for(int j=i*_PPS*L2PGAP; j<(i+1)*_PPS*L2PGAP; j++){
                size -= _validation_table[j];
            }
            if(max<size){
                max = size;
                id = i*_PPS;
            }
            //printf("Section %d, dummy size : %u\n", i, size);
        }

        
    }
    //printf("Max dummy block : %u\n", max);
    if(id == UINT32_MAX){printf("No GC Target!!!!\n"); abort();}
    return id;
}

bool done_GC(){
    for(int i=0; i<_NOS; i++){
        
        if(_section_validation_table[i] && (i!=(int)_log/_PPS )){
            u_int32_t size = _PPS*L2PGAP;
            for(int j=i*_PPS*L2PGAP; j<(i+1)*_PPS*L2PGAP; j++){
                size -= _validation_table[j];
            }
            //printf("%d %u\n", i, size);
            if(size) return false;
        }
    }
    return true;
}

void write(request* const req){
    //printf("LOG : %u\n" ,_log);
    //printf("Usable section : %u\n" ,_usable_section);
    
    KEYT lpa = req->key;
    KEYT ppa = (_log<<L2P_offset) | pftl_buffer.buffer_count;
	KEYT temp = _L2P_table[lpa];
    uint32_t section_n = (u_int32_t)(_log/_PPS);
   
    _L2P_table[lpa] = ppa;
    _P2L_table[ppa] = lpa;
    if(temp!=UINT32_MAX){
        //KEYT garbage_block_section = (u_int32_t)((temp>>2)/_PPS);
        //printf("section : %u  temp : %u\n", section_n,  _usable_section);
        if(req->type!=GCDW) _validation_table[temp] = false;
        
    }
    //printf("%u", sizeof(req->value));
    //printf("%d %d\n", (void*)pftl_buffer.data_set, (void*)pftl_buffer.data_set + LPAGESIZE*pftl_buffer.buffer_count);
    
	pftl_buffer.lpas[pftl_buffer.buffer_count] = lpa;
	//memcpy(&pftl_buffer.data_set[LPAGESIZE*pftl_buffer.buffer_count], (void*)req->value->value, LPAGESIZE);
	pftl_buffer.ppa=_log;
    pftl_buffer.buffer_count++;
	//printf("Write Logical : %u Physical : %u\n", req->key, ppa>>2);
	if(pftl_buffer.buffer_count == L2PGAP){
        if(!(_log%_PPS)){
            _section_validation_table[section_n] = true;
            _usable_section--;
        }
		//printf("check\n");
		normal_params* params=(normal_params*)malloc(sizeof(normal_params));
		//printf("LPA : %d \n", req->key);
		algo_req *my_req=(algo_req*)malloc(sizeof(algo_req));
		my_req->parents=req;
		my_req->end_req=normal_end_req;
		my_req->type=DATAW;
        if(req->type == GCDW) my_req->type = GCDW;
		my_req->param=(void*)params;
        _validation_table[ppa] = true;
		__normal.li->write(pftl_buffer.ppa,PAGESIZE, pftl_buffer.data_set,my_req);
		
        pftl_buffer.buffer_count=0;
        log_up();
		return;
	}
    _validation_table[ppa] = true;
	req->end_req(req);
    return;
}

bool GC_end_req(request* input){
    normal_params* params=(normal_params*)input->param;
    switch(input->type){
        case GCDR:
            //printf("GC end req type %d, %d\n", input->type,params->done);
            params->done = true;
            break;
        case GCDW:
            params->done = true;
            //printf("GC end req type %d,\n", input->type);
            break;
    }
    //((normal_params*)(input->param))->done = true;
    return  true;
}

void wait_for_gc(){
    while(_usable_section <= 1)
    {
        //printf("waiting\n");
    }
    return;
}