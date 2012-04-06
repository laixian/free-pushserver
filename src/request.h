/* request.h
 *
 * */
#include <inttypes.h>

#define MAX_HEADERS 128

typedef struct{
	char *key;
	char *value;
} header;

typedef struct{
	char *path;
	char *uri;
	char *query_string;
	char *fragment;
	header *headers[MAX_HEADERS];
	int header_count;
	void *next;
	void *body;
	int body_length;
	int body_readed;
} request;

typedef struct{
	request *head;
	request *tail;
	int size;
} req_queue;

header *new_header(void);

void free_header(header *hd);

request *new_req(void);

void free_req(request *req);

req_queue *new_req_queue(void);

void free_req_queue(req_queue *q);

void push_req(req_queue *q, request *req);

request *pop_req(req_queue *q);

