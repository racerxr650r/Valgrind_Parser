// filepath: tests/test_vgp.cpp
#include "CppUTest/TestHarness.h"
#include "CppUTest/CommandLineTestRunner.h"
extern "C" {
    #include "vgp.h"
}

// Main function to run all tests
int main(int ac, char** av)
{
    // Run all tests discovered in this file and linked files
    return CommandLineTestRunner::RunAllTests(ac, av);
}

// --- Test Cases for strip_valgrind_pid_prefix ---
// Test group for strip_valgrind_pid_prefix
TEST_GROUP(StripValgrindPidPrefix)
{
    // Setup/teardown can go here if needed
};

// Test case for LLR 2.1 and LLR 2.2: Standard prefix is found and removed
TEST(StripValgrindPidPrefix, RemovesStandardPrefix)
{
    // LLR 2.1: Check if the input line starts with the Valgrind PID pattern `==<digits>== `.
    // LLR 2.2: If the pattern is found, return a pointer to the character immediately following the pattern.
    char line[] = "==12345== Invalid read of size 4";
    char *expected = line + 10; // Pointer to 'I' in "Invalid"
    char *actual = strip_valgrind_pid_prefix(line);
    POINTERS_EQUAL(expected, actual);
    STRCMP_EQUAL("Invalid read of size 4", actual); // Also check content for clarity
}

// Test case for LLR 2.3: No prefix found, original pointer returned
TEST(StripValgrindPidPrefix, HandlesNoPrefix)
{
    // LLR 2.3: If the pattern is not found, return the original line pointer.
    char line[] = "   at 0x1234: main (main.c:5)";
    char *expected = line; // Should return the original pointer
    char *actual = strip_valgrind_pid_prefix(line);
    POINTERS_EQUAL(expected, actual);
    STRCMP_EQUAL("   at 0x1234: main (main.c:5)", actual);
}

// Test case for LLR 2.3: Edge case - Empty string
TEST(StripValgrindPidPrefix, HandlesEmptyString)
{
    // LLR 2.3: If the pattern is not found, return the original line pointer.
    char line[] = "";
    char *expected = line;
    char *actual = strip_valgrind_pid_prefix(line);
    POINTERS_EQUAL(expected, actual);
    STRCMP_EQUAL("", actual);
}

// Test case for LLR 2.3: Edge case - Starts with == but no digits
TEST(StripValgrindPidPrefix, HandlesDoubleEqualsNoDigits)
{
    // LLR 2.3: If the pattern is not found, return the original line pointer.
    char line[] = "== == Invalid read";
    char *expected = line;
    char *actual = strip_valgrind_pid_prefix(line);
    POINTERS_EQUAL(expected, actual);
    STRCMP_EQUAL("== == Invalid read", actual);
}

// Test case for LLR 2.3: Edge case - Starts with ==digits but no closing ==
TEST(StripValgrindPidPrefix, HandlesMissingClosingEquals)
{
    // LLR 2.3: If the pattern is not found, return the original line pointer.
    char line[] = "==123 Invalid read";
    char *expected = line;
    char *actual = strip_valgrind_pid_prefix(line);
    POINTERS_EQUAL(expected, actual);
    STRCMP_EQUAL("==123 Invalid read", actual);
}

// Test case for LLR 2.3: Edge case - Correct format but no space after ==
TEST(StripValgrindPidPrefix, HandlesMissingSpaceAfterPrefix)
{
    // LLR 2.3: If the pattern is not found, return the original line pointer.
    char line[] = "==123==Invalid read";
    char *expected = line;
    char *actual = strip_valgrind_pid_prefix(line);
    POINTERS_EQUAL(expected, actual);
    STRCMP_EQUAL("==123==Invalid read", actual);
}

// Test case for LLR 2.1/2.2: Different PID length
TEST(StripValgrindPidPrefix, HandlesDifferentPidLength)
{
    // LLR 2.1 / LLR 2.2: Check pattern and return pointer after pattern.
    char line[] = "==9== Another message";
    char *expected = line + 6; // Pointer to 'A'
    char *actual = strip_valgrind_pid_prefix(line);
    POINTERS_EQUAL(expected, actual);
    STRCMP_EQUAL("Another message", actual);
}

// --- Test Cases for is_user_code_stack_trace ---
// Test group for is_user_code_stack_trace
TEST_GROUP(IsUserCodeStackTrace)
{
    // Setup/teardown can go here if needed
};

// Test case for LLR 3.1, LLR 3.2, LLR 3.3: Valid user code line (.c)
TEST(IsUserCodeStackTrace, DetectsValidUserCodeC)
{
    // LLR 3.1: Check for user code extensions (finds ".c").
    // LLR 3.2: Check for ignored paths (finds none).
    // LLR 3.3: Return true as extension found AND no ignored path found.
    const char *line = "   at 0x4848999: my_func (in /home/user/proj/src/my_file.c:123)";
    CHECK_TRUE(is_user_code_stack_trace(line));
}

// Test case for LLR 3.1, LLR 3.2, LLR 3.3: Valid user code line (.cpp with 'by')
TEST(IsUserCodeStackTrace, DetectsValidUserCodeCppWithBy)
{
    // LLR 3.1: Check for user code extensions (finds ".cpp").
    // LLR 3.2: Check for ignored paths (finds none).
    // LLR 3.3: Return true as extension found AND no ignored path found.
    const char *line = "   by 0x5050ABC: SomeClass::method(int) (app/main.cpp:45)";
    CHECK_TRUE(is_user_code_stack_trace(line));
}

// Test case for LLR 3.4: Not a stack trace line (missing prefix)
TEST(IsUserCodeStackTrace, RejectsLineWithoutPrefix)
{
    // LLR 3.4: Return false otherwise (doesn't start with "   at " or "   by ").
    const char *line = "Invalid read of size 4";
    CHECK_FALSE(is_user_code_stack_trace(line));
}

// Test case for LLR 3.1, LLR 3.4: Stack trace line but no user extension
TEST(IsUserCodeStackTrace, RejectsLineWithoutUserExtension)
{
    // LLR 3.1: Check for user code extensions (finds none).
    // LLR 3.4: Return false otherwise (no user extension found).
    const char *line = "   at 0x40157F0: function (in /no/extension/file)";
    CHECK_FALSE(is_user_code_stack_trace(line));
}

// Test case for LLR 3.1, LLR 3.2, LLR 3.4: User extension present but path is ignored (/usr/)
TEST(IsUserCodeStackTrace, RejectsLineWithIgnoredPathUsr)
{
    // LLR 3.1: Check for user code extensions (finds ".h").
    // LLR 3.2: Check for ignored paths (finds "/usr/").
    // LLR 3.4: Return false otherwise (ignored path found).
    const char *line = "   by 0x6070DEF: __libc_start_main (in /usr/include/bits/libc-start.h:55)";
    CHECK_FALSE(is_user_code_stack_trace(line));
}

// Test case for LLR 3.1, LLR 3.2, LLR 3.4: User extension present but path is ignored (/lib/)
TEST(IsUserCodeStackTrace, RejectsLineWithIgnoredPathLib)
{
    // LLR 3.1: Check for user code extensions (finds ".c").
    // LLR 3.2: Check for ignored paths (finds "/lib/").
    // LLR 3.4: Return false otherwise (ignored path found).
    const char *line = "   at 0x7080FGH: another_func (/lib/some_library.c:99)";
    CHECK_FALSE(is_user_code_stack_trace(line));
}

// Test case for LLR 3.1, LLR 3.2, LLR 3.4: User extension present but path is ignored (vg_)
TEST(IsUserCodeStackTrace, RejectsLineWithIgnoredPathVg)
{
    // LLR 3.1: Check for user code extensions (finds ".c").
    // LLR 3.2: Check for ignored paths (finds "vg_").
    // LLR 3.4: Return false otherwise (ignored path found).
    const char *line = "   at 0x8090IJK: vg_replace_malloc (vg_replace_malloc.c:342)";
    CHECK_FALSE(is_user_code_stack_trace(line));
}

// Test case for LLR 3.1, LLR 3.2, LLR 3.3: Edge case - extension at end
TEST(IsUserCodeStackTrace, DetectsExtensionAtEnd)
{
    // LLR 3.1 / LLR 3.2 / LLR 3.3
    const char *line = "   at 0x1111111: func (myfile.c)";
    CHECK_TRUE(is_user_code_stack_trace(line));
}

// Test case for LLR 3.1, LLR 3.2, LLR 3.4: Edge case - ignore path at start
TEST(IsUserCodeStackTrace, RejectsIgnorePathAtStart)
{
    // LLR 3.1 / LLR 3.2 / LLR 3.4
    const char *line = "   at 0x2222222: func (/usr/local/myfile.c:10)";
    CHECK_FALSE(is_user_code_stack_trace(line));
}
