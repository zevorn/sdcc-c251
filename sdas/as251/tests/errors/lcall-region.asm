        .module invalid_lcall_region
        .area MCS251TEST (ABS,CODE)
        .org 0
        .binary

        lcall 0x010000
