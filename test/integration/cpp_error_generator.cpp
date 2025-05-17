// /home/john/Projects/Valgrind_Parser/test/integration/cpp_error_generator.cpp
//
// This C++ program is designed to intentionally cause various memory errors
// that Valgrind can detect. The output from Valgrind when running this
// program can then be used to test the 'vgp' (Valgrind Parser) tool.

#include <iostream>
#include <vector>
#include <cstring> // For memcpy
#include <cstdlib> // For malloc/free

void trigger_invalid_write() {
    std::cout << "Attempting invalid write..." << std::endl;
    int* array = new int[5];
    for (int i = 0; i <= 5; ++i) { // Loop goes one past the end
        array[i] = i * 10;       // Invalid write at array[5]
    }
    std::cout << "Value at intended boundary (array[4]): " << array[4] << std::endl;
    // The previous write might have corrupted memory.
    delete[] array;
}

void trigger_invalid_read() {
    std::cout << "Attempting invalid read..." << std::endl;
    int* array = new int[3];
    array[0] = 100;
    array[1] = 200;
    array[2] = 300;
    // Invalid read from array[3]
    std::cout << "Value read out of bounds (array[3]): " << array[3] << std::endl;
    delete[] array;
}

void trigger_uninitialized_value_use() {
    std::cout << "Attempting use of uninitialized value..." << std::endl;
    int uninitialized_var;
    // Using uninitialized_var in a condition
    if (uninitialized_var > 0) {
        std::cout << "Uninitialized variable was positive." << std::endl;
    } else {
        std::cout << "Uninitialized variable was not positive (or zero/negative)." << std::endl;
    }

    int* ptr_to_uninit = new int; // Allocated but not initialized
    if (*ptr_to_uninit == 5) {    // Using uninitialized heap value
        std::cout << "Uninitialized heap value was 5." << std::endl;
    }
    delete ptr_to_uninit;
}

void trigger_memory_leak_definitely_lost() {
    std::cout << "Allocating memory that will be definitely lost..." << std::endl;
    int* leaky_array = new int[100]; // Allocated with new[]
    leaky_array[0] = 777;
    std::cout << "Value in leaky_array[0]: " << leaky_array[0] << std::endl;
    // leaky_array is not deleted, causing a "definitely lost" leak.
}

void trigger_mismatched_free_delete() {
    std::cout << "Attempting mismatched free/delete..." << std::endl;

    // Case 1: new with free
    int* new_ptr_for_free = new int;
    *new_ptr_for_free = 10;
    std::cout << "Value in new_ptr_for_free: " << *new_ptr_for_free << std::endl;
    // free(new_ptr_for_free); // Mismatch: Should use delete
    // Valgrind will complain. To let the program continue for other tests,
    // we'll correctly delete it but the commented line shows the error.
    // For actual testing, uncomment free() and comment delete.
    // For now, let's cause it:
    if (std::getenv("CAUSE_MISMATCH_NEW_FREE")) { // Control via environment variable
        free(new_ptr_for_free);
    } else {
        delete new_ptr_for_free;
    }


    // Case 2: malloc with delete
    int* malloc_ptr_for_delete = (int*)malloc(sizeof(int));
    if (malloc_ptr_for_delete) {
        *malloc_ptr_for_delete = 20;
        std::cout << "Value in malloc_ptr_for_delete: " << *malloc_ptr_for_delete << std::endl;
        // delete malloc_ptr_for_delete; // Mismatch: Should use free()
        // For actual testing, uncomment delete and comment free().
        if (std::getenv("CAUSE_MISMATCH_MALLOC_DELETE")) {
            delete malloc_ptr_for_delete;
        } else {
            free(malloc_ptr_for_delete);
        }
    }

    // Case 3: new[] with delete (scalar delete)
    int* new_array_for_scalar_delete = new int[10];
    new_array_for_scalar_delete[0] = 30;
    std::cout << "Value in new_array_for_scalar_delete[0]: " << new_array_for_scalar_delete[0] << std::endl;
    // delete new_array_for_scalar_delete; // Mismatch: Should use delete[]
    // For actual testing, uncomment delete and comment delete[].
    if (std::getenv("CAUSE_MISMATCH_NEWARRAY_DELETE")) {
        delete new_array_for_scalar_delete;
    } else {
        delete[] new_array_for_scalar_delete;
    }
}

void trigger_invalid_free_delete() {
    std::cout << "Attempting invalid free/delete (double free)..." << std::endl;
    int* ptr = new int;
    *ptr = 50;
    delete ptr;
    // delete ptr; // Invalid: Double delete
    // For actual testing, uncomment the second delete.
    // To control this for testing without crashing immediately:
    if (std::getenv("CAUSE_DOUBLE_DELETE")) {
        int* ptr_to_double_delete = new int;
        *ptr_to_double_delete = 55;
        delete ptr_to_double_delete;
        delete ptr_to_double_delete; // This will cause the error
    }
}


int main() {
    std::cout << "C++ Error Generator: Starting..." << std::endl;
    std::cout << "------------------------------------" << std::endl;
    trigger_invalid_write();
    std::cout << "------------------------------------" << std::endl;
    trigger_invalid_read();
    std::cout << "------------------------------------" << std::endl;
    trigger_uninitialized_value_use();
    std::cout << "------------------------------------" << std::endl;
    trigger_memory_leak_definitely_lost();
    std::cout << "------------------------------------" << std::endl;
    trigger_mismatched_free_delete();
    std::cout << "------------------------------------" << std::endl;
    trigger_invalid_free_delete();
    std::cout << "------------------------------------" << std::endl;
    std::cout << "C++ Error Generator: Finishing." << std::endl;
    return 0;
}