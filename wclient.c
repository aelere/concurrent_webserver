//
// client.c: A very, very primitive HTTP client.
// 
// To run, try: 
//      client hostname portnumber filename
//
// Sends one HTTP request to the specified HTTP server.
// Prints out the HTTP response.
//
// For testing your server, you will want to modify this client.  
// For example:
// You may want to make this multi-threaded so that you can 
// send many requests simultaneously to the server.
//
// You may also want to be able to request different URIs; 
// you may want to get more URIs from the command line 
// or read the list from a file. 
//
// When we test your server, we will be using modifications to this client.
//

#include <pthread.h>
#include "io_helper.h"

#define MAXBUF (8192)

typedef struct {
    char *host;
    int port;
    char *filename;
} HttpRequest;

void client_send(int fd, char *hostname, char *filename) {
    char buf[MAXBUF];
    sprintf(buf, "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", filename, hostname);
    write_or_die(fd, buf, strlen(buf));
}


void client_print(int fd) {
    char buf[MAXBUF];
    int n;

    while ((n = readline_or_die(fd, buf, MAXBUF)) > 0 && strcmp(buf, "\r\n")) {
        printf("%s", buf);
    }

    // Read and display the HTTP Body
    while ((n = readline_or_die(fd, buf, MAXBUF)) > 0) {
        printf("%s", buf);
    }
}

void *client_thread(void *arg) {
    HttpRequest *request = (HttpRequest *)arg;
    struct timeval tv_start;
    gettimeofday(&tv_start, NULL);

    int clientfd = open_client_fd_or_die(request->host, request->port);
    printf("[%ld.%06ld] Opened connection for %s\n", tv_start.tv_sec, tv_start.tv_usec, request->filename);

    client_send(clientfd, request->host, request->filename);  // Pass the hostname to client_send
    client_print(clientfd);

    close_or_die(clientfd);
    free(request);
    return NULL;
}


int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <host> <port> <filename1> [<filename2> ...]\n", argv[0]);
        exit(1);
    }

    int num_files = argc - 3;
    pthread_t *threads = malloc(num_files * sizeof(pthread_t));

    for (int i = 0; i < num_files; i++) {
        HttpRequest *request = malloc(sizeof(HttpRequest));
        request->host = argv[1];
        request->port = atoi(argv[2]);
        request->filename = argv[3 + i];

        pthread_create(&threads[i], NULL, client_thread, request);
    }

    for (int i = 0; i < num_files; i++) {
        pthread_join(threads[i], NULL);
    }

    free(threads);
    return 0;
}