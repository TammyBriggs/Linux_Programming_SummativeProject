#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h> // Required for I/O multiplexing
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
    
    // Using fgets instead of scanf to prevent buffer overflow from keyboard
    char input_buf[64];
    if (fgets(input_buf, sizeof(input_buf), stdin) != NULL) {
        sscanf(input_buf, "%49s", library_id);
    }

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
        // 1. Request Book List
        pkt.type = MSG_BOOK_LIST_REQ;
        send(sock, &pkt, sizeof(Packet), 0);
        
        if (recv(sock, &response, sizeof(Packet), 0) <= 0) break;
        if (response.type == MSG_SERVER_SHUTDOWN) {
            printf("\n!!! %s !!!\n", response.payload);
            break;
        }

        printf("\n%s\n", response.payload);
        printf("Enter the Book ID you wish to reserve (or '0' to exit): ");
        fflush(stdout); // Force the prompt to print before select() blocks

        // --- ASYNCHRONOUS I/O SETUP ---
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds); // Monitor the keyboard
        FD_SET(sock, &read_fds);         // Monitor the network socket

        int max_sd = (sock > STDIN_FILENO) ? sock : STDIN_FILENO;

        // select() pauses the program until EITHER the user types, OR the server sends a message
        int activity = select(max_sd + 1, &read_fds, NULL, NULL, NULL);

        if (activity < 0) {
            printf("\nSelect error.\n");
            break;
        }

        // EVENT A: The server sent a message (Shutdown) or closed the connection (Timeout)
        if (FD_ISSET(sock, &read_fds)) {
            int valread = recv(sock, &response, sizeof(Packet), 0);
            if (valread <= 0) {
                printf("\n\n[Session Terminated: Server dropped the connection (Timeout)]\n");
                break;
            }
            if (response.type == MSG_SERVER_SHUTDOWN) {
                printf("\n\n!!! %s !!!\n", response.payload);
                break;
            }
        }

        // EVENT B: The user typed something on the keyboard
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            if (fgets(input_buf, sizeof(input_buf), stdin) != NULL) {
                if (sscanf(input_buf, "%d", &choice) != 1) {
                    printf("Invalid input. Try again.\n");
                    continue;
                }

                if (choice == 0) break;

                // Send Reservation Request
                pkt.type = MSG_RESERVE_REQ;
                snprintf(pkt.payload, BUFFER_SIZE, "%d", choice);
                send(sock, &pkt, sizeof(Packet), 0);
                
                if (recv(sock, &response, sizeof(Packet), 0) <= 0) {
                    printf("\nConnection lost during reservation.\n");
                    break;
                }
                if (response.type == MSG_SERVER_SHUTDOWN) {
                    printf("\n!!! %s !!!\n", response.payload);
                    break;
                }

                printf("\n>>> SERVER: %s <<<\n", response.payload);
                printf("Press Enter to continue...");
                fflush(stdout);
                fgets(input_buf, sizeof(input_buf), stdin); // Catch the Enter key
            }
        }
    }

    // --- Graceful Disconnect ---
    pkt.type = MSG_DISCONNECT;
    send(sock, &pkt, sizeof(Packet), 0);
    printf("\nSession closed. Goodbye, %s\n", library_id);
    close(sock);

    return 0;
}
