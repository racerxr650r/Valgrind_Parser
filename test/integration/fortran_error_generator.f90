! /home/john/Projects/Valgrind_Parser/test/integration/fortran_error_generator.f90
!
! This Fortran program is designed to intentionally cause memory errors
! that Valgrind can detect. The output from Valgrind when running this
! program can then be used to test the 'vgp' parser.
!
PROGRAM fortran_error_generator
    IMPLICIT NONE

    PRINT *, "Fortran Error Generator: Starting..."
    PRINT *, "------------------------------------"

    CALL trigger_invalid_write()
    PRINT *, "------------------------------------"
    CALL trigger_invalid_read()
    PRINT *, "------------------------------------"
    CALL trigger_uninitialized_use()
    PRINT *, "------------------------------------"
    CALL trigger_memory_leak()
    PRINT *, "------------------------------------"

    ! Note: small_array is deallocated within trigger_invalid_read if it was allocated.
    ! leaky_array is intentionally not deallocated.

    PRINT *, "Fortran Error Generator: Finishing."

CONTAINS

    SUBROUTINE trigger_invalid_write()
        INTEGER, ALLOCATABLE :: local_array(:)
        INTEGER :: i
        PRINT *, "Attempting invalid write..."
        ALLOCATE(local_array(1:5))
        DO i = 1, 10 ! Loop goes out of bounds
            local_array(i) = i * 10 ! This will write out of bounds for i > 5
        END DO
        PRINT *, "Value at intended boundary (local_array(5)): ", local_array(5)
        ! The previous overwrite might have corrupted memory used by local_array(5)
        IF (ALLOCATED(local_array)) DEALLOCATE(local_array)
    END SUBROUTINE trigger_invalid_write

    SUBROUTINE trigger_invalid_read()
        INTEGER, ALLOCATABLE :: local_array(:)
        PRINT *, "Attempting invalid read..."
        ALLOCATE(local_array(1:3))
        local_array(1) = 100
        local_array(2) = 200
        local_array(3) = 300
        PRINT *, "Value read out of bounds (local_array(4)): ", local_array(4) ! Invalid read
        IF (ALLOCATED(local_array)) DEALLOCATE(local_array)
    END SUBROUTINE trigger_invalid_read

    SUBROUTINE trigger_uninitialized_use()
        INTEGER :: local_uninitialized_var
        PRINT *, "Attempting use of uninitialized value..."
        ! 'local_uninitialized_var' is declared but not assigned a value.
        IF (local_uninitialized_var > 0) THEN
            PRINT *, "Uninitialized variable was positive."
        ELSE
            PRINT *, "Uninitialized variable was not positive (or zero/negative)."
        END IF
    END SUBROUTINE trigger_uninitialized_use

    SUBROUTINE trigger_memory_leak()
        INTEGER, ALLOCATABLE :: local_leaky_array(:)
        PRINT *, "Allocating memory that will be leaked..."
        ALLOCATE(local_leaky_array(1:1000))
        local_leaky_array(1) = 777
        PRINT *, "Value in local_leaky_array(1): ", local_leaky_array(1)
        ! We will not deallocate local_leaky_array, causing a "definitely lost" leak.
    END SUBROUTINE trigger_memory_leak

END PROGRAM fortran_error_generator