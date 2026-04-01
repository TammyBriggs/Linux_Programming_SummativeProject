#ifndef PROTOCOL_H
#define PROTOCOL_H

#define PORT 8080
#define MAX_CLIENTS 5
#define BUFFER_SIZE 512

// Structured Message Protocol (Enhancement)
typedef enum {
    MSG_AUTH_REQ,
    MSG_AUTH_SUCCESS,
    MSG_AUTH_FAIL,
    MSG_BOOK_LIST_REQ,
    MSG_BOOK_LIST_RES,
    MSG_RESERVE_REQ,
    MSG_RESERVE_SUCCESS,
    MSG_RESERVE_FAIL,
    MSG_DISCONNECT
} MessageType;

// Standardized packet for all TCP communication
typedef struct {
    MessageType type;
    char payload[BUFFER_SIZE];
} Packet;

#endif
