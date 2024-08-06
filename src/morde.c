
#include "morde.h"

void morde_handles(int sig) {
    for (int i = 0; i < __M_instance.len; i++) {
        morde_close(__M_instance.mordes[i]);
    }
    
    exit(0);
}

void morde_instance() {
    __M_instance = (MordeInstance) {
        .len = 0,
    };

    memset(&__M_instance.server_fd, 0, MORDE_SERV_MAX);
}

void morde_error(const char *msg) {
    log_error(msg);
    exit(1);
}

Morde* morde_create() {
    assert(__M_instance.len < MORDE_SERV_MAX);
    __M_instance.server_fd[__M_instance.len] = socket(AF_INET, SOCK_STREAM, 0);
    if (__M_instance.server_fd[__M_instance.len] == -1)
        morde_error("Socket creation failed."); 

    Morde* m = (Morde*) malloc(sizeof(Morde));
    m->fd = &__M_instance.server_fd[__M_instance.len];
    m->port = 0;

    m->routes = radix_create_tree();
    __M_instance.mordes[__M_instance.len] = m;
    
    signal(SIGINT, morde_handles);
    log_info("Created Morde App!");

    __M_instance.len++;
    return m;
}

void morde_close(Morde* m) {
    log_error("Closing server socket and exiting...");
    for (int i = 0; i < __M_instance.len; i++) {
        close(__M_instance.server_fd[i]);
    }
    radix_delete_tree(m->routes);
    free(m);
}

void* morde_handlec(void* arg) {
    int conn = *(int*) arg;
    log_info("New Connction.");

    close(conn);
}

int morde_run(Morde* m, const char* addr, int port) {
    struct sockaddr_in servaddr, cliaddr;
    int opt = 1;
    memset(&servaddr, 0, sizeof(servaddr)); 
   
    servaddr.sin_family = AF_INET;  
    servaddr.sin_port = htons(port);
    inet_aton(addr, (struct in_addr *)&servaddr.sin_addr.s_addr);

    if (setsockopt(*m->fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        morde_error("Setting socket options failed.");
    
    if ((bind(*m->fd, (struct sockaddr*)&servaddr, sizeof(servaddr))) != 0) 
        morde_error("Socket bind failed."); 

    if ((listen(*m->fd, 5)) != 0)
        morde_error("Listen failed.");

    int len = sizeof(cliaddr); 

    while (1) {
        log_trace("listeing..");
        pthread_t thread_id;
        int connfd = accept(*m->fd, (struct sockaddr*)&cliaddr, &len); 
        if (connfd < 0) {
            morde_error("Server accept failed.");
        }

        void* arg = &connfd;
        if (pthread_create(&thread_id, NULL, morde_handlec, arg) < 0) {
            morde_error("Thread creation failed.");
        }
            
        pthread_detach(thread_id);
    }
}

RadixNode* radix_create_node() {
    RadixNode *node = (RadixNode *)malloc(sizeof(RadixNode));
    memset(node, 0, sizeof(RadixNode));
    node->callback = NULL;
    return node;
}

RadixTree* radix_create_tree() {
    RadixTree *tree = (RadixTree *)malloc(sizeof(RadixTree));
    tree->root = radix_create_node();
    return tree;
}

void radix_insert(RadixTree* tree, const char* key, MordeCB cb) {
    RadixNode *current = tree->root;
    while (*key) {
        RadixNode *next = current->children[(unsigned char)*key];
        if (!next) {
            next = radix_create_node();
            current->children[(unsigned char)*key] = next;
        }
        current = next;
        key++;
    }
    current->callback = cb;
}

void radix_execute_callback(RadixTree* tree, const char* key, MordeReq req) {
    RadixNode *current = tree->root;
    while (*key) {
        RadixNode *next = current->children[(unsigned char)*key];
        if (!next) {
            printf("Callback not found for key: %s\n", key);
        }
        current = next;
        key++;
    }
    if (current->callback) {
        current->callback(req);
    } else {
        printf("No callback associated with key: %s\n", key);
    }
}

MordeCB radix_get_callback(RadixTree* tree, const char* key) {
    RadixNode *current = tree->root;
    while (*key) {
        RadixNode *next = current->children[(unsigned char)*key];
        if (!next) {
            printf("Callback not found for key: %s\n", key);
            return NULL; // Key not found
        }
        current = next;
        key++;
    }
    if (current->callback) {
        return current->callback; // Return the callback
    } else {
        printf("No callback associated with key: %s\n", key);
        return NULL;
    }
}

void radix_delete_node(RadixNode* node) {
    if (!node) return;
    for (int i = 0; i < 256; i++) {
        radix_delete_node(node->children[i]);
    }
    free(node);
}

void radix_delete_tree(RadixTree* tree) {
    radix_delete_node(tree->root);
    free(tree);
}
