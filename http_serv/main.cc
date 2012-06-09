/* 
 * File:   main.cc
 * Author: zhenka
 *
 * Created on 9 Июнь 2012 г., 2:55
 */

#include <cstdlib>
#include <iostream>
#include <event.h>
#include <evhttp.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

using namespace std;

/*
 * 
 */
static void http_get_cb(struct evhttp_request *request, void *ctx);
void* pthread_event_base_dispatch(void *arg);

int main( int argc, char** argv )
{
    struct event_base *ev_base;
    struct evhttp *ev_http;
    
    if (argc != 3)
    {
        cerr << "usage: " << argv[0] << " port threads" << endl;
        exit(1);
    }
    
    int r;
    int nfd;
    uint16_t port = (uint16_t)atoi(argv[1]);
    nfd = socket(AF_INET, SOCK_STREAM, 0);
    if (nfd < 0) 
    {   
        cerr << "cant create socket" << endl;
        exit(1);
    }
    int one = 1;
    r = setsockopt(nfd, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(int));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    r = bind(nfd, (struct sockaddr*)&addr, sizeof(addr));
    if (r < 0) 
    {
        cerr << "cant bind" << endl;
        exit(1);
    }
    r = listen(nfd, 10240);
    if (r < 0)
    {
        cerr << "cant listen" << endl;
        exit(1);

    }

    int flags;
    if ((flags = fcntl(nfd, F_GETFL, 0)) < 0 || fcntl(nfd, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        cerr << "bag flags" << endl;
        exit(1);

    }
    
    uint32_t nthreads = (u_int16_t)atoi(argv[2]);
    pthread_t ths[nthreads];
    for (int i = 0; i < nthreads; i++) 
    {
        struct event_base *ev_base = event_init();
        if (ev_base == NULL) 
        {
            cerr << "cant event_init()" << endl;
            exit(1);
        }
        
        struct evhttp *ev_httpd = evhttp_new(ev_base);
        if (ev_httpd == NULL) 
        {
            cerr << "cant evhttp_new()" << endl;
            exit(1);
        }
        
        r = evhttp_accept_socket(ev_httpd, nfd);
        if (r != 0) 
        {
            cerr << "cant evhttp_accept_socket()" << endl;
            exit(1);
        }
        
        evhttp_set_cb(ev_httpd, "/get", http_get_cb, NULL);
        r = pthread_create(&ths[i], NULL, pthread_event_base_dispatch, ev_base);
        if (r != 0)
        {
            cerr << "cant pthread_create()" << endl;
            exit(1);
        }
    }
    
    for (int i = 0; i < nthreads; i++) 
    {
        pthread_join(ths[i], NULL);
    }
    
    return 0;
}

void* pthread_event_base_dispatch(void *arg) 
{
    event_base_dispatch((struct event_base*)arg);
    return NULL;
}

static void http_get_cb(struct evhttp_request *request, void *ctx)
{
    struct evbuffer *evb;
    evb = evbuffer_new();
    
    evbuffer_add_printf(evb, "uri: %s", request->uri);
    
    //cerr << request->uri << endl;
    
    evhttp_send_reply(request, HTTP_OK, "HTTP_OK", evb);
    evbuffer_free(evb);
}


