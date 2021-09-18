#include"linked_list.h"

void initList(LinkedList* list){
    list->begin = 0;
    list->end = 0;
}

static void recursiveDestroyNode(struct ListNode* node){
    if(!node)
        return;
    struct ListNode* next_node = node->next;
    free(node);
    recursiveDestroyNode(next_node);
}

void destroyList(LinkedList* list){
    recursiveDestroyNode(list->begin);
    recursiveDestroyNode(list->end);
}

static void recursiveAddNode(LinkedList* list, struct ListNode* node, size_t cycles, EventFunc func){
    if(node){
        if(node->next){
            if(node->next->cycles > cycles){
                struct ListNode* tmp = node->next;
                node->next          = malloc(sizeof(struct ListNode));
                node->next->next    = tmp;
                node->next->cycles  = cycles;
                node->next->func    = func;
            } else
                recursiveAddNode(list, node->next, cycles, func);
        } else{
            node->next          = malloc(sizeof(struct ListNode));
            node->next->cycles  = cycles;
            node->next->func    = func;
            node->next->next    = NULL;
            list->end           = node->next;
        }
    }
}

void listAddNode(LinkedList* list, size_t cycles, EventFunc func){
    if(list->begin){
        if(list->begin->cycles > cycles){
                struct ListNode* tmp = list->begin;
                list->begin          = malloc(sizeof(struct ListNode));
                list->begin->cycles  = cycles;
                list->begin->func    = func;
                list->begin->next    = tmp;
        } else
            recursiveAddNode(list, list->begin, cycles, func);
    } else{
        list->begin         = malloc(sizeof(struct ListNode));
        list->end           = list->begin;
        list->begin->cycles = cycles;
        list->begin->func   = func;
        list->begin->next   = NULL;
    }
}

void listRemoveFirstNode(LinkedList* list){
    if(!list->begin)
        PANIC;
    struct ListNode* tmp = list->begin;
    list->begin = list->begin->next;
    free(tmp);
    list->end = list->begin ? list->end : list->begin;
}