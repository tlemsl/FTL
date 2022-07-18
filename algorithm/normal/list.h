#ifndef __H_LIST__
#define __H_LIST__
#include "../../include/settings.h"
typedef struct listnode
{
    struct listnode* next;
    KEYT data;
}node_t;
typedef struct list{
    uint32_t size;
    node_t* start_node;
}list_t;



void init_list(list_t* list);
void lappend(list_t* list, KEYT data);
bool lfind(list_t* list, KEYT data);
bool lremove(list_t* list, KEYT data);

#endif