#pragma once 
#include<utility.h>

typedef struct Scheduler Scheduler;
typedef void(*EventFunc)(Scheduler*);

struct ListNode{
    struct ListNode* next;
    size_t cycles;
    EventFunc func;
};

typedef struct LinkedList{
    struct ListNode* begin;
    struct ListNode* end;
    struct {
        struct ListNode* beg;
        struct ListNode* end;
    } internal;
} LinkedList;

void initList(LinkedList*);
void destroyList(LinkedList*);

void listAddNode(LinkedList*, size_t cycles, EventFunc func);

void listRemoveFirstNode(LinkedList*);