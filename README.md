## pBController.ansi-c 

Carried out as a test of the controller of the onboard digital machine of the special equipment of the ground control complex for 
space purposes.

 *  Port -B- Controller implementation
 *  ----------------------------------
 *  Designed for BSOUK apps.
 *
 *  Brief description:
 *
 *  The goal of module consists in providing independent operation of client
 *  side with port -B- (COM, RS232). We should connect to port differently in
 *  two mode, with handle line interrupts (IRQ) or not.
 *
 *  Public interface (client side functions):
 *  ----------------------------------------
 *
 *    pBInit(IsEIRCEnable, IsEITREnable) - port default settings (initialization),
 *      run first before any usages, arguments points type of IRQ mode (1/0,
 *      EIRC/EITR, receiver/transmiter, enable/disable)
 *
 *    pBTerm() - port default settings (termination), run last after any usages
 *
 *    pBOutRequest(fmt, ...) - queuering an output request (basic method),
 *      arguments fully compatible with *printf*, we can parse data
 *      by format string with any provided usages (%c, %s, %d, %x, %b ...),
 *      the function pushes a new item in the port's queue and starts
 *      transmitting from the first queue position
 *
 *    pBPush(sItem, IsNewLine, IsLog) - another way to push request (we can separate
 *      off queuering and transmitting steps by way of anticipatory filling
 *      port queue), argument *sItem* points data ready to transmit, *IsNewLine*
 *      pushes standard line delimeters ("\n\r") into request body, *IsLog*
 *      implemented to make logging, queue is an ordered list (FIFO)
 *
 *    pBSend(start) - call to port transmitter, sends currently pointed
 *      byte through RXD register, argument *start* (1/0) specifies visibility usage
 *      only (1 - puts new line '\n' before any item, designed for IRQ
 *      enabled mode only), returns finalization code (1/0) or an error as a
 *      negative value (see pBCommon.h, "Callback status code")
 *
 *    pBInRequest(char *sItem, int nMaxSize) - queuering an input request,
 *      argument *sItem* is an input buffer pointer for keeping data received
 *      from the port, *nMaxSize* specifies max size of receiving data (0 is
 *      allowed and pointed to take one symbol only, pressing *Enter*
 *      for instance)
 *
 *    pBReceive(start) - call to port receiver, takes a byte from the RXD
 *      register, argument *start* (1/0) specifies for compatibility issue
 *      with *pBSend*, returns finalization code (1/0) or an error as a
 *      negative value (see pBCommon.h, "Callback status code")
 *
 *    pBIsIRQEnabled(mode) - returns set-point IRQ state (1/0, enable/disable),
 *      argument *mode* is type of line (1/0, EIRC/EITR, receiver or
 *      transmitter)
 *
 *    pBGetchar() - gets *stdin* character (DEBUG), provided for IRQ
 *      handling
 *
 *    pBPrintf(log) - puts *stdout* messages log (DEBUG), provided for IRQ
 *      handling.
 *
 *  Sample ('s' - any string pointer):
 *  ----------------------------------

