#
/*******************************************************************************
 *  Port -B- Controller header file
 *  -------------------------------
 *  Designed for BSOUK apps.
 *
 *  v 1.03, 15/01/2010, ichar.
 *
 ***/

#ifndef __PBCONTROLLER__
#define __PBCONTROLLER__

#define MIPSBE

// -----------------------------------------------------------------------------
//  Definitions
// -----------------------------------------------------------------------------
//
//  Port -B- register state definitions
//
#define DEFAULT_TIMEOUT          1000000
#define IRQ_TIMEOUT              0

#define RXRDY                    0x02     // Data has been received (ENDRC)
#define TXRDY                    0x20     // TXD busy or ready to transmit (BTR)
//
//  Transmitting speed values, CNR_PB[02:01]
//
#define SPEED_19200              0x04
#define SPEED_38400              0x02
#define SPEED_115200             0x00

#define DEF_RS_BASE_ADDRESS_A    0xBF800030
#define DEF_RS_BASE_ADDRESS_B    0xBF800040

#define MODE_RX                 -1        // receiver is busy (occupied)
#define MODE_NONE                0        // none
#define MODE_TX                  1        // transmitter is busy (occupied)

#define TX_ERROR_MASK           (0x04 | 0x08 | 0x10)

#define MAX_OUTPUT_ITEM_SIZE     1024
#define OUTPUT_SIZE              10*MAX_OUTPUT_ITEM_SIZE
#define MAX_INPUT_ITEMS_COUNTER  10

#define LOGGER_SIZE              20*1024

#define SIZE_OFFSET              2

#define ENTER_CODE               0x0D

// *****************************************************************************
//  CLASS PROTOTYPE DECLARATIONS (INTERFACE)
// *****************************************************************************
typedef unsigned int PADDR;

typedef struct {                          // input item
    char *pItem;                          // received data buffer pointer
    int   nMaxSize;                       // max size limits
} TInItem;
//
//  Protected ------------------------------------------------------------------
//
void  SetPortSpeed        ( int );
void  SetPortLoop         ( char );
void  SetPortParity       ( int );
void  SetIRQStatus        ( int, int );
int   GetIRQStatus        ( int );
void  SetPortRegister     ( int, unsigned char );
unsigned char GetPortRegister( int, int );
int   GetPortErrorMask    ( unsigned char );
int   IsTXPortReady       ( int );
int   IsRXPortReady       ( int );
//
//  Private --------------------------------------------------------------------
//
void  _setBase            ( void );
void  _initInItemsQueue   ();
void  _initOutItemsQueue  ();
void  _initPortBController( void );
void  _termPortBController( void );
void  _saveIERState       ();
void  _restoreIERState    ();
void  _delay              ( unsigned int );
//
//  Public (client interface) --------------------------------------------------
//
int   pBInit              ( int, int );         // port intialization
void  pBTerm              ( void );             // port termination
int   pBInRequest         ( char *, int );      // start receiving of a new line (...)
int   pBPush              ( char *, int, int ); // push an output request in the queue
int   pBOutRequest        ( char *, ... );      // start transmitting with a new request
int   pBSend              ( int );              // call transmitter (sends current byte)
int   pBReceive           ( int );              // call receiver (gets current byte)
int   pBIsIRQEnabled      ( int );              // check IRQ state
void  pBPrintf            ( char * );           // print given messages log buffer
int   pBGetchar           ();                   // get a byte from *stdin*
//
//  External -------------------------------------------------------------------
//
void  logger              ( char *, int,    char *, ... );
int   startswith          ( char *, char * );
int   endswith            ( char *, char * );
void  stradd              ( char *, char * );
int   strin               ( char,   char * );
char *strpop              ( char * );
char *strpush             ( char *, char * );
void  strshift            ( char *, char *, char * );
int   strsize             ( char * );

#endif
