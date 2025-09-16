#include "csapp.h"

typedef struct Stock {
    int id, quantity, price;
    struct Stock *left, *right;
} Stock;

Stock *root = NULL;
sem_t stock_sem;

void *client_thread(void *vargp);
void parse_request(int connfd, char *buf);
Stock *load_stocks(const char *filename);
Stock *make_stock(int id, int quantity, int price);
Stock *insert_stock(Stock *root, Stock *new_stock);
Stock *find_stock(Stock *node, int stock_id);
void free_stock(Stock *node);
void print_stocks(Stock *root, char *buf);
int buy_stock(Stock *root, int id, int num);
void sell_stock(Stock *root, int id, int num);
void save_stocks(const char *filename, Stock *root);

void sigint_handler(int sig) {
    P(&stock_sem);
    save_stocks("stock.txt", root);
    free_stock(root);
    V(&stock_sem);
    exit(0);
}

int main(int argc, char **argv) {
    int listenfd, *connfdp;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    char client_hostname[MAXLINE], client_port[MAXLINE];
    pthread_t tid;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    Signal(SIGINT, sigint_handler);

    Sem_init(&stock_sem, 0, 1);
    P(&stock_sem);
    root = load_stocks("stock.txt");
    V(&stock_sem);

    listenfd = Open_listenfd(argv[1]);

    while (1) {
        clientlen = sizeof(struct sockaddr_storage);
        connfdp = Malloc(sizeof(int));
        *connfdp = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *)&clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
        printf("Connected to (%s, %s)\n", client_hostname, client_port);
        Pthread_create(&tid, NULL, client_thread, connfdp);
    }
}

void *client_thread(void *vargp) {
    int connfd = *((int *)vargp);
    Pthread_detach(Pthread_self());
    Free(vargp);

    rio_t rio;
    Rio_readinitb(&rio, connfd);
    char buf[MAXBUF];
    int n;

    while ((n = Rio_readlineb(&rio, buf, MAXBUF)) > 0) {
        printf("server received %d bytes\n", (int)n);
        if (!strncmp(buf, "exit", 4)) {
            break;
        }
        parse_request(connfd, buf);
    }

    Close(connfd);
    return NULL;
}

void parse_request(int connfd, char *buf) {
    char order[20];
    int stock_id, num;

    P(&stock_sem);
    if (!strncmp(buf, "show", 4)) {
        buf[0] = '\0';
        print_stocks(root, buf);
        if (strlen(buf) == 0) {
            strcpy(buf, "No stocks available\n");
        } else {
            buf[strlen(buf) - 1] = '\n';
        }
    } else if (!strncmp(buf, "buy", 3)) {
        if (sscanf(buf, "%s %d %d", order, &stock_id, &num) == 3) {
            if (buy_stock(root, stock_id, num)) {
                strcpy(buf, "[buy] success\n");
            } else {
                strcpy(buf, "Not enough left stock\n");
            }
        } else {
            strcpy(buf, "Wrong Command!\n");
        }
    } else if (!strncmp(buf, "sell", 4)) {
        if (sscanf(buf, "%s %d %d", order, &stock_id, &num) == 3) {
            sell_stock(root, stock_id, num);
            strcpy(buf, "[sell] success\n");
        } else {
            strcpy(buf, "Wrong Command!\n");
        }
    } else {
        strcpy(buf, "Wrong Command!\n");
    }
    V(&stock_sem);
    Rio_writen(connfd, buf, MAXLINE);
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

Stock *find_stock(Stock *node, int stock_id) {
    if (!node) return NULL;
    if (node->id == stock_id) {
        return node;
    }
    if (node->id > stock_id) {
        return find_stock(node->left, stock_id);
    }
    return find_stock(node->right, stock_id);
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