#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "protocol.h"

// --- Logging System (Enhancement) ---
#define LOG_INFO(msg, ...) printf("[INFO] " msg "\n", ##__VA_ARGS__)
#define LOG_WARN(msg, ...) printf("[WARN] " msg "\n", ##__VA_ARGS__)
#define LOG_ERR(msg, ...)  fprintf(stderr, "[ERROR] " msg "\n", ##__VA_ARGS__)

// --- System Data Structures ---
typedef struct {
    int id;
    char title[100];
    int is_reserved;
} Book;

Book library_books[] = {
    {1, "The C Programming Language", 0},
    {2, "Advanced Programming in the UNIX Environment", 0},
    {3, "Operating System Concepts", 0},
    {4, "Computer Networking: A Top-Down Approach", 0},
    {5, "Design Data-Intensive Applications", 0}
};
#define NUM_BOOKS (sizeof(library_books) / sizeof(Book))

const char* valid_users[] = {"LIB001", "LIB002", "LIB003", "LIB004", "LIB005"};
#define NUM_USERS (sizeof(valid_users) / sizeof(char*))

char active_users[MAX_CLIENTS][50];
int active_count = 0;

// --- Synchronization Primitives ---
pthread_mutex_t books_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t users_mutex = PTHREAD_MUTEX_INITIALIZER;

// --- Helper Functions ---
int authenticate_user(const char* lib_id) {
    for (int i = 0; i < NUM_USERS; i++) {
        if (strcmp(valid_users[i], lib_id) == 0) return 1;
    }
    return 0;
}

void print_server_status() {
    printf("\n--- SERVER STATUS ---\n");
    printf("Active Users (%d/%d): ", active_count, MAX_CLIENTS);
    for(int i=0; i<MAX_CLIENTS; i++) {
        if(strlen(active_users[i]) > 0) printf("%s ", active_users[i]);
    }
    printf("\nBook Status:\n");
    for(int i=0; i<NUM_BOOKS; i++) {
        printf("  [%d] %s - %s\n", library_books[i].id, library_books[i].title, 
               library_books[i].is_reserved ? "RESERVED" : "AVAILABLE");
    }
    printf("---------------------\n\n");
}

// --- Thread Handler for Concurrent Clients ---
void* handle_client(void* arg) {
    int client_socket = *((int*)arg);
    free(arg); // Free allocated memory for socket descriptor
    
    Packet pkt;
    char current_user[50] = "";
    int is_authenticated = 0;
    int user_slot = -1;

    while (recv(client_socket, &pkt, sizeof(Packet), 0) > 0) {
        Packet response;
        memset(&response, 0, sizeof(Packet));

        if (pkt.type == MSG_AUTH_REQ) {
            if (authenticate_user(pkt.payload)) {
                is_authenticated = 1;
                strncpy(current_user, pkt.payload, sizeof(current_user)-1);
                
                // Track active user safely
                pthread_mutex_lock(&users_mutex);
                for(int i=0; i<MAX_CLIENTS; i++) {
                    if(strlen(active_users[i]) == 0) {
                        strncpy(active_users[i], current_user, sizeof(active_users[i])-1);
                        user_slot = i;
                        active_count++;
                        break;
                    }
                }
                pthread_mutex_unlock(&users_mutex);

                response.type = MSG_AUTH_SUCCESS;
                strncpy(response.payload, "Authentication successful.", BUFFER_SIZE-1);
                LOG_INFO("User Authenticated: %s", current_user);
                print_server_status();
            } else {
                response.type = MSG_AUTH_FAIL;
                strncpy(response.payload, "Invalid Library ID.", BUFFER_SIZE-1);
                LOG_WARN("Failed auth attempt with ID: %s", pkt.payload);
            }
            send(client_socket, &response, sizeof(Packet), 0);
        }
        else if (is_authenticated) {
            if (pkt.type == MSG_BOOK_LIST_REQ) {
                response.type = MSG_BOOK_LIST_RES;
                char list_buffer[BUFFER_SIZE] = "Available Books:\n";
                
                pthread_mutex_lock(&books_mutex);
                for (int i = 0; i < NUM_BOOKS; i++) {
                    char line[120];
                    snprintf(line, sizeof(line), "[%d] %s - %s\n", 
                             library_books[i].id, library_books[i].title,
                             library_books[i].is_reserved ? "RESERVED" : "AVAILABLE");
                    strncat(list_buffer, line, BUFFER_SIZE - strlen(list_buffer) - 1);
                }
                pthread_mutex_unlock(&books_mutex);
                
                strncpy(response.payload, list_buffer, BUFFER_SIZE-1);
                send(client_socket, &response, sizeof(Packet), 0);
            }
            else if (pkt.type == MSG_RESERVE_REQ) {
                int requested_id = atoi(pkt.payload);
                int found = 0;

                // Sync protection to prevent race conditions (double booking)
                pthread_mutex_lock(&books_mutex);
                for (int i = 0; i < NUM_BOOKS; i++) {
                    if (library_books[i].id == requested_id) {
                        found = 1;
                        if (library_books[i].is_reserved) {
                            response.type = MSG_RESERVE_FAIL;
                            strncpy(response.payload, "Already reserved by another user.", BUFFER_SIZE-1);
                        } else {
                            library_books[i].is_reserved = 1;
                            response.type = MSG_RESERVE_SUCCESS;
                            strncpy(response.payload, "Reserved successfully.", BUFFER_SIZE-1);
                            LOG_INFO("User %s reserved Book ID %d", current_user, requested_id);
                        }
                        break;
                    }
                }
                pthread_mutex_unlock(&books_mutex);

                if (!found) {
                    response.type = MSG_RESERVE_FAIL;
                    strncpy(response.payload, "Invalid Book ID.", BUFFER_SIZE-1);
                }
                
                send(client_socket, &response, sizeof(Packet), 0);
                print_server_status();
            }
            else if (pkt.type == MSG_DISCONNECT) {
                break;
            }
        }
    }

    // --- Session Lifecycle Handling (Graceful Disconnect) ---
    if (is_authenticated && user_slot != -1) {
        pthread_mutex_lock(&users_mutex);
        memset(active_users[user_slot], 0, sizeof(active_users[user_slot]));
        active_count--;
        pthread_mutex_unlock(&users_mutex);
        LOG_INFO("User Disconnected: %s", current_user);
        print_server_status();
    }
    close(client_socket);
    return NULL;
}

int main() {
    int server_fd, *new_sock;
    struct sockaddr_in address;
    int opt = 1;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        LOG_ERR("Socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        LOG_ERR("Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        LOG_ERR("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Limit to 5 concurrent clients
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        LOG_ERR("Listen failed");
        exit(EXIT_FAILURE);
    }

    LOG_INFO("Library Server running on port %d...", PORT);
    memset(active_users, 0, sizeof(active_users)); // Initialize active users array

    while (1) {
        int client_socket;
        struct sockaddr_in client_addr;
        socklen_t addr_size = sizeof(struct sockaddr_in);

        client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &addr_size);
        if (client_socket < 0) continue;

        LOG_INFO("New connection established. Assigning thread...");

        // Allocate memory for socket to pass safely to thread
        new_sock = malloc(1 * sizeof(int));
        *new_sock = client_socket;

        pthread_t client_thread;
        if (pthread_create(&client_thread, NULL, handle_client, (void*)new_sock) < 0) {
            LOG_ERR("Could not create thread");
            free(new_sock);
        }
        pthread_detach(client_thread); // Detach so resources are freed upon thread exit
    }
    return 0;
}
