#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>

typedef struct Station {
    char *name;
    char *auth;
    char *logfile;
    int port;
    int interface;
    int processed;
    int notMine;
    int formatErr;
    int noFwd;
} Station;

typedef struct Connected {
    char *name;
    int fd;
    struct Connected *next;
} Connected;

typedef struct Resource {
    char *name;
    int quantity;
    struct Resource *next;
} Resource;

typedef struct Threadinfo {
    int fd;
    FILE *fp;
    char *name;
    struct Station *station;
    struct Connected *connected;
    struct Resource *resource;
} Threadinfo;

/* global variable for semaphore*/
sem_t sem;
/* pointer of station information to pass in signal handler */
Station *sigStation;
/* pointer of connected station information to pass in signal handler */
Connected *sigConnected;
/* pointer of resource information to pass in signal handler */
Resource *sigResource;

/* takes in error code, then print stderr message and exit program */
void error(int errorCode) {
    switch (errorCode) {
        case 1:
            fprintf(stderr, "Usage: station name authfile logfile "\
                    "[port [host]]\n");
            exit(1);
            break;
        case 2:
            fprintf(stderr, "Invalid name/auth\n");
            exit(2);
            break;
        case 3:
            fprintf(stderr, "Unable to open log\n");
            exit(3);
            break;
        case 4:
            fprintf(stderr, "Invalid port\n");
            exit(4);
            break;
        case 5:
            fprintf(stderr, "Listen error\n");
            exit(5);
            break;
        case 6:
            fprintf(stderr, "Unable to connect to station\n");
            exit(6);
            break;
        case 7:
            fprintf(stderr, "Duplicate station names\n");
            exit(7);
            break;
        case 99:
            fprintf(stderr, "Unspecified system call failure\n");
            exit(8);
            break;
        case 0:
            exit(0);
            break;
    }
}

/*
 * use a dynamic buffer to read one line from given file pointer
 * return a char pointer for that line
 */
char *read_line(FILE *readF) {
    char ch;
    int len = 1;
    char *buffer = (char*)malloc(sizeof(char) * len);
    int i = 0;
    while (((ch = fgetc(readF)) != EOF) && (ch != '\n')) {
        buffer[i] = ch;
        i++;
        if (i == len) {
            len *= 2;
            buffer = (char*)realloc(buffer, sizeof(char) * len);
        }
    }
    if (ch == EOF) {
        return NULL;
    }
    buffer[i] = '\0';
    return buffer;
}


/*
 * add a new station's name and fd into connected station
 * linked list "head"
 */
void add_connected(Connected *head, char *n, int fd) {
    Connected *new, *pre;
    pre = head;
    if (pre->next != NULL) {
        while (pre->next->next != NULL) {
            if (strcmp(n, pre->next->name) < 0) {
                break;
            } else {
                pre = pre->next;
            }
        }
        if (!(strcmp(n, pre->next->name) < 0)) {
            pre = pre->next;
        }
    }

    if ((new = (Connected *)malloc(sizeof(Connected))) == NULL) {
        error(99);
    }
    new->name = n;
    new->fd = fd;
    new->next = pre->next;
    pre->next = new;
}

/*
 * check if the station name is already in connected station
 * linked list "head"
 * return 1 if already in the list, otherwise return 0
 */
int has_connected(Connected *head, char *n) {
    if (head->next == NULL) {
        return 0;
    }
    Connected *p = head->next;
    while ((p->next) != NULL) {
        if (strcmp(p->name, n) == 0) {
            return 1;
        }
        p = p->next;
    }
    if (strcmp(p->name, n) == 0) {
        return 1;
    } else {
        return 0;
    }
}

/*
 * remove the station that has name n from the connected station list head
 */
void remove_connected(Connected *head, char *n) {
    Connected *now, *pre;
    pre = head;
    if (head->next != NULL) {
        now = pre->next;
        while (now->next != NULL) {
            if (strcmp(n, now->name) == 0) {
                break;
            } else {
                pre = pre->next;
                now = now->next;
            }
        }
        if (strcmp(n, now->name) == 0) {
            pre->next = now->next;
        }
    }
}

/*
 * check and add the station into the connected station
 * linked list "head"
 */
void process_station(Connected *head, Station *station, char *n, int fd) {
    if (has_connected(head, n) || strcmp(n, station->name) == 0) {
        error(7);
    } else {
        add_connected(head, n, fd);
    }
}

/*
 * takes in a station name n, and return that station's fd
 */
int get_fd(Connected *head, char *n) {
    Connected *p = head->next;
    while ((p->next) != NULL) {
        if (strcmp(p->name, n) == 0) {
            return p->fd;
        }
        p = p->next;
    }
    return p->fd;
}


/*
 * add a new resource name and quantity into resource
 * linked list "head"
 */
void add_resource(Resource *head, char *n, int quantity) {
    Resource *new, *pre;
    pre = head;
    if (pre->next != NULL) {
        while (pre->next->next != NULL) {
            if (strcmp(n, pre->next->name) < 0) {
                break;
            } else {
                pre = pre->next;
            }
        }
        if (!(strcmp(n, pre->next->name) < 0)) {
            pre = pre->next;
        }
    }
    if ((new = (Resource *)malloc(sizeof(Resource))) == NULL) {
        error(99);
    }
    new->name = n;
    new->quantity = quantity;
    new->next = pre->next;
    pre->next = new;
}

/*
 * check if the resource name is already in resource
 * linked list "head"
 * return 1 if already in the list, otherwise return 0
 */
int has_resource(Resource *head, char *n) {
    if (head->next == NULL) {
        return 0;
    }
    Resource *p = head->next;
    while ((p->next) != NULL) {
        if (strcmp(p->name, n) == 0) {
            return 1;
        }
        p = p->next;
    }
    if (strcmp(p->name, n) == 0) {
        return 1;
    } else {
        return 0;
    }
}

/*
 * takes in a resource name n, and a number of quantity q,
 * update that resource's quantity in the linked list head
 */
void update_quantity(Resource *head, char *n, int q) {
    Resource *p = head->next;
    while ((p->next) != NULL) {
        if (strcmp(p->name, n) == 0) {
            p->quantity += q;
            return;
        }
        p = p->next;
    }
    p->quantity += q;
}

/*
 * check and load the resource into the resource
 * linked list "head"
 */
void process_resource(Resource *head, char *n, int q) {
    if (has_resource(head, n)) {
        update_quantity(head, n, q);
    } else {

        add_resource(head, n, q);
    }
}

/*
 * takes in a resources name n, and return that resource's quantity
 */
int get_quantity(Resource *head, char *n) {
    Resource *p = head->next;
    while ((p->next) != NULL) {
        if (strcmp(p->name, n) == 0) {
            return p->quantity;
        }
        p = p->next;
    }
    return p->quantity;
}

/*
 * given a exit Status, print the log append to the logfile
 */
void print_log(int exitStatus, Station *station, Connected *connected, 
        Resource *resource) {
    FILE *logfile = fopen(station->logfile, "a");
    if (logfile == NULL) {
        error(3);
    }
    fprintf(logfile, "=======\n");
    fprintf(logfile, "%s\n", station->name);
    fprintf(logfile, "Processed: %d\n", station->processed);
    fprintf(logfile, "Not mine: %d\n", station->notMine);
    fprintf(logfile, "Format err: %d\n", station->formatErr);
    fprintf(logfile, "No fwd: %d\n", station->noFwd);
    if (connected->next == NULL) {
        fprintf(logfile, "NONE\n");
    } else {
        Connected *p = connected->next;
        while ((p->next) != NULL) {
            fprintf(logfile, "%s,", p->name);
            p = p->next;
        }
        fprintf(logfile, "%s\n", p->name);
    }
    if (resource->next == NULL) {
    } else {
        Resource *p = resource->next;
        while ((p->next) != NULL) {
            fprintf(logfile, "%s %d\n", p->name, p->quantity);
            p = p->next;
        }
        fprintf(logfile, "%s %d\n", p->name, p->quantity);
    }
    if (exitStatus == 1) {
        fprintf(logfile, "doomtrain\n");
    } else if (exitStatus == 2) {
        fprintf(logfile, "stopstation\n");
    }
    fclose(logfile);
}

/*
 * interpret hot name to ip address,
 * if cannot interpret, return NULL,
 * otherwise, return the address pointer
 */
struct in_addr *name_to_ip_addr(char *hostname) {
    int error;
    struct addrinfo *addressInfo;

    error = getaddrinfo(hostname, NULL, NULL, &addressInfo);
    if (error) {
        return NULL;
    }
    return &(((struct sockaddr_in*)(addressInfo->ai_addr))->sin_addr);
}

/*
 * connect to the given ip address and port, return the fd
 */
int connect_to(struct in_addr *ipAddress, int port) {
    struct sockaddr_in socketAddr;
    int fd;
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        error(6);
    }
    socketAddr.sin_family = AF_INET;
    socketAddr.sin_port = htons(port);
    socketAddr.sin_addr.s_addr = ipAddress->s_addr;
    if (connect(fd, (struct sockaddr*)&socketAddr, sizeof(socketAddr)) < 0) {
        error(6);
    }
    return fd;
}

#define MAXHOSTNAMELEN 128

void *client_thread(void *arg);

/*
 * takes in a fd and get and print the binding port
 */
void print_port(int fd) {
    struct sockaddr_in add;
    socklen_t len = sizeof(add);
    if (getsockname(fd, (struct sockaddr *)&add, &len) < 0) {
        error(99);
    } else {
        printf("%d\n", ntohs(add.sin_port));
        fflush(stdout);
    }
}

/*
 * start to listen to the given port and interface(if is specified)
 */
int open_listen(int port, int argc, char *argv[]) {
    int fd, optVal = 1;
    struct sockaddr_in serverAddr;
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        error(5);
    }
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(int)) < 0) {
        error(5);
    }
    if (argc == 6) {
        struct addrinfo hints;
        struct addrinfo *result, *rp;
        memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_flags = AI_PASSIVE;
        hints.ai_protocol = 0;
        hints.ai_canonname = NULL;
        hints.ai_addr = NULL;
        hints.ai_next = NULL;
        if ((getaddrinfo(argv[5], argv[4], &hints, &result)) != 0) {
            error(99);
        }
        for (rp = result; rp != NULL; rp = rp->ai_next) {
            if (bind(fd, rp->ai_addr, rp->ai_addrlen) == 0) {
                break;
            }
        }
        if (rp == NULL) {
            error(5);
        }
        freeaddrinfo(result);
    } else {
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        if (bind(fd, (struct sockaddr*)&serverAddr, 
                sizeof(struct sockaddr_in)) < 0) {
            error(5);
        }
    }
    if (listen(fd, SOMAXCONN) < 0) {
        error(5);
    }
    print_port(fd);
    return fd;
}

/*
 * handle incoming connections, if connected successfully, add the station to
 * the linked list "connected", and start a new thread to deal with it.
 */
void process_connections(int fdServer, Station *station, Connected *connected,
        Resource *resource) {
    int fd;
    struct sockaddr_in fromAddr;
    socklen_t fromAddrSize;
    pthread_t threadId;
    while (1) {
        fromAddrSize = sizeof(struct sockaddr_in);
        fd = accept(fdServer, (struct sockaddr*)&fromAddr, &fromAddrSize);
        if (fd < 0) {
            error(99);
        }
        FILE *readF = fdopen(fd, "r");
        FILE *writeF = fdopen(fd, "w");
        char *buffer;
        buffer = read_line(readF);
        if (strlen(buffer) == 0) {
            close(fd);
            continue;
        }
        int a;
        if ((a = (strcmp(buffer, station->auth)) != 0)) {
            close(fd);
            continue;
        }
        buffer = read_line(readF);
        if (strlen(buffer) == 0) {
            close(fd);
            continue;
        }
        fprintf(writeF, "%s\n", station->name);
        fflush(writeF);
        sem_wait(&sem);
        process_station(connected, station, buffer, fd);
        sem_post(&sem);
        Threadinfo *info;
        if((info = (Threadinfo *)malloc(sizeof(Threadinfo))) == NULL) {
            error(99);
        }
        info->fd = fd;
        info->fp = readF;
        info->name = buffer;
        info->station = station;
        info->connected = connected;
        info->resource = resource;
        pthread_create(&threadId, NULL, client_thread, (void*)(int64_t)info);
        pthread_detach(threadId);
    }
}

/*
 * handle doomtrain, sent doomtrain to all connected stations
 */
void process_doom_train(char *str, Threadinfo *info) {
    if ((info->connected->next) != NULL) {
        Connected *p = info->connected->next;
        while ((p->next) != NULL) {
            FILE *writeF = fdopen(p->fd, "w");
            fprintf(writeF, "%s:doomtrain\n", p->name);
            fflush(writeF);
            p = p->next;
        }
        FILE *writeF = fdopen(p->fd, "w");
        fprintf(writeF, "%s:doomtrain\n", p->name);
        fflush(writeF);
    }
}

/*
 * check add-train's format. return 0 if the format is invalid
 * otherwise return 1
 */
int add_train_validation(char *str) {
    int isDigit = 1;
    int at = 0;
    int comma = 0;
    int count = 0;
    char *p = str;
    if (*p == '@' || *p == ',') {
        return 0;
    }
    for (; *p != '\0'; p++) {
        if (*p == '@') {
            if (count == 0) {
                return 0;
            }
            count = 0;
            isDigit = 0;
            at++;
        } else if (*p == ',') {
            if (count == 0) {
                return 0;
            }
            count = 0;
            isDigit = 1;
            if (at == 0) {
                return 0;
            }
            comma++;
        } else {
            if (isDigit == 1 && (*p < '0' || *p > '9')) {
                return 0;
            }
            count++;
        }
    }
    if (at - comma != 1) {
        return 0;
    }
    return 1;
}

/*
 * connect to the station with given hostname and port
 * return 1 if added successfully, otherwise return 0
 */
int connect_station(char *hostname, int port, Threadinfo *info) {
    struct in_addr *ipAddress = name_to_ip_addr(hostname);
    if (ipAddress == NULL) {
        error(6);
        return 0;
    }
    int fd;
    fd = connect_to(ipAddress, port);
    FILE *readF = fdopen(fd, "r");
    FILE *writeF = fdopen(fd, "w");
    fprintf(writeF, "%s\n%s\n", info->station->auth, info->station->name);
    fflush(writeF);
    char *buffer = read_line(readF);
    if (buffer != NULL) {
        process_station(info->connected, info->station, buffer, fd);
    } else {
        return 0;
    }
    pthread_t threadId;
    Threadinfo *infoNew;
    if((infoNew = (Threadinfo *)malloc(sizeof(Threadinfo))) == NULL) {
        error(99);
    }
    infoNew->fd = fd;
    infoNew->fp = readF;
    infoNew->name = buffer;
    infoNew->station = info->station;
    infoNew->connected = info->connected;
    infoNew->resource = info->resource;
    pthread_create(&threadId, NULL, client_thread, (void*)(int64_t)infoNew);
    pthread_detach(threadId);
    return 1;
}

/*
 * handle add train, check format first and then connect them
 */
int process_add_train(char *str, Threadinfo *info) {
    if (strchr(str, ')') == 0 || *(strchr(str, ')') + 1) != '\0' || 
            strchr(str, '@') == 0) {
        (info->station->formatErr)++;
        return 0;
    }
    str = str + 4;
    *(strchr(str, ')')) = '\0';
    if (add_train_validation(str) == 0) {
        (info->station->formatErr)++;
        return 0;
    }
    int stationNumber = 0;
    for (char *p = str; *p != '\0'; p++) {
        if (*p == ',') {
            stationNumber++;
        }
    }
    char *p = str;
    char *next = NULL;
    for (int i = 0; i < stationNumber; i++) {
        next = strchr(p, ',') + 1;
        *(strchr(p, ',')) = '\0';
        char *host = NULL;
        char *port = p;
        host = strchr(p, '@') + 1;
        *(strchr(p, '@')) = '\0';
        if (connect_station(host, atoi(port), info) != 1) {
            error(6);
        }
        p = next;
    }
    char *host = NULL;
    char *port = p;
    host = strchr(p, '@') + 1;
    *(strchr(p, '@')) = '\0';
    if (connect_station(host, atoi(port), info) != 1) {
        error(6);
    }
    return 1;
}

/*
 * check resource train's format. return 0 if the format is invalid
 * otherwise return 1
 */
int resource_train_validation(char *str) {
    int isDigit = 0;
    int operator = 0;
    int comma = 0;
    int count = 0;
    char *p = str;
    if (*p == '+' || *p == '-' || *p == ',') {
        return 0;
    }
    for (; *p != '\0'; p++) {
        if (*p == '+' || *p == '-') {
            if (count == 0) {
                return 0;
            }
            count = 0;
            isDigit = 1;
            operator++;
        } else if (*p == ',') {
            if (count == 0) {
                return 0;
            }
            count = 0;
            isDigit = 0;
            if (operator == 0) {
                return 0;
            }
            comma++;
        } else {
            if (isDigit == 1 && (*p < '0' || *p > '9')) {
                return 0;
            }
            count++;
        }
    }
    if (operator - comma != 1) {
        return 0;
    }
    return 1;
}

/*
 * handle resource train, check format first and then load/unlload them
 */
int process_resource_train(char *str, Threadinfo *info) {
    if (resource_train_validation(str) == 0) {
        (info->station->formatErr)++;
        return 0;
    }
    int resourceNumber = 0;
    for (char *p = str; *p != '\0'; p++) {
        if (*p == ',') {
            resourceNumber++;
        }
    }

    char *p = str;
    char *next = NULL;
    for (int i = 0; i < resourceNumber; i++) {
        next = strchr(p, ',') + 1;
        *(strchr(p, ',')) = '\0';
        char operator = (strchr(p, '+') != 0) ? '+' : '-';
        char *number = NULL;
        char *name = p;
        number = strchr(p, operator) + 1;
        *(strchr(p, operator)) = '\0';
        int quantity = (operator == '+') ? atoi(number) : (0 - atoi(number));
        process_resource(info->resource, name, quantity);
        p = next;
    }
    char operator = (strchr(p, '+') != 0) ? '+' : '-';
    char *number = NULL;
    char *name = p;
    number = strchr(p, operator) + 1;
    *(strchr(p, operator)) = '\0';
    int quantity = (operator == '+') ? atoi(number) : (0 - atoi(number));
    process_resource(info->resource, name, quantity);
    return 1;
}

/*
 * forward the string to other stations
 */
void process_fwd(char *str, Threadinfo *info) {
    if (strchr(str, ':')) {
        char *p = strchr(str, ':');
        *p = '\0';
        if (has_connected(info->connected, str)) {

            FILE *writeF = fdopen(get_fd(info->connected, str), "w");
            *p = ':';
            fprintf(writeF, "%s\n", str);
            fflush(writeF);
        } else {
            (info->station->noFwd)++;
            return;
        }
    } else {
        (info->station->noFwd)++;
        return;
    }
}

/*
 * main function to process a train string,
 * check the category of the train and handle it using
 * corresponding functions.
 */
void process_train(char *buffer, Threadinfo *info) {
    if (strchr(buffer, ':') == 0) {
        (info->station->formatErr)++;
        return;
    }
    if (strstr(buffer, info->station->name) == buffer && 
            *(buffer + strlen(info->station->name)) == ':') {
        char *current = buffer + strlen(info->station->name) + 1;
        char *next = NULL;
        if (strlen(current) == 0) {
            (info->station->formatErr)++;
            return;
        } else {
            if (strchr(current, ':')) {
                next = (strchr(current, ':')) + 1;
                *(strchr(current, ':')) = '\0';
            }
            int fwdStatus = 0, exitStatus = 0;
            if (strcmp(current, "doomtrain") == 0) {
                process_doom_train(current, info);
                (info->station->processed)++;
                exitStatus = 1;
            } else if (strcmp(current, "stopstation") == 0) {
                (info->station->processed)++;
                fwdStatus = 1;
                exitStatus = 2;
            } else if (strstr(current, "add(") == current) {
                if ((fwdStatus = process_add_train(current, info)) == 1) {
                    (info->station->processed)++;
                }
            } else if (strchr(current, '+') || strchr(current, '-')) {
                if ((fwdStatus = process_resource_train(current, info)) == 1) {
                    (info->station->processed)++;
                }
            } else {
                (info->station->formatErr)++;
                return;
            }
            if (next != NULL && fwdStatus) {
                process_fwd(next, info);
            }
            if (exitStatus) {
                print_log(exitStatus, info->station, 
                        info->connected, info->resource);
                exit(0);
            }
        }
    } else {
        (info->station->notMine)++;
    }
}

/*
 * read trains from connected station
 */
void *client_thread(void *arg) {
    Threadinfo *info;
    char *buffer;
    info = (Threadinfo *)(int64_t)arg;
    while (1) {
        if ((buffer = read_line(info->fp)) == NULL) {
            break;
        } else {
            sem_wait(&sem);
            process_train(buffer, info);
            sem_post(&sem);
        }
    }
    fflush(stdout);
    sem_wait(&sem);
    remove_connected(info->connected, info->name);
    sem_post(&sem);
    close(info->fd);
    pthread_exit(NULL);
    return NULL;
}

/*
 * read and get the auth string from the auth file
 */
void read_auth_file(FILE *authFile, Station *station) {
    int length = 0;
    char ch;
    while (((ch = fgetc(authFile)) != EOF) && (ch != '\n')) {
        length++;
    }
    if (length == 0) {
        error(2);
    }
    station->auth = (char *)malloc(sizeof(char) * (length + 1));
    rewind(authFile);
    int i = 0;
    while (((ch = fgetc(authFile)) != EOF) && (ch != '\r') && (ch != '\n')) {
        station->auth[i] = ch;
        i++;
    }
    station->auth[i] = '\0';
}

/* check if arguments are valid, if not, exit */
void check_argu(int argc, char *argv[], Station *station) {
    if (argc < 4 || argc > 6) {
        error(1);
    }
    if (argv[1] == NULL || strlen(argv[1]) == 0) {
        error(2);
    } else {
        station->name = argv[1];
    }

    FILE *authFile = fopen(argv[2], "r");
    if (authFile == NULL) {
        error(2);
    } else {
        read_auth_file(authFile, station);
        fclose(authFile);
    }

    FILE *temp = fopen(argv[3], "w");
    if (temp == NULL) {
        error(3);
    } 
    fclose(temp);
    station->logfile = argv[3];

    if (argc >= 5) {
        if (strlen(argv[4]) == 0) {
            error(4);
        }
        for (int i = 0; argv[4][i] != '\0'; i++) {
            if (argv[4][i] < '0' || argv[4][i] > '9') {
                error(4);
            }
        }
        if (atoi(argv[4]) <= 0 || atoi(argv[4]) >= 65535) {
            error(4);
        }
        station->port = atoi(argv[4]);
    }
    if (argc == 6) {
        station->interface = atoi(argv[5]);
    }
}

/* handle SIGHUP */
void sighup_handler(int sig) {
    sem_wait(&sem);
    print_log(0, sigStation, sigConnected, sigResource);
    sem_post(&sem);
}

int main(int argc, char *argv[]) {
    if (sem_init(&sem, 0, 1) == -1) {
        error(99);
    }

    Station station = {NULL, NULL, NULL, 0, 0, 0, 0, 0, 0};
    Connected connected = {NULL, -1, NULL};
    Resource resource = {NULL, 0, NULL};
    check_argu(argc, argv, &station);
    sigStation = &station;
    sigConnected = &connected;
    sigResource = &resource;

    struct sigaction sa;
    sa.sa_handler = &sighup_handler;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGHUP, &sa, 0);

    int fdServer;
    fdServer = open_listen(station.port, argc, argv);
    process_connections(fdServer, &station, &connected, &resource);
}