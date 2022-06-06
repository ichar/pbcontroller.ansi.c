MEMORY {
    zero_memory  : ORIGIN = 0xffff8000, LENGTH = 64K
    
    dram_rsvd1   : ORIGIN = 0x80000000, LENGTH = 32K
    dram_memory  : ORIGIN = .,          LENGTH = 10M-32K
    dram_rsvd2   : ORIGIN = ., 	        LENGTH = 0

    flash_rsvd1  : ORIGIN = 0xbfc00000, LENGTH = 32K
    flash_memory : ORIGIN = .,          LENGTH = 10M - 32K
    flash_rsvd2  : ORIGIN = .,          LENGTH = 0
}
