#include "list.h"
#include "pFTL.h"
#include "normal.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
pftl_buffer_t pftl_buffer;

void init_list(list_t* list){
    list->size = 0;
    list->start_node = (node_t*)malloc(sizeof(node_t));
    list->start_node->next = NULL;
}

void lappend(list_t* list, KEYT data){
    if(!list->size){
        list->start_node->data = data;
        list->start_node->next = (node_t*)malloc(sizeof(node_t));
    }    
    else{
                //printf("call!!, %u %u\n", list->size, _PPS);

        node_t* temp = list->start_node;
        for(int i=0; i<list->size; i++){
            temp = temp->next;
        }
        temp->data = data;
        temp->next = (node_t*)malloc(sizeof(node_t));
    }
    list->size++;
    return;
}

bool lfind(list_t* list, KEYT data){
    if(!list->size) return false;
    else{
        node_t* temp = list->start_node;
        for(int i=0; i<list->size; i++){
            if(temp->data == data) return true;
            temp = temp->next;
        }
        return false;
    }
}

bool lremove(list_t* list, KEYT data){
    if(!list->size) return false;
    else{
        node_t* prev_node = NULL, *temp = list->start_node;
        for(int i=0; i<list->size; i++){
            if(temp->data == data){
                list->size--;
                if(temp == list->start_node) return true;
                prev_node->next = temp->next;
                free(temp);
                return true;
            }
            prev_node = temp;
            temp = temp->next;
        }
        return false;
    }
}
extern algorithm __normal;

static KEYT* _L2P_table;
static KEYT* _P2L_table;
static bool* _validation_table;
static section_info_t* _section_info_table;
static uint32_t _log = 0, _garbage_cnt = 0, _usable_section = _NOS;

void pFTL_init(){
    
    _L2P_table = (KEYT*)malloc(4UL*RANGE);
    _P2L_table = (KEYT*)malloc(4UL*L2PGAP*_NOP);
    _section_info_table = (section_info_t*)malloc(sizeof(section_info_t)*_NOS);
    
    pftl_buffer.lpas = (KEYT*)malloc(sizeof(KEYT)*L2PGAP);
    pftl_buffer.data_set = inf_get_valueset(NULL,FS_BUSE_W,PAGESIZE*L2PGAP);

    for(int i=0; i<_NOS; i++){
        init_list(&_section_info_table[i].dead_block_list);
    }
    //printf("%u", _usable_section);
    memset((void*)_L2P_table, 255, 4UL*RANGE);
    memset((void*)_P2L_table, 255, 4UL*L2PGAP*_NOP); 
    pthread_t GC_thread;
    pthread_create(&GC_thread, NULL, GC, NULL);
    printf("%u %u %u\n", _usable_section, _L2P_table, _P2L_table);
}

KEYT get_key(KEYT lpa){
    return _L2P_table[lpa];
}

void* GC(void* qwer){
    printf("initialize GC thread\n");
    while(true){
        if(_usable_section==1){
            printf("start GC!\n");
            request* read_request = (request*)malloc(sizeof(request)),
                    *write_request = (request*)malloc(sizeof(request));
            normal_params* read_param = (normal_params*)malloc(sizeof(normal_params));
            while (!done_GC()){
                KEYT gc_target_section = get_GC_target();
                KEYT empty_section = get_empty_section();
                printf("GC target : %u empty section : %u \n", gc_target_section/_PPS,empty_section/_PPS);
                read_param->done = false;
                read_request->value = inf_get_valueset(NULL, FS_MALLOC_R, PAGESIZE);
                read_request->param = (void*)read_param;
                read_request->type = GCDR;
                read_request->end_req = GC_end_req;

                write_request->type = GCDW;
                write_request->end_req = GC_end_req;



                for(int i=0; i<_PPS; i++){
                    read_request->key = gc_target_section+i;
                    normal_get(read_request);
                    wait_for_request(read_request);
                    request temp_request;
                    temp_request.value;
                    for(int pidx=0; pidx<L2PGAP; pidx++){
                        KEYT ppa = ((gc_target_section+i)<<2) | pidx;
                        KEYT lpa = _P2L_table[ppa];
                        if( lremove(&_section_info_table[gc_target_section/_PPS].dead_block_list, ppa)) continue;
                        write_request->key = lpa;
                        write_request->value = read_request->value+LPAGESIZE*pidx;
                        write(write_request);

                    }
                    read_param->done=false;
                }
                __normal.li->trim_a_block(gc_target_section);
                _section_info_table[gc_target_section/_PPS].valid=false;
                _usable_section++;

                
                        
            }
            free(read_param);
            free(read_request);
            free(write_request);

        }
        
    }
    
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
        if(_section_info_table[(int)_log/_PPS].valid){
            _log=get_empty_section();
        }
    }    
}

KEYT get_empty_section(){
    KEYT id=UINT32_MAX;
    for(int i=0; i<_NOS; i++){
        if(!_section_info_table[i].valid){
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
        u_int32_t size = _section_info_table[i].dead_block_list.size;
        if(max<size){
            max = size;
            id = i*_PPS;
        }
    }
    printf("Max dummy block : %u\n", max);
    if(id == UINT32_MAX){printf("No GC Target!!!!\n"); abort();}
    return id;
}

bool done_GC(){
    for(int i=0; i<_NOS; i++){
        //printf("%d %u\n", i, _section_info_table[i].dead_block_list.size);
        if(_section_info_table[i].dead_block_list.size) return false;
    }
    return true;
}

void write(request* const req){
    //printf("%u\n" ,_usable_section);
    
    KEYT lpa = req->key;
    KEYT ppa = (_log<<L2P_offset) | pftl_buffer.buffer_count;
	KEYT temp = _L2P_table[lpa];
    uint32_t section_n = (u_int32_t)(_log/_PPS);
    

    _section_info_table[section_n].valid = true;
    _L2P_table[lpa] = ppa;
    _P2L_table[ppa] = lpa;
    if(temp!=UINT32_MAX){
        KEYT garbage_block_section = (u_int32_t)((temp>>2)/_PPS);
        // /printf("section : %u  garbage section : %u temp : %u\n", section_n, garbage_block_section, _usable_section);
        lappend(&_section_info_table[garbage_block_section].dead_block_list, temp);
        
    }
	pftl_buffer.lpas[pftl_buffer.buffer_count] = lpa;
	memcpy((void*)pftl_buffer.data_set + LPAGESIZE*pftl_buffer.buffer_count, (void*)req->value, sizeof(req->value));
	pftl_buffer.ppa=_log;
    pftl_buffer.buffer_count++;
	//printf("Write Logical : %u Physical : %u\n", req->key, ppa>>2);
	if(pftl_buffer.buffer_count == L2PGAP){
        if(!(_log%_PPS)){
            _section_info_table[section_n].valid=true;
            _usable_section--;
        }
		//printf("check\n");
		normal_params* params=(normal_params*)malloc(sizeof(normal_params));
		//printf("LPA : %d \n", req->key);
		algo_req *my_req=(algo_req*)malloc(sizeof(algo_req));
		my_req->parents=req;
		my_req->end_req=normal_end_req;
		my_req->type=DATAW;
		my_req->param=(void*)params;
		__normal.li->write(pftl_buffer.ppa,PAGESIZE, pftl_buffer.data_set,my_req);
		pftl_buffer.buffer_count=0;
        log_up();
		return;
	}
	req->end_req(req);
    return;
}

bool GC_end_req(request* input){
    normal_params* params=(normal_params*)input->param;
    switch(input->type){
        case GCDR:
            printf("GC end req type %d, %d\n", input->type,params->done);
            params->done = true;
            break;
        case GCDW:
            printf("GC end req type %d,\n", input->type);
            break;
    }
    //((normal_params*)(input->param))->done = true;
    return  true;
}