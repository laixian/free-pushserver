#include<stdio.h>
#include<stdlib.h>
#include "request.h"
#include<string.h>
//#include "zmalloc.h"

header *new_header(void){
	header *hd = NULL;
	hd = (header *)malloc(sizeof(header));
	memset(hd, 0, sizeof(header));
	return hd;
}

void free_header(header *hd){
	free(hd);
}

request *new_req(void){
	request *req = NULL;
	req = (request *)malloc(sizeof(request));
	memset(req, 0, sizeof(request));
	return req;
}

void free_req(request *req){
	int i;
	header *tmp_hd;
	for(i = 0; i < req->header_count; i++)
	{
		tmp_hd = req->headers[i];
		free_header(tmp_hd);
	}
	free(req);
}

req_queue *new_req_queue(void){
	req_queue *q = NULL;
	q = (req_queue *)malloc(sizeof(req_queue));
	memset(q, 0, sizeof(req_queue));
	return q;
}

void free_req_queue(req_queue *q){
	request *req , *temp_req;
	req = q->head;
	while(req){
		temp_req = req;
		req = (request *)temp_req->next;
		free_req(temp_req);
	}
	free(q);
}

