#ifndef __COMM_H__
#define __COMM_H__

#define NAMESIZE 16
#define DATASIZE 1024

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <assert.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/epoll.h>

typedef struct Msg
{
    char name[NAMESIZE];
    char data[DATASIZE];
}Msg;
#endif 
