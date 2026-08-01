        .module page_a
        .globl page_target
        .area PAGECALLER (REL,CON,CODE)

        acall page_target
        ajmp page_target
