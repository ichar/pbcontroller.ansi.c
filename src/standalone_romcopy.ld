//
// Program layout for starting in ROM, copying text and data to RAM,
// and then running out of RAM.
//

SECTIONS
{
//
// RAM SECTIONS
//
    .zdata						ABS : > zero_memory
    .zbss						ABS : > .
    .rozdata					        ABS : > .

    .text				           ALIGN(4) : > dram_memory
    .boottext ALIGN(4) : {
	  // All .text which is reachable between
	  // _start and __ghs_ind_crt1 needs to be
	  // pulled into .boottext and left
	  // uncompressed.
	  crt0.o(.text)
	  libstartup.a(*)(.text)
	  libsys.a(ind_crt1.o)(.text)
     }                                                        > .
    .syscall					   ALIGN(4) : > .


    .sdabase				           ALIGN(8) : > .
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
    .ROM.boottext                            ROM(.boottext) : > flash_memory
    .ROM.syscall		              ROM(.syscall) : > .

    .secinfo						    : > .
    .fixaddr						    : > .
    .fixtype						    : > .

    .robase				           ALIGN(8) : > .
    .rodata						    : > .

//
// Compressed ROM SECTIONS
//

    .CROM.zdata			               CROM(.zdata) : > .
    .CROM.rozdata		             CROM(.rozdata) : > .
    .CROM.text			                CROM(.text) : > .
    .CROM.sdata			               CROM(.sdata) : > .
    .CROM.rosdata			     CROM(.rosdata) : > .
    .CROM.data			                CROM(.data) : > .
    .CROM.profile		             CROM(.profile) : > .

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
    __ghs_rambootcodestart  = ADDR(.boottext);
    __ghs_rambootcodeend    = ENDADDR(.stack);
    __ghs_rombootcodestart  = ADDR(.ROM.boottext);
    __ghs_rombootcodeend    = ENDADDR(.rodata);
}



