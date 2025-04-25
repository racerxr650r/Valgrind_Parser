// filepath: tests/test_vgp.cpp
#include "CppUTest/TestHarness.h"
#include "CppUTest/CommandLineTestRunner.h"
extern "C" {
    #include "vgp.h"
}

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

int main(int ac, char** av)
{
    // Run all tests discovered in this file and linked files
    return CommandLineTestRunner::RunAllTests(ac, av);
}