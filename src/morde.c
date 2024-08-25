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
    MordeConn conn = *(MordeConn*) arg;
    free(arg);
    log_debug("New Connction. SOCKFD = %d", conn.connfd);

    char buf[MORDE_BUFF_SIZE];

    memset(buf, 0, MORDE_BUFF_SIZE);
    int cliread = read(conn.connfd, buf, MORDE_BUFF_SIZE);
    log_debug("Conn Read Size %d", cliread);
    if (cliread < 0)
        morde_error("Read failed.");
    else {
        log_info("BODY: %s", buf);
    }

    char mr[20] = "";
    for (int i = 0; i < MORDE_BUFF_SIZE; i++) {
        if (strcmp(buf[i], "\n") == 0) {
            // for (int n = 0; n < i; i++) {
            //     mr[n] = buf[n];
            // }
            break;
        }
    }
    log_info("Parsed %s", mr);
    
    close(conn.connfd);
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

        MordeConn* tconn = malloc(sizeof(MordeConn));
        tconn->connfd = connfd;
        tconn->routes = m->routes;
        
        if (pthread_create(&thread_id, NULL, morde_handlec, (void*)tconn) < 0) {
            morde_error("Thread creation failed.");
        }
            
        pthread_detach(thread_id);
    }
}

void morde_add_route(Morde* m, MordeMethod method, const char* route, MordeCB cb) {
    log_info("Added Route %s %s", __m_method_to_s[method], route);
}

RadixNode* radix_create_node() {
    RadixNode* node = malloc(sizeof(RadixNode));
    for (int i = 0; i < 512; i++) {
        node->children[i] = NULL;
    }
    node->callback = NULL;
    return node;
}

RadixTree* radix_create_tree() {
    RadixTree* tree = malloc(sizeof(RadixTree));
    tree->root = radix_create_node();
    return tree;
}

void radix_insert(RadixTree* tree, const char* key, MordeCB callback) {
    RadixNode* node = tree->root;
    while (*key) {
        if (node->children[(unsigned char)*key] == NULL) {
            node->children[(unsigned char)*key] = radix_create_node();
        }
        node = node->children[(unsigned char)*key];
        key++;
    }
    node->callback = callback;
}

void radix_execute_callback(RadixTree* tree, const char* key, MordeReq req) {
    MordeCB cb = radix_get_callback(tree, key);
    if (cb) {
        MordeRes res = cb(req);
        log_info("Response: %s", res.body);
    } else {
        log_error("Callback not found for %s", key);
    }
}

MordeCB radix_get_callback(RadixTree* tree, const char* key) {
    RadixNode* node = tree->root;
    while (*key && node) {
        node = node->children[(unsigned char)*key];
        key++;
    }
    return node ? node->callback : NULL;
}

void radix_delete_node(RadixNode* node) {
    for (int i = 0; i < 512; i++) {
        if (node->children[i]) {
            radix_delete_node(node->children[i]);
        }
    }
    free(node);
}

void radix_delete_tree(RadixTree* tree) {
    radix_delete_node(tree->root);
    free(tree);
}

// Hashtable implementation
unsigned long MordeHashFunction(char *str) {
    unsigned long i = 0;
    for (int j = 0; str[j]; j++) {
        i += str[j];
    }
    return i % MORDE_HT_CAPACITY;
}

LinkedList *MordeAllocateList() {
    return (LinkedList*) malloc(sizeof(LinkedList));
}

LinkedList *MordeLinkedListInsert(LinkedList *list, HtItem *item) {
    if (!list) {
        LinkedList* newList = MordeAllocateList();
        newList->item = item;
        newList->next = NULL;
        list = newList;
    } else if (!list->next) {
        LinkedList* node = MordeAllocateList();
        node->item = item;
        node->next = NULL;
        list->next = node;
    } else {
        list->next = MordeLinkedListInsert(list->next, item);
    }
    return list;
}

HtItem *MordeLinkedListRemove(LinkedList *list) {
    if (!list) return NULL;
    if (!list->next) return NULL;

    LinkedList *node = list->next;
    HtItem *item = node->item;
    list->next = NULL;
    free(node);
    return item;
}

void MordeFreeLinkedList(LinkedList *list) {
    LinkedList *temp = list;
    while (list) {
        temp = list;
        list = list->next;
        free(temp->item->key);
        free(temp->item->value);
        free(temp->item);
        free(temp);
    }
}

LinkedList **MordeCreateOverflowBuckets(HashTable *table) {
    LinkedList **buckets = (LinkedList**) calloc(table->size, sizeof(LinkedList*));
    for (int i = 0; i < table->size; i++) {
        buckets[i] = NULL;
    }
    return buckets;
}

void MordeFreeOverflowBuckets(HashTable *table) {
    LinkedList **buckets = table->overflowBuckets;
    for (int i = 0; i < table->size; i++) {
        MordeFreeLinkedList(buckets[i]);
    }
    free(buckets);
}

HtItem *MordeCreateItem(char *key, char *value) {
    HtItem *item = (HtItem*) malloc(sizeof(HtItem));
    item->key = strdup(key);
    item->value = strdup(value);
    return item;
}

HashTable *MordeCreateTable(int size) {
    HashTable *table = (HashTable*) malloc(sizeof(HashTable));
    table->size = size;
    table->count = 0;
    table->items = (HtItem**) calloc(table->size, sizeof(HtItem*));

    for (int i = 0; i < table->size; i++) {
        table->items[i] = NULL;
    }
    table->overflowBuckets = MordeCreateOverflowBuckets(table);
    return table;
}

void MordeFreeItem(HtItem *item) {
    free(item->key);
    free(item->value);
    free(item);
}

void MordeFreeTable(HashTable *table) {
    for (int i = 0; i < table->size; i++) {
        HtItem *item = table->items[i];
        if (item != NULL)
            MordeFreeItem(item);
    }

    MordeFreeOverflowBuckets(table);
    free(table->items);
    free(table);
}

void MordeHandleCollision(HashTable *table, unsigned long index, HtItem *item) {
    LinkedList *head = table->overflowBuckets[index];

    if (head == NULL) {
        head = MordeAllocateList();
        head->item = item;
        table->overflowBuckets[index] = head;
        return;
    } else {
        table->overflowBuckets[index] = MordeLinkedListInsert(head, item);
        return;
    }
}

void MordeInsert(HashTable *table, char *key, char *value) {
    HtItem *item = MordeCreateItem(key, value);
    unsigned long index = MordeHashFunction(key);
    HtItem *current_item = table->items[index];

    if (current_item == NULL) {
        if (table->count == table->size) {
            printf("Insert Error: Hash Table is full\n");
            MordeFreeItem(item);
            return;
        }

        table->items[index] = item;
        table->count++;
    } else {
        if (strcmp(current_item->key, key) == 0) {
            strcpy(table->items[index]->value, value);
            return;
        } else {
            MordeHandleCollision(table, index, item);
            return;
        }
    }
}

char *MordeSearch(HashTable *table, char *key) {
    unsigned long index = MordeHashFunction(key);
    HtItem *item = table->items[index];
    LinkedList *head = table->overflowBuckets[index];

    while (item != NULL) {
        if (strcmp(item->key, key) == 0)
            return item->value;
        if (head == NULL)
            return NULL;
        item = head->item;
        head = head->next;
    }
    return NULL;
}

void MordeDelete(HashTable *table, char *key) {
    unsigned long index = MordeHashFunction(key);
    HtItem *item = table->items[index];
    LinkedList *head = table->overflowBuckets[index];

    if (item == NULL) {
        return;
    } else {
        if (head == NULL && strcmp(item->key, key) == 0) {
            table->items[index] = NULL;
            MordeFreeItem(item);
            table->count--;
            return;
        } else if (head != NULL) {
            if (strcmp(item->key, key) == 0) {
                MordeFreeItem(item);
                LinkedList *node = head;
                head = head->next;
                node->next = NULL;
                table->items[index] = MordeAllocateList();
                table->items[index] = node->item;
                table->overflowBuckets[index] = head;
                return;
            }

            LinkedList *curr = head;
            LinkedList *prev = NULL;

            while (curr) {
                if (strcmp(curr->item->key, key) == 0) {
                    if (prev == NULL) {
                        MordeFreeItem(curr->item);
                        MordeLinkedListRemove(curr);
                        table->overflowBuckets[index] = curr->next;
                        return;
                    } else {
                        prev->next = curr->next;
                        MordeFreeItem(curr->item);
                        MordeLinkedListRemove(curr);
                        return;
                    }
                }
                prev = curr;
                curr = curr->next;
            }
        }
    }
}

void MordePrintSearch(HashTable *table, char *key) {
    char *val;
    if ((val = MordeSearch(table, key)) == NULL) {
        printf("%s does not exist\n", key);
        return;
    } else {
        printf("Key:%s, Value:%s\n", key, val);
    }
}

void MordePrintTable(HashTable *table) {
    printf("\nHash Table\n-------------------\n");
    for (int i = 0; i < table->size; i++) {
        if (table->items[i]) {
            printf("Index:%d, Key:%s, Value:%s", i, table->items[i]->key, table->items[i]->value);
            if (table->overflowBuckets[i]) {
                printf(" => Overflow Bucket => ");
                LinkedList *head = table->overflowBuckets[i];
                while (head) {
                    printf("Key:%s, Value:%s ", head->item->key, head->item->value);
                    head = head->next;
                }
            }
            printf("\n");
        }
    }
    printf("-------------------\n");
}
