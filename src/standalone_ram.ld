//
// Program layout for running out of RAM.
//
//

SECTIONS
{
    .zdata						ABS : > zero_memory
    .zbss						ABS : > .
    .rozdata					        ABS : > .

    .text						    : > dram_memory
    .syscall						    : > .
    .secinfo						    : > .
    .fixaddr						    : > .
    .fixtype						    : > .
    .robase				           ALIGN(8) : > .
    .rodata						    : > .

    .sdabase			                   ALIGN(8) : > .
    .sdata						    : > .
    .sbss						    : > .
    .rosdata						    : > .
    .data						    : > .
    .profile						    : > .
    .bss						    : > .
    .heap		        ALIGN(8) PAD(heap_reserve)  : > .
    .stack		        ALIGN(8) PAD(stack_reserve) : > .

//
// These special symbols mark the bounds of RAM and ROM memory.
// They are used by the MULTI debugger.
//
    __ghs_ramstart  = MEMADDR(dram_rsvd1);
    __ghs_ramend    = MEMENDADDR(dram_rsvd2);
    __ghs_romstart  = MEMADDR(flash_rsvd1);
    __ghs_romend    = MEMENDADDR(flash_rsvd2);
    
}
