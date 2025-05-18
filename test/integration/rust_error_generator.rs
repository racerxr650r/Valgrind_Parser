// /home/john/Projects/Valgrind_Parser/test/integration/rust_error_generator.rs
//
// This Rust program is designed to intentionally cause various memory errors
// that Valgrind can detect. The output from Valgrind when running this
// program can then be used to test the 'vgp' (Valgrind Parser) tool.

use std::alloc::{alloc, dealloc, Layout};
use std::mem;
use std::ptr;

fn trigger_invalid_write() {
    println!("Attempting invalid write...");
    unsafe {
        let layout = Layout::array::<i32>(5).unwrap();
        let ptr = alloc(layout) as *mut i32;

        for i in 0..=5 { // Loop goes one past the end (0 to 5 is 6 elements)
            // Invalid write at index 5
            ptr.add(i).write( (i as i32) * 10);
        }
        println!("Value at intended boundary (index 4): {}", *ptr.add(4));
        // The previous write might have corrupted memory.
        dealloc(ptr as *mut u8, layout);
    }
}

fn trigger_invalid_read() {
    println!("Attempting invalid read...");
    unsafe {
        let layout = Layout::array::<i32>(3).unwrap();
        let ptr = alloc(layout) as *mut i32;

        for i in 0..3 {
            ptr.add(i).write((i as i32 + 1) * 100);
        }

        // Invalid read from index 3
        println!("Value read out of bounds (index 3): {}", *ptr.add(3));
        dealloc(ptr as *mut u8, layout);
    }
}

fn trigger_uninitialized_value_use() {
    println!("Attempting use of uninitialized value...");
    unsafe {
        // Stack uninitialized (Rust usually prevents this for safe types,
        // but with MaybeUninit or raw pointers we can achieve it)
        let uninitialized_stack_var: i32 = mem::MaybeUninit::uninit().assume_init();
        if uninitialized_stack_var > 0 {
            println!("Uninitialized stack variable was positive.");
        } else {
            println!("Uninitialized stack variable was not positive (or zero/negative).");
        }

        // Heap uninitialized
        let layout = Layout::new::<i32>();
        let ptr_to_uninit = alloc(layout) as *mut i32;
        // We don't write to *ptr_to_uninit
        if *ptr_to_uninit == 5 { // Using uninitialized heap value
            println!("Uninitialized heap value was 5.");
        }
        dealloc(ptr_to_uninit as *mut u8, layout);
    }
}

fn trigger_memory_leak_definitely_lost() {
    println!("Allocating memory that will be definitely lost...");
    unsafe {
        let layout = Layout::array::<i32>(100).unwrap();
        let leaky_ptr = alloc(layout) as *mut i32;
        leaky_ptr.write(777); // Write something to it
        println!("Value in leaky_ptr[0]: {}", *leaky_ptr);
        // We don't call dealloc(leaky_ptr as *mut u8, layout);
        // This will cause a "definitely lost" leak.
        // To avoid the compiler complaining about an unused variable that's not dropped:
        mem::forget(leaky_ptr); // Tell Rust not to worry about dropping this raw pointer
    }
}

fn trigger_mismatched_deallocation() {
    println!("Attempting mismatched deallocation (less direct in pure Rust)...");
    // Rust's standard allocators (global allocator) are usually consistent.
    // A true new/free or malloc/delete mismatch is harder to create without FFI
    // or custom allocators that behave differently.
    // However, we can simulate a scenario that Valgrind might flag if it were C/C++.
    // For instance, trying to deallocate with a different layout than allocation.
    // This might not always trigger the *exact* "mismatched free" Valgrind message
    // but can lead to heap corruption.

    // For a more direct C-style mismatch, you'd typically use FFI to call C's malloc/free.
    // Let's try a double free, which is a common related issue.
    println!("Simulating a double free scenario...");
    unsafe {
        let layout = Layout::new::<i32>();
        let ptr = alloc(layout);
        if !ptr.is_null() {
            *(ptr as *mut i32) = 50;
            dealloc(ptr, layout);
            // Attempting to deallocate again
            // To control this for testing without crashing immediately:
            if std::env::var("CAUSE_DOUBLE_FREE_RUST").is_ok() {
                 println!("Actually causing double free now...");
                 dealloc(ptr, layout); // This will cause the error
            } else {
                 println!("Skipping actual double free for program stability.");
            }
        }
    }
}

fn trigger_invalid_free() {
    println!("Attempting invalid free (e.g., freeing non-heap or middle of block)...");
    unsafe {
        // Freeing a pointer to the middle of an allocation
        let layout = Layout::array::<i32>(10).unwrap();
        let ptr_base = alloc(layout) as *mut i32;
        if !ptr_base.is_null() {
            let ptr_middle = ptr_base.add(5); // Pointer to the middle

            if std::env::var("CAUSE_INVALID_FREE_RUST").is_ok() {
                println!("Actually causing invalid free (middle of block)...");
                // Deallocating from ptr_middle is incorrect. The original layout is for ptr_base.
                // This is a tricky one for Valgrind to always report as "Invalid free"
                // in Rust without more direct heap manipulation like in C.
                // It might report heap corruption or an invalid read/write later.
                // For a more direct "Invalid free" Valgrind often needs to see `free()`
                // on a non-heap address or an already freed address.
                dealloc(ptr_middle as *mut u8, layout); // This is problematic
            } else {
                println!("Skipping actual invalid free for program stability. Correctly deallocating base.");
                dealloc(ptr_base as *mut u8, layout);
            }
        }
    }
}

fn main() {
    println!("Rust Error Generator: Starting...");
    println!("------------------------------------");
    trigger_invalid_write();
    println!("------------------------------------");
    trigger_invalid_read();
    println!("------------------------------------");
    trigger_uninitialized_value_use();
    println!("------------------------------------");
    trigger_memory_leak_definitely_lost();
    println!("------------------------------------");
    trigger_mismatched_deallocation(); // Focuses on double free
    println!("------------------------------------");
    trigger_invalid_free();
    println!("------------------------------------");
    println!("Rust Error Generator: Finishing.");
}