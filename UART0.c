// Hardware configuration:
// UART Interface:
//   U0TX (PA1) and U0RX (PA0) are connected to the 2nd controller
//   The USB on the 2nd controller enumerates to an ICDI interface and a virtual COM port

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <JLib/UART0.h>
#include <stdint.h>
#include <stdbool.h>
#include "JLib/tm4c123gh6pm.h"

// PortA masks
#define UART_TX_MASK 2 // PA1
#define UART_RX_MASK 1 // PA0

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// Initialize UART0
void initUart0()
{
    // Enable clocks
    SYSCTL_RCGCUART_R |= SYSCTL_RCGCUART_R0;
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R0;
    _delay_cycles(3);

    // Configure UART0 pins
    GPIO_PORTA_DR2R_R |= UART_TX_MASK;                  // set drive strength to 2mA (not needed since default configuration -- for clarity)
    GPIO_PORTA_DEN_R |= UART_TX_MASK | UART_RX_MASK;    // enable digital on UART0 pins
    GPIO_PORTA_AFSEL_R |= UART_TX_MASK | UART_RX_MASK;  // use peripheral to drive PA0, PA1
    GPIO_PORTA_PCTL_R &= ~(GPIO_PCTL_PA1_M | GPIO_PCTL_PA0_M); // clear bits 0-7
    GPIO_PORTA_PCTL_R |= GPIO_PCTL_PA1_U0TX | GPIO_PCTL_PA0_U0RX;
                                                        // select UART0 to drive pins PA0 and PA1: default, added for clarity

    // Configure UART0 to 115200 baud, 8N1 format
    UART0_CTL_R = 0;                                    // turn-off UART0 to allow safe programming
    UART0_CC_R = UART_CC_CS_SYSCLK;                     // use system clock (40 MHz)
    UART0_IBRD_R = 21;                                  // r = 40 MHz / (Nx115.2kHz), set floor(r)=21, where N=16
    UART0_FBRD_R = 45;                                  // round(fract(r)*64)=45
    UART0_LCRH_R = UART_LCRH_WLEN_8 | UART_LCRH_FEN;    // configure for 8N1 w/ 16-level FIFO
    UART0_CTL_R = UART_CTL_TXE | UART_CTL_RXE | UART_CTL_UARTEN;
                                                        // enable TX, RX, and module
}

// Set baud rate as function of instruction cycle frequency
void setUart0BaudRate(uint32_t baudRate, uint32_t fcyc)
{
    uint32_t divisorTimes128 = (fcyc * 8) / baudRate;   // calculate divisor (r) in units of 1/128,
                                                        // where r = fcyc / 16 * baudRate
    divisorTimes128 += 1;                               // add 1/128 to allow rounding
    UART0_CTL_R = 0;                                    // turn-off UART0 to allow safe programming
    UART0_IBRD_R = divisorTimes128 >> 7;                // set integer value to floor(r)
    UART0_FBRD_R = ((divisorTimes128) >> 1) & 63;       // set fractional value to round(fract(r)*64)
    UART0_LCRH_R = UART_LCRH_WLEN_8 | UART_LCRH_FEN;    // configure for 8N1 w/ 16-level FIFO
    UART0_CTL_R = UART_CTL_TXE | UART_CTL_RXE | UART_CTL_UARTEN;
                                                        // turn-on UART0
}

void setupUart0()
{
    initUart0();
    setUart0BaudRate(115200, 40e6);
}

// Blocking function that writes a serial character when the UART buffer is not full
void putcUart0(char c)
{
    while (UART0_FR_R & UART_FR_TXFF);               // wait if uart0 tx fifo full
    UART0_DR_R = c;                                  // write character to fifo
}

// Blocking function that writes a string when the UART buffer is not full
void putsUart0(char* str)
{
    uint8_t i = 0;
    while (str[i] != '\0')
        putcUart0(str[i++]);
}

// Blocking function that returns with serial data once the buffer is not empty
char getcUart0()
{
    while (UART0_FR_R & UART_FR_RXFE);               // wait if uart0 rx fifo empty
    return UART0_DR_R & 0xFF;                        // get character from fifo
}

// Returns the status of the receive buffer
bool kbhitUart0()
{
    return !(UART0_FR_R & UART_FR_RXFE);
}

/*
// receive string input from UART
int32_t getsUart0(USER_DATA *data)
{
    char count = 0;
    while(1)
    {
        char input = getcUart0();

        //if backspace then have the last character overwritten
        if ((input == 8 || input == 127) && count > 0)
        {
            count--;
        }

        //if carriage return then add null terminator to end of string
        else if (input == 13)
        {
            data->buffer[count] = '\0';
            return 0;
        }
        else if (input >= 32)
        {
            data->buffer[count] = input;
            count++;

            if (count == MAX_CHARS - 1)
            {
                data->buffer[count] = '\0';
                return 0;
            }
        }
    }
}
*/

// parse the data into fields to make it easier to read from
void parseFields(USER_DATA *data)
{

    char lastField = 'd';
    uint8_t i = 0;
    data->fieldCount = 0;

    for (i = 0; data->buffer[i] != '\0' && i < MAX_CHARS && data->fieldCount < MAX_FIELDS; i++)
    {
        //if alpha
        if ((data->buffer[i] >= 'a' && data->buffer[i] <= 'z') || (data->buffer[i] >= 'A' && data->buffer[i] <= 'Z'))
        {
            if (lastField == 'd')
            {
                data->fieldPosition[data->fieldCount] = i;
                data->fieldType[data->fieldCount] = 'a';
                data->fieldCount++;
                lastField = 'a';
            }
        }
        //if numeric
        else if (data->buffer[i] >= '0' && data->buffer[i] <= '9')
        {
            if (lastField == 'd')
            {
                data->fieldPosition[data->fieldCount] = i;
                data->fieldType[data->fieldCount] = 'n';
                data->fieldCount++;
                lastField = 'n';
            }

        }
        //if delimiter
        else
        {
            data->buffer[i] = '\0';
            lastField = 'd';
        }
    }
}

// receive integer input from a field
int32_t getFieldInteger(USER_DATA* data, uint8_t fieldNumber)
{
    uint8_t index;
    int32_t sum = 0;

    for (index = data->fieldPosition[fieldNumber]; data->buffer[index] != '\0'; index++)
    {
        sum = (sum*10) + data->buffer[index] - '0';
    }

    return sum;
}

// receive float input from a field
float getFieldFloat(USER_DATA* data, uint8_t fieldNumber)
{
    uint8_t index;
    float sum = 0;

    for (index = data->fieldPosition[fieldNumber]; data->buffer[index] != '\0'; index++)
    {
        sum = (sum*10) + data->buffer[index] - '0';
    }

    return sum;
}

// receive string input from a field
char * getFieldString(USER_DATA* data, uint8_t fieldNumber)
{
    char returnString[MAX_CHARS] = "";

    if (fieldNumber >= MAX_FIELDS)
    {
        return '\0';
    }

    int32_t i;

    for (i = 0; data->buffer[i + data->fieldPosition[fieldNumber]] != '\0'; i++)
    {
        returnString[i] = data->buffer[i + data->fieldPosition[fieldNumber]];
    }

    return returnString;
}


// put an integer into UART buffer
void putiUart0(int32_t num)
{

    int32_t i = 0;
    int32_t total = num;
    int32_t currentDigit = 0;
    char outputString[99] = "";
    if (total == 0)
    {
        outputString[0] = '0';
    }

    for (i = 0; total != 0; i++)
    {
        // digit will equal the remainder
        currentDigit = total % 10;

        // save value in a temporary string
        outputString[i] = currentDigit + '0';

        // move on to the next digit for the next iteration
        total /= 10;
    }

    // find length of current string
    for (i = 0; outputString[i] != '\0'; i++);

    // length of string
    int32_t length = i;

    // j will be starting at the end of the string
    int32_t j;
    char tempChar;

    // we will flip the string since it is on least significant digit
    for (i = 0, j = length - 1; i < j; i++, j--)
    {
        tempChar = outputString[i];
        outputString[i] = outputString[j];
        outputString[j] = tempChar;
    }

   putsUart0(outputString);
}
