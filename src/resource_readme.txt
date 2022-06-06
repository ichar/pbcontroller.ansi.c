Board Setup Files (.dbs)
------------------------------------------------------------------------------
The board setup files (.dbs) are used by Green Hills Software's debug servers 
to initialize target hardware before beginning a debugging session.

The automatically-created default.con connection file contains
connection methods that reference the .dbs files in this resource.bld.

You can modify the scripts in the board setup files to suit your specific
hardware configuration and connection needs.

For more information about board setup files and connection files, 
see the "Target Connection User's Guide" for your processor.


Linker Directive Files (.ld)
------------------------------------------------------------------------------
The linker directive files (.ld) are used to link your program.

This resource.bld contains five linker directives files:
** memory.ld --- Defines the memory map for the target board.
** standalone_config.ld --- Defines constants that the other linker 
directives files use to define the program layout. For example, it
can define the size of the target's stack and heap. 
** standalone_ram.ld -- Used for programs that are linked into and run out
of RAM. 
** standalone_romcopy.ld -- Used for programs that are linked into ROM, but 
run out of RAM.
** standalone_romrun.ld -- Used for programs that are linked into and run
out of ROM. 
 
memory.ld and standalone_config.ld are always used by the linker in 
conjunction with one of the files that defines a program layout 
(standalone_ram.ld, standalone_romcopy.ld, or standalone_romrun.ld). 
To choose which linker directives file will be used for the program layout:
1. In the MULTI Builder, choose Project -> File Options.
2. On the Advanced tab, edit the Program Layout drop-down list.

To use a custom .ld file instead of the .ld files located in resource.bld, 
place your .ld file into the .bld file of your executable project.

You can modify the linker directives files to suit your specific
hardware configuration and program layout needs.

For more information about linker directives files, see the "Embedded 
Development Guide" for your processor.
