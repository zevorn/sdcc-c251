        .module acall_page
        .area MCS251TEST (ABS,CODE)
        .org 0
        .source

        acall 0x0800
        ajmp 0x0800
