#ifndef __H_PFTL__
#define __H_PFTL__
#include "../../include/container.h"
#include "../../include/settings.h"
#include "../../interface/vectored_interface.h"
#include "list.h"
#include <math.h>
#define L2P_offset 2

typedef struct pftl_buffer
{
    uint8_t buffer_count;
    KEYT ppa;
    KEYT* lpas;
    value_set* data_set;
}pftl_buffer_t;

typedef struct section_info{
    bool valid;
    list_t dead_block_list;
}section_info_t;

void pFTL_init();
KEYT get_key(KEYT lpa);
void* GC(void*);
KEYT get_empty_section();
KEYT get_GC_target();
//void GC_read(request* const req);
void wait_for_request(request* const req);
bool done_GC();
inline void log_up();
KEYT get_empty_section();
void write(request* const req);
bool GC_end_req(request* input);
#endif