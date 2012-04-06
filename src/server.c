#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <arpa/inet.h> 
#include "picoev/picoev.h"

#include "http_parser/http_parser.h"

#define HOST 0 /* 0x7f000001 for localhost */
#define PORT 8080
#define MAX_FDS 1024 * 4
#define TIMEOUT_SECS 60
#define BACKLOG_SIZE 1024 * 5

#define HELLO_RESPONSE \
	"HTTP/1.1 200 OK\r\n" \
	"Content-Type: text/plain\r\n" \
	"Content-Length: 12\r\n" \
	"\r\n" \
	"Hello world\n"

typedef struct{
	char *data;
	int len;
} buf_t;

typedef struct{
	int fd;
	picoev_loop *loop;
	char *remote_addr;
	int remote_port;
	http_parser parser;
} client_t;

static buf_t resbuf;


static void setup_sock(int fd)
{
  	int on = 1, r;
  	r = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));
  	assert(r == 0);
  	r = fcntl(fd, F_SETFL, O_NONBLOCK);
  	assert(r == 0);
}

static void close_conn(client_t *cli)
{
  	picoev_del(cli->loop, cli->fd);
  	close(cli->fd);
  	free(cli);
	printf("\nclosed: %d\n", cli->fd);
}


static void write_cb(picoev_loop* loop, int fd, int events, void* cb_arg)
{
	client_t *client = (client_t *)cb_arg;
	resbuf.data = HELLO_RESPONSE;
	resbuf.len = sizeof(HELLO_RESPONSE);
	if (write(client->fd, resbuf.data, resbuf.len) != resbuf.len) {
        close_conn(client); /* failed to send all data at once, close */
		printf("\nclose by write failed");
		return;
	}
    close_conn(client); 
}


int headers_complete_cb(http_parser* parser)
{
	client_t *client = (client_t *)parser->data;	
	printf("\nUserIP:%s", client->remote_addr);
	picoev_del(client->loop, client->fd);
	picoev_add(client->loop, client->fd, PICOEV_WRITE, 0, write_cb, (void *)client);
	return 1;
}

static http_parser_settings parser_settings = {
	.on_headers_complete = headers_complete_cb
};


static void rw_callback(picoev_loop* loop, int fd, int events, void* cb_arg)
{
	client_t *client = (client_t *)cb_arg;
  if ((events & PICOEV_TIMEOUT) != 0) {
    /* timeout */
    close_conn(client);
    
  } else if ((events & PICOEV_READ) != 0) {
    
    /* update timeout, and read */
    char buf[1024];
    ssize_t r;
    picoev_set_timeout(loop, fd, TIMEOUT_SECS);
    r = read(fd, buf, sizeof(buf));
    switch (r) {
		case 0: /* connection closed by peer */
		  close_conn(client);
		  printf("\nclosed by peer");
		  break;
		case -1: /* error */
		  if (errno == EAGAIN || errno == EWOULDBLOCK) { /* try again later */
			break;
		  } else { /* fatal error */
			close_conn(client);
			printf("\nclose by fatal error");
		  }
		  break;
		default: /* got some data, send back */
			http_parser_execute(&client->parser, &parser_settings, buf, r);
		  break;
    }
  
  }
}

static void accept_callback(picoev_loop* loop, int fd, int events, void* cb_arg)
{	
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
	char *remote_addr;
  	int client_fd = accept(fd, (struct sockaddr *)&client_addr, &client_len);
  	if (client_fd != -1) {
    	printf("connected: %d\n", client_fd);
    	setup_sock(client_fd);
		client_t *client = malloc(sizeof(client_t));
		client->loop = loop;
		remote_addr = inet_ntoa(client_addr.sin_addr);
		client->remote_addr = remote_addr;
		client->remote_port = ntohs(client_addr.sin_port);
		client->parser.data = client;
		client->fd = client_fd;
		http_parser_init(&client->parser, HTTP_REQUEST);
    	picoev_add(loop, client_fd, PICOEV_READ, TIMEOUT_SECS, rw_callback, (void *)client);
  	}
}

int main(void)
{
  picoev_loop* loop;
  int listen_sock, flag;
  /* listen to port */
  assert((listen_sock = socket(AF_INET, SOCK_STREAM, 0)) != -1);
  flag = 1;
  assert(setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag))
	 == 0);
  struct sockaddr_in listen_addr;
  listen_addr.sin_family = AF_INET;
  listen_addr.sin_port = htons(PORT);
  listen_addr.sin_addr.s_addr = htonl(HOST);
  assert(bind(listen_sock, (struct sockaddr*)&listen_addr, sizeof(listen_addr))
	 == 0);
  assert(listen(listen_sock, BACKLOG_SIZE) == 0);
  setup_sock(listen_sock);
  
  /* init picoev */
  picoev_init(MAX_FDS);
  /* create loop */
  loop = picoev_create_loop(60);
  /* add listen socket */
  picoev_add(loop, listen_sock, PICOEV_READ, 0, accept_callback, NULL);
  /* loop */
  while (1) {
    picoev_loop_once(loop, 10);
  }
  /* cleanup */
  picoev_destroy_loop(loop);
  picoev_deinit();
  
  return 0;
}
