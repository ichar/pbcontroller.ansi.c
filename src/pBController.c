#
/*******************************************************************************
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
 *
 *  ... // port -B- callback code
 *  ...    int code, IsError;
 *  ...    char *s[] = "any buffer to send a message";
 *  ... // initialize port -B- (turn on interrupts for receiver/transmitter)
 *  ...    pBInit(1, 1);
 *  ... // reallocate PMON exceptions handler (XXX)
 *  ...    initExcept();
 *  ... // push an input request to the port queue (i.e. receive a command)
 *  ...    IsError = pBInRequest(0, 0);
 *  ... // check an error...
 *  ...    if( !IsError ) code = 0;
 *  ... // push an output request to the port queue (i.e. send a message)
 *  ...    IsError = pBOutRequest("%s", s);
 *  ... // check an error...
 *  ...    if( !IsError ) code = 0;
 *  ... // or push it explicitly (put standard line delimeters inside)
 *  ...    push(s1, 1, 0);
 *  ...    push(s2, 1, 0);
 *  ...    ...
 *  ... // start global IRQ events distribution (note: var *events* is not specified)
 *  ...    while( events ) {
 *  ... // call transmitter to send a byte, 'code' is output callback code
 *  ...        do code = pBSend(0); while( code < 0 );
 *  ... // the output request was done, message was sent
 *  ...        if( code == PB_OK ) ...;
 *  ... // call receiver to get a byte, 'code' is input callback code
 *  ...        code = pBReceive(0);
 *  ... // the input request was done, command was received
 *  ...        if( code == PB_OK ) ...;
 *  ...    }
 *  ... // restore PMON exceptions handler (XXX)
 *  ...    DeinitExcept();
 *  ... // terminate port -B-
 *  ...    pBTerm();
 *
 *  Assembly: see '..\pBController\start.c' example.
 *
 *  v 1.03, 15/01/2010, ichar.
 *
 ***/

#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include "..\config.h"

#include "pBController.h"

#include "..\common\pBCommon.h"
#include "..\common\pBIRQ.h"

// -----------------------------------------------------------------------------
//  Declarations
// -----------------------------------------------------------------------------
unsigned char *BaseAddress;             // registers area base pointer

unsigned char  pb_cnr_saved;            // saved *CNR* register
unsigned char  pb_ier_saved;            // saved *IER* register

unsigned char  isr_pb_state;            // port interrupt reason (*ISR_PB*)

#ifdef PB_USE_LOGGER
char  msg[LOGGER_SIZE];                 // trace messages log
#endif

int   port_mode = MODE_NONE;            // port direction mode
unsigned char  rx;                      // auxiliary

// *****************************************************************************
//  RS232 PORT -B- SPECIFICATIONS
// *****************************************************************************

int   RS232_Parity[2] = {0,1};          // parity
                                        // transmitting speed values
int   RS232_Speeds[3] = {
           SPEED_19200, SPEED_38400, SPEED_115200
      };
char *RS232_SpeedsStr[2][3] = {
          {"19200", "38400", "115200"}, {"1200", "2400", "4800"}
      };

// *****************************************************************************
//  DATA INPUT QUEUE (INPUT REQUESTS)
// *****************************************************************************
                                        // input queue and pointers (FIFO)
TInItem aInItemsQueue[MAX_INPUT_ITEMS_COUNTER], *pInItemsQueue, *pInNext;
TInItem null_in_item = { 0, 0 };
int   nInItems = 0;                     // input items counter

// *****************************************************************************
//  DATA OUTPUT QUEUE (OUTPUT REQUESTS)
// *****************************************************************************
                                        // output queue and pointers (FIFO)
char  aOutItemsQueue[OUTPUT_SIZE] = "\0", *pOutItemsQueue, *pOutNext;
int   nOutItems = 0;                    // output items counter

#ifdef PB_STATISTICS
int   nMaxOutItems, nMaxOutQueueSize, nMaxOutItemSize;
#endif

// *****************************************************************************
//  PORT STATE CONTROL (PROTECTED)
// *****************************************************************************

void SetPortSpeed( int Speed ) {
//
//  Set data transmitting speed (*CNR->SPEED*).
//  -------------------------------------------
//  Arguments:
//
//      Speed -- speed value {0,1,2}.
//
#ifdef PB_CLEAN_REGISTER
    BaseAddress[PB_CNR] &= ~0x07;          // clean&set *SPEED* and *E_P(ready)*
#endif
    BaseAddress[PB_CNR] |= ((Speed & 0x06) | 0x01);
}

void SetPortLoop( char IsLoop ) {
//
//  Set port loop mode (*CNR->LOOP*).
//  --------------------------------
//  Arguments:
//
//      IsLoop -- 1/0.
//
#ifdef PB_CLEAN_REGISTER
    BaseAddress[PB_CNR] &= ~(0x08 | 0x01); // clean&set *LOOP* and *E_P(ready)*
#endif
    BaseAddress[PB_CNR] |= ( (IsLoop ? 0x08:0x00) | 0x01 );
}

void SetPortParity( int Parity ) {
//
//  Set parity control mode (*CNR->TP*).
//  ------------------------------------
//  Arguments:
//
//      Parity -- 1/0 (even/odd).
//
#ifdef PB_CLEAN_REGISTER
    BaseAddress[PB_CNR] &= ~(0x10 | 0x01); // clean&set *TP* and *E_P(ready)*
#endif
    BaseAddress[PB_CNR] |= ( (Parity ? 0x10:0x00) | 0x01 );
}

void SetIRQStatus( int mode, int IsEnable ) {
//
//  Set interrupts mode (*IER*).
//  ----------------------------
//  Arguments:
//
//      mode -- 1/0 (EIRC/EITR, receiver/transmitter)
//
//      IsEnable -- 1/0 (enable/disable).
//
#ifdef PB_USE_PORT_INTERRUPTS
    if( IsEnable )
        BaseAddress[PB_IER] |= (mode ? 0x02:0x01);
    else
        BaseAddress[PB_IER] &= (mode ? 0xFD:0xFE);
#endif
}

int GetIRQStatus( int mode ) {
//
//  Set interrupts mode (*IER*).
//  ----------------------------
//  Arguments:
//
//      mode -- 1/0 (EIRC/EITR, receive/transmit)
//
//  Returns:
//
//      IRQ status (a byte).
//
    return (BaseAddress[PB_IER] & (mode ? 0x02:0x01));
}

void SetPortRegister( int Register, unsigned char Value ) {
//
//  Set *register* state.
//  ---------------------
//  Arguments:
//
//      Register -- offset
//
//      Value -- state (byte).
//
    BaseAddress[Register] = Value;
}

unsigned char GetPortRegister( int Register, int IsLog ) {
//
//  Get *register* state.
//  ---------------------
//  Arguments:
//
//      Register -- offset
//
//      IsLog -- 1/0, send to messages log or not.
//
//  Returns:
//
//      Register state value (byte).
//
    rx = BaseAddress[Register];

#ifdef PB_USE_LOGGER
    if( IsLog )
        logger( msg, 1, "... REGISTER[%x]: %d\n", Register, rx );
#endif

    return rx;
}

int GetPortErrorMask( unsigned char status ) {
//
//  Checks and returns port *error* bits (*ISR->ERP, ERF, OV*).
//  -----------------------------------------------------------
//
//  Returns:
//
//      Error status (a byte).
//
#ifdef DEBUG
    rx = GetPortRegister(PB_CNR, 1);
    rx = GetPortRegister(PB_STATUS, 1);
    rx = GetPortRegister(PB_IER, 1);
    rx = GetPortRegister(PB_TXHR, 1);
#endif
    if( status )
        return (status & TX_ERROR_MASK);

    return (BaseAddress[PB_STATUS] & TX_ERROR_MASK);
}

int IsTXPortReady( int Timeout ) {
//
//  Waiting transmitter to be ready (*ISR->BTR*). Ready to send.
//  ------------------------------------------------------------
//
//  Arguments:
//
//      Timeout -- timeout value. If zero, check IRQ state.
//
//  Returns:
//
//      1/0 -- ready or not.
//
    if( !Timeout ) return ( !(isr_pb_state & TXRDY) ? 1:0 );

    while( ( BaseAddress[PB_STATUS] & TXRDY ) )
     {
        if( !( --Timeout ) )
            return 0;
     }

    return 1;
}

int IsRXPortReady( int Timeout ) {
//
//  Waiting receiver to be ready (*ISR->ENDRC*). Ready to receive.
//  --------------------------------------------------------------
//
//  Arguments:
//
//      Timeout -- timeout value. If zero, check IRQ state.
//
//  Returns:
//
//      1/0 -- ready or not.
//
    if( !Timeout ) return ( (isr_pb_state & RXRDY) ? 1:0 );

    while( !( BaseAddress[PB_STATUS] & RXRDY ) )
     {
        if( !( --Timeout ) )
            return 0;
     }

    return 1;
}

// *****************************************************************************
//  SERVER CONTROL (PRIVATE)
// *****************************************************************************

void _setBase() {
//
//  Set registers base address
//
    BaseAddress = (void *)DEF_RS_BASE_ADDRESS_B;
#ifdef MIPSBE
    BaseAddress +=3;
#endif
}

void _initInItemsQueue() {
//
//  Initialize receiver queue
//  -------------------------
//
    int i;
    for( i=0; i<MAX_INPUT_ITEMS_COUNTER; i++ )
        aInItemsQueue[i] = null_in_item;
    pInItemsQueue = &aInItemsQueue[0];
    pInNext = pInItemsQueue;
    nInItems = 0;
}

void _initOutItemsQueue() {
//
//  Initialize transmitter queue
//  ----------------------------
//
    aOutItemsQueue[0] = '\0';
    pOutItemsQueue = &aOutItemsQueue[0];
    pOutNext = pOutItemsQueue;
    nOutItems = 0;

#ifdef PB_STATISTICS
    nMaxOutItems = 0;
    nMaxOutQueueSize = 0;
    nMaxOutItemSize = 0;
#endif
}

void _initPortBController() {
//
//  Check port "B" state and initialize it to work.
//  -----------------------------------------------
//
//  Returns:
//
//      NONE (successfully) or NEGATIVE QUANTITY (error).
//
    pb_cnr_saved = BaseAddress[PB_CNR];

    SetPortParity(1);           // set 'even' parity control
    SetPortLoop(0);             // disable LOOP
    SetPortSpeed(SPEED_38400);  // set speed

    port_mode = MODE_NONE;
}

void _termPortBController() {
//
//  Terminate port "B".
//  -------------------
//  Shift the queue and set current data pointer to the next queue item.
//
    TInItem *pr;
    char *ps;

#ifndef PB_RING_QUEUE
    int i;
#endif

#ifdef PB_USE_PORT_INTERRUPTS
    DisableInt();
#endif

    if( port_mode == MODE_TX ) {
    //  keep the queue beginning
        ps = &aOutItemsQueue[0];
    //  check the last item in the queue
        if( nOutItems <= 1 ) {
        //  continue at the beggining
            pOutItemsQueue = pOutNext = ps;
        //  set items counter
            nOutItems = 0;
        }
        else {
        //  check overstep the boundaries
            if( pOutNext > pOutItemsQueue ) {

#ifdef PB_RING_QUEUE
            //  continue at the next of the queue
                ++pOutItemsQueue;
#else
            //  shift the queue (*pop* off current item, FIFO)
                strshift(ps, ++pOutItemsQueue, pOutNext);
            //  set the next offset
                pOutNext = ps + ( pOutNext - pOutItemsQueue );
            //  continue at first of the queue
                pOutItemsQueue = ps;
#endif

            }
        //  set items counter
            --nOutItems;
        }
    //  clean the queue
        if( pOutNext == pOutItemsQueue ) aOutItemsQueue[0] = '\0';
    }
    else if( port_mode == MODE_RX ) {
    //  keep the queue beginning
        pr = &aInItemsQueue[0];
    //  check overstep the boundaries
        if( nInItems <= 1 ) {
        //  continue at the beggining
            pInItemsQueue = pInNext = pr;
        //  set items counter
            nInItems = 0;
        }
        else {
        //  check overstep the boundaries
            if( pInNext > pInItemsQueue ) {

#ifdef PB_RING_QUEUE
            //  continue at the next of the queue
                ++pInItemsQueue;
#else
            //  shift input queue (*pop* off current item, FIFO)
                for( i=0; i<nInItems; i++ )
                    aInItemsQueue[i] = aInItemsQueue[i+1];
                aInItemsQueue[i] = null_in_item;
                pInItemsQueue = pr;
#endif

            }
        //  set input items counter
            --nInItems;
        }
    }

    port_mode = MODE_NONE;

#ifdef PB_USE_PORT_INTERRUPTS
    EnableInt();
#endif
}

void _saveIERState() {
//
//  Save current IER state and disable port interrupts.
//  ---------------------------------------------------
//
//  save IRQ state
    pb_ier_saved = BaseAddress[PB_IER];
//  disable interrupts on receiver\transmitter
    if( pb_ier_saved ) BaseAddress[PB_IER] = '\0';
}

void _restoreIERState() {
//
//  Restore IER state.
//  ------------------
//
    if( BaseAddress[PB_IER] != pb_ier_saved ) BaseAddress[PB_IER] = pb_ier_saved;
}

void _delay( unsigned int Timeout ) {
    volatile int t = Timeout;
    while(--t) ;
}

// *****************************************************************************
//  CLIENT INTERFACE (PUBLIC)
// *****************************************************************************

int pBInit( int IsEIRCEnable, int IsEITREnable ) {
//
//  Initialize port -B- (set required operational state).
//  -----------------------------------------------------
//  Should be ran before any utilization.
//
//  Arguments:
//
//      IsEIRCEnable -- 1/0, receiver interrupts mode (enable/disable)
//
//      IsEITREnable -- 1/0, transmitter interrupts mode (enable/disable).
//
//  Returns:
//
//      NONE (successfully) or error status (not ready).
//

//  set registers area pointer
    _setBase();

#ifdef PB_USE_LOGGER
    logger( msg, 0, "" );
#endif

//  make default settings
    _initPortBController();

//  initialize receiver queue
    _initInItemsQueue();

//  initialize transmitter queue
    _initOutItemsQueue();

//  enable or disable IRQ
    pBEnableIRQ( IsEIRCEnable, IsEITREnable );

//  check port ready state
    return (GetPortErrorMask(0) ? 0:1);
}

void pBTerm() {
//
//  Terminate port -B- (set default state).
//  ---------------------------------------
//  Should be ran after any utilization.
//
    pBDisableIRQ( 0,0 );

#ifdef PB_USE_LOGGER
#ifdef PB_STATISTICS
    logger( msg, 1, "--> PORT -B- QUEUE STATISTICS:\n" );
    logger( msg, 1, "    queue size:     %d\n", OUTPUT_SIZE );
    logger( msg, 1, "    max queue size: %d\n", nMaxOutQueueSize );
    logger( msg, 1, "    max items:      %d\n", nMaxOutItems );
    logger( msg, 1, "    max item size:  %d\n", nMaxOutItemSize );
#endif
    logger( msg, 2, "" );
#endif
}

int pBInRequest( char *sItem, int nMaxSize ) {
//
//  Asynchronous Data Receiving from the port -B-.
//  ----------------------------------------------
//  Arguments:
//
//      sItem -- input buffer pointer
//
//      nMaxSize -- max size limits.
//
//  Returns:
//
//      NONE (successfully continued) or error callback code.
//
    int code = 0;
    int errors;

    if( !sItem && nMaxSize != 0 )
        return PB_ERR_UNDEFINED;

//  check port state
    if((errors = GetPortErrorMask(0)))
        return errors;

//  check *item* overflow
    if( nInItems >= MAX_INPUT_ITEMS_COUNTER )
        return PB_ERR_OVERFLOW;

//  push *item* in the queue
    (*pInNext).pItem = sItem;
    (*pInNext).nMaxSize = (nMaxSize > 0 ? nMaxSize:0);
    ++pInNext;

    ++nInItems;

//  OK. Let's go. Receive the first byte...
    code = pBReceive(1);
    return (code ? code : PB_ERR_NONE);
}

int pBPush( char *sItem, int IsNewLine, int IsLog ) {
//
//  Push item in the output queue.
//  ------------------------------
//
//  Arguments:
//
//      sItem -- output request string (queue item)
//
//      IsNewLine -- 1/0, insert new line/line feed
//
//      IsLog -- 1/0, debug logger
//
//  Returns:
//
//      1/0 - successfully or overflow.
//
    int i, nSize;
    char new_line[] = NEW_LINE;

#ifdef PB_USE_LOGGER
#ifdef DEBUG
    char *p;
#endif
#ifdef TRACE
    logger( msg, 1, "... sItem: %s\n", sItem );
#endif
#endif

    i = strsize(sItem);

#ifdef PB_NO_EMPTY_REQUEST
//  check an empty request (given *item*)
    if( i==0 || ( i==1 && ( strin(sItem[0], (char *)"\n\r\t\0") ) ) )
        return 1;
#endif

//  XXX  DisableInt();  XXX

//  check *item* overflow
    nSize = i + SIZE_OFFSET;
    if( nSize > MAX_OUTPUT_ITEM_SIZE ||
        nSize + strsize(aOutItemsQueue) > sizeof(aOutItemsQueue) )
        return 0;

//  push *item* in the queue
    if( nSize > SIZE_OFFSET ) {
        if( !pOutItemsQueue ) _initOutItemsQueue();
    //  make string delimeters
        if( IsNewLine && !endswith(sItem, new_line) )
            stradd(sItem, new_line);
    //  push it as the latest in the queue
        pOutNext = strpush(pOutNext, sItem);
        ++nOutItems;
    }

#ifdef PB_STATISTICS
//  statinfo
    if( nOutItems > nMaxOutItems ) nMaxOutItems = nOutItems;
    if( pOutNext > pOutItemsQueue + nMaxOutQueueSize ) nMaxOutQueueSize = pOutNext - pOutItemsQueue;
    if( nSize > nMaxOutItemSize ) nMaxOutItemSize = nSize;
#endif

//  XXX  EnableInt();  XXX

#ifdef DEBUG
#ifdef PB_USE_LOGGER
//  log *item* if needed
    if( IsLog ) {
        p = pOutNext;
        logger( msg, 1, "... QUEUE, items: %d, pOutItemsQueue: %x, pOutNext: %x\n", nOutItems, pOutItemsQueue, p );
        for( i=0; i<nOutItems; i++ ) {
            p = strpop(p);
            logger( msg, 1, "%s", p );
        }
    }
#endif
#endif

    return 1;
}

int pBOutRequest( char *fmt, ... ) {
//
//  Asynchronous Data Transmitting to the port -B-.
//  -----------------------------------------------
//  Arguments list is compatible with *printf*.
//
//  Returns:
//
//      NONE (successfully continued) or error callback code.
//
    va_list args;
    char sItem[MAX_OUTPUT_ITEM_SIZE];
    int code = 0;
    int errors;

//  check port state
    if((errors = GetPortErrorMask(0)))
        return errors;

//  get formatted string to push it in the queue
    va_start(args, fmt);
    vsprintf(sItem, fmt, args);

    if( !pBPush(sItem, 1, 0) ) return PB_ERR_OVERFLOW;

#ifdef DEBUG
#ifdef PB_USE_LOGGER
    logger( msg, 1, "... QUEUE, items: %d, current size: %d\n%s", nOutItems, strsize(aOutItemsQueue), aOutItemsQueue );
#endif
#endif

    if( !nOutItems ) return PB_ERR_EMPTY;

//  OK. Let's go. Transmit the first byte...
    code = pBSend(1);
    return (code ? code : PB_ERR_NONE);
}

int pBSend( int start ) {
//
//  *** SEND DATA ***
//  -----------------
//  Wait *TXRDY* ready state and write data into *TXD* register (a byte).
//
//  Arguments:
//
//      start -- 1/0, is it beggining of the request (i.e. we should send
//               the first byte of an *item* now) or not.
//
//  Returns:
//
//      NONE (successfully) or Error (invalid data transmitted or any...).
//
    unsigned char Data;
    int IsError = 0, IsFlushed = 0, IsIRQEnabled = 0, IsStart = 0;

//  check if request exists
    if( !nOutItems )
        return PB_OK;

//  check port direction
    if( port_mode == MODE_RX ) return PB_ERR_IS_BUSY;

    IsIRQEnabled = pBIsIRQEnabled( PB_EITR );

#ifdef PB_START_WITH_NEWLINE
//  get data for transmitting...(byte under queue's current position)
    if( start && IsIRQEnabled ) {
        Data = '\n';
        IsStart = 1;
    }
    else
#endif

//  if no interrupts, wait...
    if( port_mode == MODE_TX && IsIRQEnabled && !isr_pb )
        return PB_ERR_NONE;
    else
        Data = *pOutItemsQueue;

//  set transmitter port mode
    port_mode = MODE_TX;

//  reset IRQ trigger
    isr_pb = 0;

//  check the flush (riched last byte of a given item)
    if( Data == '\0' ) IsFlushed = 1;

    if( Data ) {
    //  check the errors
        if( IsIRQEnabled ) {
    //  if interrups enabled, check the reason XXX
            if( !IsTXPortReady( IRQ_TIMEOUT ) ) IsError = PB_ERR_IS_NOT_READY;
            isr_pb_state = 0;
        } else {
            if( !IsTXPortReady( DEFAULT_TIMEOUT ) ) IsError = PB_ERR_IS_NOT_READY;
        }

#ifdef DEBUG
#ifdef PB_USE_LOGGER
        logger( msg, 1, "--> SENT(%d): %d\n", IsError, Data );
#endif
#endif

    //  send data and move current position
        if( !IsError ) {
            BaseAddress[PB_TXHR] = Data;
            if( !IsStart ) ++pOutItemsQueue;
        }
    }

#ifdef DEBUG
#ifdef PB_USE_LOGGER
    else
        logger( msg, 1, "--> NONE: %d\n", Data );
#endif
#endif

//  shift the queue and terminate the port if finalized
    if( IsFlushed ) {
        _termPortBController();
    //  request was done
        return PB_OK;
    }

    return (IsError | PB_ERR_NONE);
}

int pBReceive( int start ) {
//
//  *** RECEIVE DATA ***
//  --------------------
//  Wait *RDYTR* ready state and read data from *RXD* register (a byte).
//
//  Arguments:
//
//      start -- 1/0, is it beggining of the request (i.e. we should receive
//               the first byte of an *item* now) or not.
//
//  Returns:
//
//      NONE (successfully) or Error (invalid data received or any...).
//
    unsigned char Data;
    int IsFlushed = 0, IsIRQEnabled = 0, IsOverflow = 0;

#ifdef PB_CHECK_ERRORS
    int IsError;
#endif

//  check if request exists
    if( !nInItems )
        return PB_OK;

//  check port direction
    if( port_mode == MODE_TX ) return PB_ERR_IS_BUSY;

    IsIRQEnabled = pBIsIRQEnabled( PB_EIRC );

    if( IsIRQEnabled ) {
    //  if no interrupts, wait...
        if( !isr_pb )
            return PB_ERR_NONE;

    //  reset IRQ trigger
        isr_pb = 0;

        if( !IsRXPortReady( IRQ_TIMEOUT ) ) {

#ifdef DEBUG
#ifdef PB_USE_LOGGER
            logger( msg, 1, "... NOT READY(%08b)\n", isr_pb_state );
#endif
#endif

            return PB_ERR_NONE;
        }

#ifdef PB_CHECK_ERRORS
    //  if an error, return...
        if( IsError = GetPortErrorMask(isr_pb_state) ) {

#ifdef DEBUG
#ifdef PB_USE_LOGGER
            logger( msg, 1, "... ERROR(%08b)\n", isr_pb_state );
#endif
#endif

#ifdef PB_USE_DELAY
            _delay( 100 );
#endif
            return IsError;
        }
#endif

    //  reset IRQ reason state
        isr_pb_state = 0;
    } 
    else if( !IsRXPortReady( DEFAULT_TIMEOUT ) )
        return PB_ERR_NONE;

//  set receiver port mode
    port_mode = MODE_RX;

//  check data for overflow
    if( (*pInItemsQueue).nMaxSize <= 1 ) {

#ifdef DEBUG
#ifdef PB_USE_LOGGER
        logger( msg, 1, "--> OVERFLOW: %d\n", (*pInItemsQueue).nMaxSize );
#endif
#endif

        Data = ENTER_CODE;
        IsOverflow = 1;
    } else
        Data = BaseAddress[PB_RXHR];

    if( Data ) {

#ifdef DEBUG
#ifdef PB_USE_LOGGER
        logger( msg, 1, "--> RECEIVED(%d): %d\n", isr_pb_state, Data );
#endif
#endif

        if( Data == ENTER_CODE ) {
            if( !IsOverflow ) *((*pInItemsQueue).pItem) = '\0';
            IsFlushed = 1;
        } 
        else
            *((*pInItemsQueue).pItem++) = Data;

        --(*pInItemsQueue).nMaxSize;
    }

#ifdef DEBUG
#ifdef PB_USE_LOGGER
    else
        logger( msg, 1, "--> NONE: %d\n", Data );
#endif
#endif

//  shift the queue and terminate the port if finalized
    if( IsFlushed ) {
        _termPortBController();
    //  request was done
        return PB_OK;
    }

    return PB_ERR_NONE;
}

int pBIsIRQEnabled( int mode ) {
//
//  Checks if IRQ port -B- enabled.
//  -------------------------------
//
//  Arguments:
//
//      mode -- 1/0 (EIRC/EITR, receiver/transmitter)
//
//  Returns:
//
//      1/0 -- enabled or not.
//
    return ( GetIRQStatus(mode) ? 1:0 );
}

void pBPrintf( char *log ) {
//
//  Print the *log*, disable port interrupts before.
//  ------------------------------------------------
//
//  Arguments:
//
//      log -- ponter to the messages buffer.
//
    _saveIERState();  printf(log);  _restoreIERState();
}

int pBGetchar() {
//
//  Get from *stdin*, disable port interrupts before.
//  -------------------------------------------------
//
    _saveIERState();  rx = getc(stdin);  _restoreIERState();  return rx;
}
