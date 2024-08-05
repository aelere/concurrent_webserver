//Geladze Lasha

#include <pthread.h>
#include "request.h"
#include "io_helper.h"

#define MAX_THREAD_AMOUNT 128
#define MAX_BUFFER_SIZE 512

#define DEFAULT_BASEDIR "."
#define DEFAULT_PORT 10000
#define DEFAULT_THREADS 1
#define DEFAULT_BUFFERS 1
#define DEFAULT_SCHEDALG "FIFO"

//This is a server that accepts connections
//and handles them using a thread pool
//it supports two scheduling algorithms: FIFO and SFF


// thread pool
pthread_t threads[MAX_THREAD_AMOUNT];

// command line arguments
char *root_dir;
int port;
int num_threads;
int num_buffers;
char *schedalg;

int count= 0;  //curr elements in queue
int in = 0;    //next free pos
int out = 0;  //next full pos

//mutex and condition variables
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_nonempty = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_nonfull = PTHREAD_COND_INITIALIZER;

// buffer entry
typedef struct {
    int fd;        //the connection file descriptor
    off_t size;    //file size
} BufferEntry;

// buffer that stores the connection file descriptors
BufferEntry buffer[MAX_BUFFER_SIZE];

// function that inserts the new request into the sorted array at the right place
// returns void and takes a BufferEntry as an argument
void sorted_insert(BufferEntry entry);

// function that peeks at the request and returns the file size
// returns a long and takes an int as an argument
long peek_at_request(int fd);

// function that parses the cmd arguments
// returns void and takes two arguments: an int and a char pointer
void parse_command_line_arguments(int argc, char *argv[]);

// worker threads that handle the request
// returns void and takes a void pointer as an argument
void *worker(void *arg);

// main function (thread) accepting the connections
int main(int argc, char *argv[]) {

    //first parse the command line arguments
    parse_command_line_arguments(argc, argv);

    // change current directory
    chdir_or_die(root_dir);

    // initialize worker threads
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, worker, NULL);
    }

    // set up the listening socket and accept connections
    int listen_fd = open_listen_fd_or_die(port);

    // accepting connections
    while (1) {
        // retrieve the client address
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        //get the connection file descriptor
        int conn_fd = accept_or_die(listen_fd, (struct sockaddr *)&client_addr, &client_len);

        // peek at the request and get the file size
        long size = peek_at_request(conn_fd);

        // lock the mutex, only one thread can access the buffer
        pthread_mutex_lock(&lock);

        // main thread waits until the buffer is full
        while (count == num_buffers) {
            pthread_cond_wait(&cond_nonfull, &lock);
        }

        printf("Accepted connection %d, size %lx\n", conn_fd, size);

        // insert the new request into the buffer
        BufferEntry entry = { .fd = conn_fd, .size = size };

        //depending on the scheduling algorithm, insert into the sorted buffer or normally
        if (strcmp(schedalg, "SFF") == 0) {
            sorted_insert(entry); // Insert the new request into the sorted array
        } else {
            // FIFO
            buffer[in] = entry;
            in = (in + 1) % num_buffers;
        }

        // the amount of elements in the buffer goes up
        count++;
        // send a signal that the buffer is not empty, unlock when accessing buffer is done
        pthread_cond_signal(&cond_nonempty);
        pthread_mutex_unlock(&lock);
    }
    return 0; // unreachable
}

void sorted_insert(BufferEntry entry) {
    // start from the last element
    int i = count - 1;
    // elements of buffer that are greater than entry move to one position ahead of their current
    while (i >= 0 && buffer[i].size > entry.size) {
        buffer[i + 1] = buffer[i];
        i--;
    }
    //i+1 because i is decremented one more time
    buffer[i + 1] = entry;
}

long peek_at_request(int fd) {
    char buf[MAXBUF], method[MAXBUF], uri[MAXBUF], version[MAXBUF];
    char filename[MAXBUF], cgiargs[MAXBUF];

    // use recv so that the file descriptor is not consumed
    ssize_t n = recv(fd, buf, MAXBUF - 1, MSG_PEEK);
    if (n < 0) {
        perror("recv");
        return -1;
    }
    buf[n] = '\0';  // null terminate

    //code copied from request.c
    sscanf(buf, "%s %s %s", method, uri, version);

    if (strstr(uri, "..") || uri[0] != '/') {
        return -1;  // Return -1 to indicate an error
    }

    if (!strstr(uri, "cgi")) {
        // static
        strcpy(cgiargs, "");
        if (uri[0] == '/') {
            sprintf(filename, ".%s", uri);
        } else {
            strcpy(filename, uri);
        }
        if (uri[strlen(uri)-1] == '/') {
            strcat(filename, "index.html");
        }
    } else {
        // dynamic
        char *ptr = index(uri, '?');
        if (ptr) {
            strcpy(cgiargs, ptr+1);
            *ptr = '\0';
        } else {
            strcpy(cgiargs, "");
        }
        if (uri[0] == '/') {
            sprintf(filename, ".%s", uri);
        } else {
            strcpy(filename, uri);
        }
    }

    //printf("Peeked filename: %s\n", filename);

    // retrieve the file size using stat
    struct stat sbuf;
    if (stat(filename, &sbuf) < 0) {
        perror("stat");
        // if stat fails
        return -1;
    }

    return sbuf.st_size;
}

void parse_command_line_arguments(int argc, char *argv[]) {
    // default values
    int opt;
    root_dir = DEFAULT_BASEDIR;
    port = DEFAULT_PORT;
    num_threads = DEFAULT_THREADS;
    num_buffers = DEFAULT_BUFFERS;
    schedalg = DEFAULT_SCHEDALG;

    // parse
    while ((opt = getopt(argc, argv, "d:p:t:b:s:")) != -1) {
        switch (opt) {
            case 'd':
                root_dir = optarg;
                break;
            case 'p':
                port = atoi(optarg);
                break;
            case 't':
                num_threads = atoi(optarg);
                if (num_threads <= 0 || num_threads > MAX_THREAD_AMOUNT) {
                    fprintf(stderr, "wrong number of threads.\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'b':
                num_buffers = atoi(optarg);
                if (num_buffers <= 0 || num_buffers > MAX_BUFFER_SIZE) {
                    fprintf(stderr, "wrong number of buffers.\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 's':
                schedalg = optarg;
                if (strcmp(schedalg, "FIFO") != 0 && strcmp(schedalg, "SFF") != 0) {
                    fprintf(stderr, "wrong scheduling algorithm.\n");
                    exit(EXIT_FAILURE);
                }
                break;
            default:
                fprintf(stderr, "Usage: %s [-d basedir] [-p port] [-t threads] [-b buffers]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
}

void *worker(void *arg) {
    // thread id for testing purposes
    //pthread_t tid = pthread_self();

    while (1) {
        pthread_mutex_lock(&lock);
        while (count == 0) { // wait if no items in the buffer
            pthread_cond_wait(&cond_nonempty, &lock);
        }

        BufferEntry conn_fd;
        if (strcmp(schedalg, "SFF") == 0) {
            // SFF : get the connection descriptor with the smallest file
            conn_fd = buffer[0]; // buffer is sorted by file size

            // all elements are shifted one position to the left
            for (int i = 0; i < count - 1; i++) {
                buffer[i] = buffer[i + 1];
            }
        } else {
            // FIFO : get the connection descriptor
            conn_fd = buffer[out];
            out = (out + 1) % num_buffers;
        }
        count--;

        pthread_cond_signal(&cond_nonfull); // signal that there is more space in the buffer now
        pthread_mutex_unlock(&lock);

        printf("Handling connection %d\n", conn_fd.fd);
        request_handle(conn_fd.fd); // handle the request
        printf("Done handling connection %d\n", conn_fd.fd);
        close_or_die(conn_fd.fd); // close
    }
    return NULL;    // unreachable
}


