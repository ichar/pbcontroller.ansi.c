#
/*******************************************************************************
 *  Port -B- Controller debugger (usage sample)
 *  -------------------------------------------
 *
 *  v 1.03, 15/01/2010, ichar.
 *
 ***/

#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "..\config.h"

// -----------------------------------------------------------------------------
//  Interface
// -----------------------------------------------------------------------------

int  main( int, char **, char ** );
void test_transmitter( char * );
void test_receiver( void );

// -----------------------------------------------------------------------------
//  RS232 PORT -B- EXTERNAL DECLARATIONS
// -----------------------------------------------------------------------------

#include "..\common\common.h"
// .............................................................................
#include "..\common\pBCommon.h"
#include "..\common\pBExt.h"
#include "..\common\usr.h"

// *****************************************************************************
//  PORT -B- DEBUGGER
// *****************************************************************************

int main( int argc, char **argv, char **envp ) {
    char s[1024];

#ifdef PB_USE_LOGGER
    int  UseLogger;
#endif

//  initialize system controller
    initSC();

//  initialize port -B- (no interrupts by default)
    pBInit(0, 0);

    while( 1 ) {

#ifdef PB_USE_LOGGER
    //  start trace log
        logger( msg, 0, "" );
        UseLogger = 1;
#endif

    //  get *stdin* command line
        intype(s, sizeof(s), 0);

    //  commands...
        if( strcmp(s, (char *)"exit") == 0 )
            break;
        else if( strcmp(s, (char *)"help") == 0 || strcmp(s, (char *)"h") == 0 ) {
            pBPrintf( "Port -B- Controller debugger (v 1.0, 20/12/2009).\n" );
            pBPrintf( "Use commands:\n" );
            pBPrintf( " 'GET EITR' - print current IER state\n" );
            pBPrintf( " 'push ...' - push output request in the controller queue\n" );
            pBPrintf( " 'receive'  - push input request in the controller queue\n" );
            pBPrintf( " 'regs'     - print port registers\n" );
            pBPrintf( " 'on'       - enable IRQ\n" );
            pBPrintf( " 'tr on'    - enable EITR IRQ\n" );
            pBPrintf( " 'rc on'    - enable EIRC IRQ\n" );
            pBPrintf( " 'off'      - disable IRQ\n" );
            pBPrintf( "press *Enter* to GO or checking state, another way put data of a new output request.\n" );
        }
        else if( strcmp(s, (char *)"GET EITR") == 0 ) {
            isr_pb_state = GetPortRegister(PB_IER, 1);
        }
        else if( strcmp(s, (char *)"GET REGISTERS") == 0 || strcmp(s, (char *)"regs") == 0 ) {

#ifdef PB_USE_LOGGER
            logger( msg, 1, "... CNR   (0x00): %08b\n", GetPortRegister(PB_CNR, 0) );
            logger( msg, 1, "... STATUS(0x04): %08b\n", GetPortRegister(PB_STATUS, 0) );
            logger( msg, 1, "... IER   (0x08): %08b\n", GetPortRegister(PB_IER, 0) );
#endif

        }
        else if( strcmp(s, (char *)"on") == 0 ) {
            pBEnableIRQ(1,1);
            rx = GetPortRegister(PB_IER, 1);
            isr_pb_state = 0;
        }
        else if( strcmp(s, (char *)"SET EITR ON") == 0 || strcmp(s, (char *)"tr on") == 0 ) {
            pBEnableIRQ(0,1);
            rx = GetPortRegister(PB_IER, 1);
            isr_pb_state = 0;
        }
        else if( strcmp(s, (char *)"SET EIRC ON") == 0 || strcmp(s, (char *)"rc on") == 0 ) {
            pBEnableIRQ(1,0);
            rx = GetPortRegister(PB_IER, 1);
            isr_pb_state = 0;
        }
        else if( strcmp(s, (char *)"off") == 0 ) {
            pBDisableIRQ(0,0);
            rx = GetPortRegister(PB_IER, 1);
        }

    //  an input request...
        else if( strmatch(s, (char *)"receive") ) {
            test_receiver();
        }
    //  or an output request...
        else if( strmatch(s, (char *)"push") ) {
            pBPush(&s[5], 1, 1);
        }
        else {
            test_transmitter( s );
        }


#ifdef PB_USE_LOGGER
    //  print debug log
        if( UseLogger ) logger( msg, 2, "" );
#endif
    }

//  terminate port -B-
    pBTerm();

//  terminate system controller
    termSC();

    printf("end.\n");
    exit(0);
}

void test_transmitter( char *s ) {
    int  n, code, IsError;
    int  UseEITR = 0;

#ifdef DEBUG
    int  IsInterrupt;
#endif

    initExcept();

//  check IRQ state
    UseEITR = pBIsIRQEnabled(PB_EITR);

//  push request to the port -B- queue
    IsError = pBOutRequest("%s", s);

//  check it's accepted or not
    if( IsError )
#ifdef PB_USE_LOGGER
        logger( msg, 1, "... OUTPUT REQUEST ERROR: %d\n", IsError );
#else
        n = IsError;
#endif
    else {
        code = 0;
        n = 0;

#ifdef DEBUG
        IsInterrupt = 0;
#endif
    //  call transmitter to send a byte
        while( !code ) {
            if( ++n > 1000000 ) break;

        //  if interrupt's enabled, wait it ...
            if( UseEITR ) {
                if( isr_pb == 0 ) continue;
#ifdef DEBUG
#ifdef PB_USE_LOGGER
                logger( msg, 1, "... INTERRUPT[%d], STATUS: %08b\n", isr_pb, isr_pb_state );
#endif
                IsInterrupt = 1;
#endif
                n = 0;
            }
#ifdef GREEN
            code = OK;
#else
            code = pBSend(0);
#endif
        //  if interrupt's enabled, clean IRQ reason registry
//            if( UseEITR ) isr_pb_state = 0;
        }

#ifdef DEBUG
        if( UseEITR && !IsInterrupt ) {
#ifdef PB_USE_LOGGER
            logger( msg, 1, "... NO INTERRUPTS(%d:%d:%08b)\n", n, isr_pb, isr_pb_state );
#endif
            isr_pb_state = BaseAddress[PB_STATUS];
        }
#endif
    }

    DeinitExcept();
}

void test_receiver() {
    char s[10];
    int  n, code, IsError;
    int  UseEIRC = 0;

#ifdef DEBUG
    int  IsInterrupt;
#endif

    initExcept();

//  check IRQ state
    UseEIRC = pBIsIRQEnabled(PB_EIRC);

    IsError = pBInRequest(s, 9);

//  check it's accepted or not
    if( IsError )
#ifdef PB_USE_LOGGER
        logger( msg, 1, "... INPUT REQUEST ERROR: %d\n", IsError );
#else
        n = IsError;
#endif
    else {
        code = 0;
        n = 0;

#ifdef DEBUG
        IsInterrupt = 0;
#endif
    //  call receiver to receive a byte
        while( code != PB_OK ) {
            if( ++n > 10000000 ) break;

        //  if interrupt's enabled, wait it ...
            if( UseEIRC ) {
                if( isr_pb == 0 ) continue;
#ifdef DEBUG
#ifdef PB_USE_LOGGER
                logger( msg, 1, "... INTERRUPT[%d], STATUS: %08b\n", isr_pb, isr_pb_state );
#endif
                IsInterrupt = 1;
#endif
                n = 0;
            }
#ifdef GREEN
            code = OK;
#else
            code = pBReceive(0);
#endif
        //  if interrupt's enabled, clean IRQ reason registry
//            if( UseEIRC ) isr_pb_state = 0;
        }

#ifdef DEBUG
        if( UseEIRC && !IsInterrupt ) {
#ifdef PB_USE_LOGGER
            logger( msg, 1, "... NO INTERRUPTS(%d:%d:%08b)\n", n, isr_pb, isr_pb_state );
#endif
            isr_pb_state = BaseAddress[PB_STATUS];
        }
#endif

        if( code == PB_OK ) {
#ifdef PB_USE_LOGGER
            logger( msg, 1, "%s\n", s );
#else
            pBPrintf(s);
            pBPrintf("\n");
#endif
        }
    }

    DeinitExcept();
}
