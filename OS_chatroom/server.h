typedef struct ClientList {
    int data;
    struct ClientList* prev;
    struct ClientList* next;
    char ip[16];
    char name[31];
} ClientList;

ClientList *newNode(int skt, char* ip) {
    ClientList *np = (ClientList *)malloc( sizeof(ClientList) );
    np->data = skt;
    np->prev = NULL;
    np->next = NULL;
    strncpy(np->ip, ip, 16);
    strncpy(np->name, "NULL", 5);
    return np;
}
