#ifndef UTILS_H
#define UTILS_H
#include <stdlib.h>
#include <sys/socket.h>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "def.h"

int readChar(int clnt_sockid, char *ch);
int getBodyLenFromHeader(char *head);
void handleReq(int clnt_sockid, char *head, char *body, int bodyLen);
int sendHttpResp(int sock, int statCode, char *resp, int len, char * content_type);
void transFileName(char *uri);
#endif