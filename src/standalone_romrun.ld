//
// Program layout for starting in ROM, copying data to RAM,
// and continuing to execute out of ROM.
//

SECTIONS
{
//
// RAM SECTIONS
//
    .zdata						ABS : > zero_memory
    .zbss						ABS : > .
    .rozdata					        ABS : > .

    .sdabase			                   ALIGN(8) : > dram_memory
    .sdata						    : > .
    .sbss						    : > .
    .rosdata					            : > .
    .data						    : > .
    .profile					            : > .
    .bss						    : > .
    .heap		        ALIGN(8) PAD(heap_reserve)  : > .
    .stack		        ALIGN(8) PAD(stack_reserve) : > .

//
// ROM SECTIONS
//

    .text						    : > flash_memory
    .syscall				                    : > .

    .robase				           ALIGN(8) : > .
    .rodata					            : > .

    .secinfo					            : > .
    .fixaddr					            : > .
    .fixtype					            : > .

    .ROM.sdata			                ROM(.sdata) : > .
    .ROM.data			                 ROM(.data) : > .
    .ROM.profile		              ROM(.profile) : > .

//
// These special symbols mark the bounds of RAM and ROM memory.
// They are used by the MULTI debugger.
//
    __ghs_ramstart  = MEMADDR(dram_rsvd1);
    __ghs_ramend    = MEMENDADDR(dram_rsvd2);
    __ghs_romstart  = MEMADDR(flash_rsvd1);
    __ghs_romend    = MEMENDADDR(flash_rsvd2);

//
// These special symbols mark the bounds of RAM and ROM images of boot code.
// They are used by the GHS startup code (_start and __ghs_ind_crt0).
//
    __ghs_rambootcodestart  = 0;
    __ghs_rambootcodeend    = 0;
    __ghs_rombootcodestart  = ADDR(.text);
    __ghs_rombootcodeend    = ENDADDR(.ROM.profile);
}
