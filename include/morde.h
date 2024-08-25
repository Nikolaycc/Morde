#ifndef MORDE_H
#define MORDE_H

#include <stdio.h> 
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <assert.h>

#define LOG_USE_COLOR
#include "log.h"

#define MORDE_METHOD_SIZE 9
#define MORDE_SERV_MAX 3
#define MORDE_BUFF_SIZE 10000
#define MORDE_HEADER_MAX 21
#define RADIX_TREE_MAX_KEY_LENGTH 512
#define MORDE_HT_CAPACITY 650

#define morde_get(m, route, cb) morde_add_route(m, MGET, route, cb)
#define morde_post(m, route, cb) morde_add_route(m, MPOST, route, cb)
#define morde_delete(m, route, cb) morde_add_route(m, MDELETE, route, cb)
#define morde_options(m, route, cb) morde_add_route(m, MOPTIONS, route, cb)
#define morde_put(m, route, cb) morde_add_route(m, MPUT, route, cb)
#define morde_patch(m, route, cb) morde_add_route(m, MPATCH, route, cb)
#define morde_trace(m, route, cb) morde_add_route(m, MTRACE, route, cb)
#define morde_connect(m, route, cb) morde_add_route(m, MCONNECT, route, cb)
#define morde_head(m, route, cb) morde_add_route(m, MHEAD, route, cb)

typedef enum {
    MGET = 0,
    MPOST,
    MDELETE,
    MOPTIONS,
    MPUT,
    MPATCH,
    MTRACE,
    MCONNECT,
    MHEAD,
} MordeMethod;

static const char* __m_method_to_s[MORDE_METHOD_SIZE] = {
    [MGET] = "GET",
    [MPOST] = "POST",
    [MDELETE] = "DELETE",
    [MOPTIONS] = "OPTIONS",
    [MPUT] = "PUT",
    [MPATCH] = "PATCH",
    [MTRACE] = "TRACE",
    [MCONNECT] = "CONNECT",
    [MHEAD] = "HEAD",
};

typedef struct HtItem {
    char *key;
    char *value;
} HtItem;

typedef struct LinkedList {
    HtItem *item;
    struct LinkedList *next;
} LinkedList;

typedef struct {
    HtItem **items;
    LinkedList **overflowBuckets;
    int size;
    int count;
} HashTable;

typedef struct {
    char body[MORDE_BUFF_SIZE];
} MordeRes;

typedef struct {
    char method[9];
    HashTable header;
    char body[MORDE_BUFF_SIZE];
} MordeReq;

typedef MordeRes (*MordeCB)(MordeReq);

typedef struct RadixNode {
    struct RadixNode *children[512];
    MordeCB callback;
    char prefix[RADIX_TREE_MAX_KEY_LENGTH];
} RadixNode;

typedef struct RadixTree {
    RadixNode *root;
} RadixTree;

typedef struct {
    int connfd;
    RadixTree* routes;
} MordeConn;

typedef struct {
    int* fd;
    int port;
    RadixTree* routes;
} Morde;

typedef struct {
    int server_fd[MORDE_SERV_MAX];
    size_t len;
    Morde* mordes[MORDE_SERV_MAX];
} MordeInstance;

static MordeInstance __M_instance;

void morde_error(const char *msg);
void morde_instance();
Morde* morde_create();
void morde_close(Morde*);
void morde_handles(int);
void* morde_handlec(void*);
int morde_run(Morde*, const char*, int);
void morde_add_route(Morde*, MordeMethod, const char*, MordeCB);

// radix
RadixNode* radix_create_node();
RadixTree* radix_create_tree();
void radix_insert(RadixTree*, const char*, MordeCB);
void radix_execute_callback(RadixTree*, const char*, MordeReq);
MordeCB radix_get_callback(RadixTree*, const char*);
void radix_delete_node(RadixNode*);
void radix_delete_tree(RadixTree*);

// hashtable
unsigned long MordeHashFunction(char *str);
LinkedList *MordeAllocateList();
LinkedList *MordeLinkedListInsert(LinkedList *list, HtItem *item);
HtItem *MordeLinkedListRemove(LinkedList *list);
void MordeFreeLinkedList(LinkedList *list);
LinkedList **MordeCreateOverflowBuckets(HashTable *table);
void MordeFreeOverflowBuckets(HashTable *table);
HtItem *MordeCreateItem(char *key, char *value);
HashTable *MordeCreateTable(int size);
void MordeFreeItem(HtItem *item);
void MordeFreeTable(HashTable *table);
void MordeHandleCollision(HashTable *table, unsigned long index, HtItem *item);
void MordeInsert(HashTable *table, char *key, char *value);
char *MordeSearch(HashTable *table, char *key);
void MordeDelete(HashTable *table, char *key);
void MordePrintSearch(HashTable *table, char *key);
void MordePrintTable(HashTable *table);

#endif
