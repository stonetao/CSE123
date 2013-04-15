#ifndef TFTP_H
#define TFTP_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

typedef int bool;
#define true 1
#define false 0

#define SERV_UDP_PORT   60010
//
#define SERV_HOST_ADDR  "127.0.0.1"
//
#define MAX_TFTP_CLIENTS 100
#define MAX_TFTP_TIMEOUTS 10
#define TIMEOUT_DURATION 5 //seconds
//
//
////max packet size
#define MAX_BUFF_SIZE 2048
//
////for testing
#define DEBUG true
#define MAX_STRING_SIZE 256
#define MAX_MODE_SIZE 8
#define MAX_DATA_SIZE 512

//optcodes
#define OPTCODE_RRQ  1
#define OPTCODE_WRQ  2
#define OPTCODE_DATA 3
#define OPTCODE_ACK  4
#define OPTCODE_ERR  5
//
////Error Codes
#define ERRCODE_UNDEFINED           0
#define ERRCODE_FILE_NOT_FOUND      1
#define ERRCODE_ACCESS_VIOLATION    2
#define ERRCODE_DISK_FULL           3
#define ERRCODE_ILLEGAL_OPERATION   4
#define ERRCODE_UNKNOWN_TRANSFER_ID 5
#define ERRCODE_FILE_ALREADY_EXISTS 6
#define ERRCODE_NO_SUCH_USER        7
//

typedef struct {
    unsigned short optcode;
    char FileName[MAX_STRING_SIZE+1] = "tesgdfghds";
    char Mode[MAX_MODE_SIZE+1] = "octet";
} RRQ, WRQ;

typedef struct {
    unsigned short optcode = 3;
    unsigned short No_Block;
    char data[MAX_DATA_SIZE];
} DATA;

typedef struct {
    unsigned short optcode = 4;
    unsigned short No_Block;
} ACK;

typedef struct {
    unsigned short optcode = 5;
    unsigned short ErrorCode;
    char ErrMsg[MAX_STRING_SIZE];
} ERROR;

#endif


