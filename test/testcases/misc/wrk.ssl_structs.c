//
// Created by akhin on 3/14/16.
//

#include <openssl/ssl.h>

typedef struct connection {
    SSL* ssl;
} connection;

int main() {
    connection* c = malloc(sizeof(connection));
    return c->ssl->client_version;
}
