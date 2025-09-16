#include "csapp.h"

typedef struct Stock {
    int id, quantity, price;
    struct Stock *left, *right;
} Stock;

typedef struct {
    int maxfd;
    fd_set read_set;
    fd_set ready_set;
    int nready;
    int maxi;
    int clientfd[FD_SETSIZE];
    rio_t clientrio[FD_SETSIZE];
} pool;

Stock *root = NULL;

void init_pool(int listenfd, pool *p);
void add_client(int connfd, pool *p);
void check_clients(pool *p);
void parse_request(int connfd, char *buf);
Stock *load_stocks(const char *filename);
Stock *make_stock(int id, int quantity, int price);
Stock *insert_stock(Stock *root, Stock *new_stock);
Stock *find_stock(Stock *node, int id);
void free_stock(Stock *node);
void print_stocks(Stock *root, char *buf);
int buy_stock(Stock *root, int id, int num);
void sell_stock(Stock *root, int id, int num);
void save_stocks(const char *filename, Stock *root);
int no_connections(pool *p);

void sigint_handler(int sig) {
    save_stocks("stock.txt", root);
    free_stock(root);
    exit(0);
}

int main(int argc, char **argv) {
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    char client_hostname[MAXLINE], client_port[MAXLINE];
    static pool pool;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    Signal(SIGINT, sigint_handler);

    root = load_stocks("stock.txt");
    listenfd = Open_listenfd(argv[1]);
    init_pool(listenfd, &pool);

    while (1) {
        pool.ready_set = pool.read_set;
        pool.nready = Select(pool.maxfd + 1, &pool.ready_set, NULL, NULL, NULL);

        if (FD_ISSET(listenfd, &pool.ready_set)) {
            clientlen = sizeof(struct sockaddr_storage);
            connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
            Getnameinfo((SA *)&clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
            printf("Connected to (%s, %s)\n", client_hostname, client_port);
            add_client(connfd, &pool);
        }

        check_clients(&pool);

        if (no_connections(&pool)) {
            save_stocks("stock.txt", root);
        }
    }
}

void init_pool(int listenfd, pool *p) {
    p->maxi = -1;
    for (int i = 0; i < FD_SETSIZE; i++) {
        p->clientfd[i] = -1;
    }
    p->maxfd = listenfd;
    FD_ZERO(&p->read_set);
    FD_ZERO(&p->ready_set);
    FD_SET(listenfd, &p->read_set);
}

void add_client(int connfd, pool *p) {
    int i;
    p->nready--;
    for (i = 0; i < FD_SETSIZE; i++) {
        if (p->clientfd[i] < 0) {
            p->clientfd[i] = connfd;
            Rio_readinitb(&p->clientrio[i], connfd);
            FD_SET(connfd, &p->read_set);
            if (connfd > p->maxfd) {
                p->maxfd = connfd;
            }
            if (i > p->maxi) {
                p->maxi = i;
            }
            return;
        }
    }
    Close(connfd);
    app_error("add_client error: Too many clients");
}

void check_clients(pool *p) {
    int connfd, n;
    char buf[MAXBUF];
    rio_t *rio;

    for (int i = 0; (i <= p->maxi) && (p->nready > 0); i++) {
        connfd = p->clientfd[i];
        rio = &p->clientrio[i];
        if ((connfd > 0) && (FD_ISSET(connfd, &p->ready_set))) {
            p->nready--;
            buf[0] = '\0';
            n = Rio_readlineb(rio, buf, MAXBUF);
            printf("server received %d bytes\n", (int)n);
            if (n <= 0 || strncmp(buf, "exit", 4) == 0) {
                Close(connfd);
                FD_CLR(connfd, &p->read_set);
                p->clientfd[i] = -1;
                continue;
            }
            parse_request(connfd, buf);
        }
    }
}

void parse_request(int connfd, char *buf) {
    char order[20];
    int id, num;

    if (!strncmp(buf, "show", 4)) {
        buf[0] = '\0';
        print_stocks(root, buf);
        if (strlen(buf) == 0) {
            strcpy(buf, "No stocks available\n");
        } else {
            buf[strlen(buf) - 1] = '\n';
        }
        Rio_writen(connfd, buf, MAXLINE);
    } else if (!strncmp(buf, "buy", 3)) {
        if (sscanf(buf, "%s %d %d", order, &id, &num) == 3) {
            if (buy_stock(root, id, num)) {
                strcpy(buf, "[buy] success\n");
            } else {
                strcpy(buf, "Not enough left stock\n");
            }
        } else {
            strcpy(buf, "Wrong Command!\n");
        }
        Rio_writen(connfd, buf, MAXLINE);
    } else if (!strncmp(buf, "sell", 4)) {
        if (sscanf(buf, "%s %d %d", order, &id, &num) == 3) {
            sell_stock(root, id, num);
            strcpy(buf, "[sell] success\n");
        } else {
            strcpy(buf, "Wrong Command!\n");
        }
        Rio_writen(connfd, buf, MAXLINE);
    } else {
        strcpy(buf, "Wrong Command!\n");
        Rio_writen(connfd, buf, MAXLINE);
    }
}

Stock *load_stocks(const char *filename) {
    FILE *fp = Fopen(filename, "r");
    Stock *root = NULL;
    char line[100];
    int id, quantity, price;

    while (Fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "%d %d %d", &id, &quantity, &price) == 3) {
            Stock *new_stock = make_stock(id, quantity, price);
            root = insert_stock(root, new_stock);
        }
    }
    Fclose(fp);
    return root;
}

Stock *make_stock(int id, int quantity, int price) {
    Stock *new_stock = Malloc(sizeof(Stock));
    new_stock->id = id;
    new_stock->quantity = quantity;
    new_stock->price = price;
    new_stock->left = new_stock->right = NULL;
    return new_stock;
}

Stock *insert_stock(Stock *root, Stock *new_stock) {
    if (!root) return new_stock;
    if (root->id < new_stock->id) {
        root->right = insert_stock(root->right, new_stock);
    } else if (root->id > new_stock->id) {
        root->left = insert_stock(root->left, new_stock);
    } else {
        root->price = new_stock->price;
        root->quantity = new_stock->quantity;
    }
    return root;
}

Stock *find_stock(Stock *node, int id) {
    if (!node) return NULL;
    if (node->id == id) {
        return node;
    }
    if (node->id > id) {
        return find_stock(node->left, id);
    }
    return find_stock(node->right, id);
}

void free_stock(Stock *node) {
    if (!node) return;
    free_stock(node->left);
    free_stock(node->right);
    Free(node);
}

void print_stocks(Stock *root, char *buf) {
    if (!root) return;
    char line[100];
    print_stocks(root->left, buf);
    sprintf(line, "%d %d %d\n", root->id, root->quantity, root->price);
    strcat(buf, line);
    print_stocks(root->right, buf);
}

int buy_stock(Stock *root, int id, int num) {
    Stock *buy_stock = find_stock(root, id);
    if (!buy_stock || buy_stock->quantity < num) {
        return 0;
    }
    buy_stock->quantity -= num;
    return 1;
}

void sell_stock(Stock *root, int id, int num) {
    Stock *sell_stock = find_stock(root, id);
    if (sell_stock) {
        sell_stock->quantity += num;
    }
}

void save_stocks(const char *filename, Stock *root) {
    FILE *fp = Fopen(filename, "w");
    char buf[MAXBUF] = {0};
    print_stocks(root, buf);
    Fputs(buf, fp);
    Fclose(fp);
}

int no_connections(pool *p) {
    for (int i = 0; i <= p->maxi; i++) {
        if (p->clientfd[i] != -1) {
            return 0;
        }
    }
    return 1;
}