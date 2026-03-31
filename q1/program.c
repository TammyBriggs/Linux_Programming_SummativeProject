#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global variable requirement
int global_counter = 0;

// Function 1: Dynamic memory allocation and write operation
char* allocate_and_init(int size) {
    char *buffer = (char *)malloc(size * sizeof(char));
    if (buffer != NULL) {
        // Write operation to allocated memory
        strncpy(buffer, "ELF_Analysis_Test", size);
        buffer[size - 1] = '\0'; 
    }
    return buffer;
}

// Function 2: Loop requirement
void process_data(char *data) {
    int i = 0;
    // Loop to iterate through the string
    while (data[i] != '\0') {
        global_counter++;
        i++;
    }
}

// Function 3: Conditional branch and standard library print
void evaluate_and_print(int threshold) {
    // Conditional branch requirement
    if (global_counter > threshold) {
        printf("Success: Processed %d characters.\n", global_counter);
    } else {
        printf("Failure: Processed only %d characters.\n", global_counter);
    }
}

int main() {
    int buffer_size = 32;
    char *my_data = allocate_and_init(buffer_size);

    if (my_data != NULL) {
        process_data(my_data);
        evaluate_and_print(10);
        free(my_data);
    }

    return 0;
}
