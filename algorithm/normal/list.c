#include "list.h"


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