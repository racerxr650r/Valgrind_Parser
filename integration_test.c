#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- Helper Structs for Leaks ---
typedef struct Node {
    int data;
    struct Node* next;
} Node;

// --- Functions to Generate Valgrind Issues ---

// 1. Function that returns a pointer (definite leak)
char* allocate_for_definite_leak() {
    char* ptr = (char*)malloc(50 * sizeof(char));
    if (ptr) {
        // Intentionally using strcpy for a simple, common operation.
        // Consider snprintf for production code to prevent buffer overflows if source is variable.
        strcpy(ptr, "This memory is part of a definite leak.");
        printf("Allocated for definite leak: %s\n", ptr);
    }
    return ptr; // Caller (main) will not free this
}

// Function for an indirect leak
Node* create_indirect_leak_callee() {
    Node* child = (Node*)malloc(sizeof(Node));
    if (child) {
        child->data = 200;
        child->next = NULL;
        printf("Allocated child node for indirect leak.\n");
    }
    return child;
}

Node* create_indirect_leak_caller() {
    Node* parent = (Node*)malloc(sizeof(Node));
    if (parent) {
        parent->data = 100;
        parent->next = create_indirect_leak_callee(); // Child is indirectly leaked if parent is leaked
        printf("Allocated parent node for indirect leak.\n");
    }
    return parent; // Caller (main) will not free this, leading to parent (definite) and child (indirect)
}

// Global pointer for still reachable leak
char *g_still_reachable_ptr = NULL;

void setup_global_still_reachable() {
    g_still_reachable_ptr = (char*)malloc(30 * sizeof(char));
    if (g_still_reachable_ptr) {
        strcpy(g_still_reachable_ptr, "Global still reachable data");
        printf("Setup global still reachable pointer.\n");
    }
}

// 2. Invalid Write (Heap)
void cause_invalid_write_heap() {
    char* buffer = (char*)malloc(10 * sizeof(char));
    if (!buffer) return;
    strcpy(buffer, "short"); 
    printf("About to cause an invalid heap write...\n");
    buffer[15] = 'X'; // Invalid write past allocated heap region
    printf("Invalid heap write attempted (buffer[15]=%c).\n", buffer[15]); 
    free(buffer); 
}

// 3. Invalid Read (Heap)
void cause_invalid_read_heap() {
    char* buffer = (char*)malloc(10 * sizeof(char));
    char c = ' ';
    if (!buffer) return;
    strcpy(buffer, "data");
    printf("About to cause an invalid heap read...\n");
    c = buffer[20]; // Invalid read past allocated heap region
    printf("Invalid heap read attempted (value: %c).\n", c); 
    free(buffer); 
}

// 4. Use of Uninitialized Value
int *use_uninitialized_value_conditional() 
{
    int uninitialized_var;
    int result = 5;
    printf("About to use an uninitialized value in a conditional...\n");
    if (uninitialized_var > 10)
    { // Conditional jump depends on uninitialized value
        result = 10;
    }
    printf("Result after uninitialized check: %d\n", result);

    return(&uninitialized_var);
}

// 5. Double Free
void cause_double_free() {
    char* ptr = (char*)malloc(20 * sizeof(char));
    if (!ptr) return;
    strcpy(ptr, "Double free target");
    printf("About to cause a double free...\n");
    free(ptr);
    printf("First free done.\n");
    free(ptr); // Double free
    printf("Second free attempted.\n");
}

// 6. Invalid Free (Mid-block)
void cause_invalid_free_mid_block() {
    char* block = (char*)malloc(30 * sizeof(char));
    if(!block) return;
    strcpy(block, "Test block for mid-free");
    printf("About to cause an invalid free (mid-block)...\n");
    free(block + 5); // Invalid free (address is not a block start)
    printf("Mid-block free attempted.\n");
    // The original 'block' might be leaked now or heap corrupted.
    // For testing, we'll let Valgrind report the invalid free and potential subsequent issues.
}

// 7. Source and Destination Overlap
void cause_overlap_memcpy() {
    char buffer[30];
    strcpy(buffer, "1234567890abcdefghij");
    printf("About to cause overlap with memcpy...\n");
    memcpy(buffer + 5, buffer, 10); // Overlap: dest (buffer+5) is within src (buffer)
    printf("Buffer after overlap: %s\n", buffer);
}


// --- Main Test Driver ---
int main(int argc, char *argv[]) {
    printf("Valgrind Test Input Program Starting...\n");
    printf("This program generates various errors and leaks for vgp to parse.\n");
    printf("NOTE: Some errors may terminate the program early.\n");
    printf("Valgrind should report errors that occurred before termination.\n\n");

    // --- Test Execution ---

    // Leaks (generally non-fatal for program execution)
    printf("--- Triggering Definite Leak ---\n");
    char* definite_leak_ptr = allocate_for_definite_leak();
    if (definite_leak_ptr) {
        printf("Content of definitely leaked memory: %s\n", definite_leak_ptr);
    }

    printf("\n--- Triggering Indirect Leak ---\n");
    Node* indirect_leak_root_ptr = create_indirect_leak_caller();
    if (indirect_leak_root_ptr && indirect_leak_root_ptr->next) {
        printf("Indirect leak: parent data %d, child data %d\n",
               indirect_leak_root_ptr->data, indirect_leak_root_ptr->next->data);
    }

    printf("\n--- Triggering Still Reachable Leak (Global) ---\n");
    setup_global_still_reachable();

    // Potentially non-fatal errors
    printf("\n--- Triggering Use of Uninitialized Value ---\n");
    int *value = use_uninitialized_value_conditional();

    printf("\n--- Triggering Source/Destination Overlap (memcpy) ---\n");
    cause_overlap_memcpy();

    // Errors more likely to be fatal or destabilizing
    // Order can matter if one crash prevents others.
    // Valgrind often reports an error and continues if possible.

    printf("\n--- Triggering Invalid Write (Heap) ---\n");
    cause_invalid_write_heap(); 

    printf("\n--- Triggering Invalid Read (Heap) ---\n");
    cause_invalid_read_heap(); 

    // Errors highly likely to be fatal
    printf("\n--- Triggering Double Free ---\n");
    cause_double_free();

    // This might not run if double_free crashes hard, or heap is too corrupted.
    printf("\n--- Triggering Invalid Free (Mid-block) ---\n");
    cause_invalid_free_mid_block();


    printf("\nValgrind Test Input Program Finishing (if not crashed).\n");
    // Leaks to be reported:
    // - definite_leak_ptr: definitely lost
    // - indirect_leak_root_ptr: definitely lost (parent), indirect_leak_root_ptr->next: indirectly lost (child)
    // - g_still_reachable_ptr: still reachable
    return 0;
}