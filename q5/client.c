#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "protocol.h"

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    Packet pkt, response;
    char library_id[50];

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Error: Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nError: Invalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nError: Connection Failed \n");
        return -1;
    }

    // --- Authentication Phase ---
    printf("Welcome to the Digital Library Platform\n");
    printf("Enter your Library ID to authenticate: ");
    scanf("%49s", library_id);

    pkt.type = MSG_AUTH_REQ;
    strncpy(pkt.payload, library_id, BUFFER_SIZE);
    send(sock, &pkt, sizeof(Packet), 0);
    
    if (recv(sock, &response, sizeof(Packet), 0) <= 0) {
        printf("\nConnection to server lost.\n");
        close(sock);
        return 0;
    }

    if (response.type == MSG_SERVER_SHUTDOWN) {
        printf("\n!!! %s !!!\n", response.payload);
        close(sock);
        return 0;
    }

    printf("\nServer Response: %s\n", response.payload);

    if (response.type != MSG_AUTH_SUCCESS) {
        printf("Closing session.\n");
        close(sock);
        return 0;
    }

    // --- Main Session Loop ---
    int choice;
    while (1) {
        // Request Book List
        pkt.type = MSG_BOOK_LIST_REQ;
        send(sock, &pkt, sizeof(Packet), 0);
        
        if (recv(sock, &response, sizeof(Packet), 0) <= 0) break;
        if (response.type == MSG_SERVER_SHUTDOWN) {
            printf("\n!!! %s !!!\n", response.payload);
            break;
        }

        printf("\n%s\n", response.payload);
        printf("Enter the Book ID you wish to reserve (or '0' to exit): ");
        
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input. Exiting.\n");
            break;
        }

        if (choice == 0) break;

        // Request Reservation
        pkt.type = MSG_RESERVE_REQ;
        snprintf(pkt.payload, BUFFER_SIZE, "%d", choice);
        send(sock, &pkt, sizeof(Packet), 0);
        
        if (recv(sock, &response, sizeof(Packet), 0) <= 0) break;
        if (response.type == MSG_SERVER_SHUTDOWN) {
            printf("\n!!! %s !!!\n", response.payload);
            break;
        }

        printf("\n>>> SERVER: %s <<<\n", response.payload);
        printf("Press Enter to continue...");
        getchar(); getchar(); // Clear buffer and wait
    }

    // --- Graceful Disconnect ---
    pkt.type = MSG_DISCONNECT;
    send(sock, &pkt, sizeof(Packet), 0);
    printf("\nSession closed. Goodbye, %s\n", library_id);
    close(sock);

    return 0;
}
