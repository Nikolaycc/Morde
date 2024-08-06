#include <stdio.h>
#include "morde.h"

int main(void) {
    morde_instance();
    RadixTree *tree = radix_create_tree();
    Morde* app = morde_create();

    MordeRes index(MordeReq req) {
        MordeRes res = (MordeRes) {
            .body = "<h1>Hello, World</h1>",
        };

        return res;
    }
    radix_insert(tree, "/index", index);

    MordeRes info(MordeReq req) {
        MordeRes res = (MordeRes) {
            .body = req.body,
        };

        log_info(req.body);
        
        return res;
    }
    radix_insert(tree, "/info", info);

    MordeReq req = (MordeReq) {
        .body = "Connection-Type: keep-alive"
    };
    radix_execute_callback(tree, "/info", req);
    
    morde_run(app, "127.0.0.1", 8000);
    
    morde_close(app);
    return 0;
} 
