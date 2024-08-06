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

#define MORDE_SERV_MAX 3

#define MORDE_BUFF_SIZE 1000000

#define RADIX_TREE_MAX_KEY_LENGTH 256

typedef struct {
    char body[MORDE_BUFF_SIZE];
} MordeRes;

typedef struct {
    char body[MORDE_BUFF_SIZE];
} MordeReq;

typedef MordeRes (*MordeCB)(MordeReq);

typedef struct RadixNode {
    struct RadixNode *children[256];
    MordeCB callback;
    char prefix[RADIX_TREE_MAX_KEY_LENGTH];
} RadixNode;

typedef struct RadixTree {
    RadixNode *root;
} RadixTree;

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

// radix
RadixNode* radix_create_node();
RadixTree* radix_create_tree();
void radix_insert(RadixTree*, const char*, MordeCB);
void radix_execute_callback(RadixTree*, const char*, MordeReq);
MordeCB radix_get_callback(RadixTree*, const char*);
void radix_delete_node(RadixNode*);
void radix_delete_tree(RadixTree*);

#endif
