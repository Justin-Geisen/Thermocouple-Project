//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz
// Stack:           4096 bytes (needed for snprintf)

// Hardware configuration:
// UART Interface:
//   U0TX (PA1) and U0RX (PA0) are connected to the 2nd controller
//   The USB on the 2nd controller enumerates to an ICDI interface and a virtual COM port
//   Configured to 115,200 baud, 8N1

// I2C devices on I2C bus 0 with 2kohm pullups on SDA (PB3) and SCL (PB2)

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "tm4c123gh6pm.h"
#include "clock.h"
#include "uart0.h"
#include "i2c0.h"
#include "wait.h"

// Range of polled devices
// 0 for general call, 1-3 for compatible i2c variants
// 120-123 are for 10-bit address mode
// 123-127 reserved
#define MIN_I2C_ADD 0x08
#define MAX_I2C_ADD 0x77


/*

PB2 SCL
PB3 SDA

 */


//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// Initialize Hardware
void initHw()
{
    // Initialize system clock to 40 MHz
    initSystemClockTo40Mhz();
}


void getsUart0(char str[], uint8_t size)
{
    uint8_t count = 0;
    bool end = false;
    char c;
    while(!end)
    {
        c = getcUart0();
        end = (c == 13) || (count == size);
        if (!end)
        {
            if ((c == 8 || c == 127) && count > 0)
                count--;
            if (c >= ' ' && c < 127)
                str[count++] = c;
        }
    }
    str[count] = '\0';
}


uint8_t asciiToUint8(const char str[])
{
    uint8_t data;
    if (str[0] == '0' && tolower(str[1]) == 'x')
        sscanf(str, "%hhx", &data);
    else
        sscanf(str, "%hhu", &data);
    return data;
}

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------

#define MAX_CHARS 80
int main(void)
{

    while(1)
    {
        char strInput[MAX_CHARS+1];
        char* token;
        char str[80];
        uint8_t i;
        uint8_t add;
        uint8_t reg;
        uint8_t data;
        uint8_t arg;
        bool regUsed;
        bool found;
        bool valid;
        bool ok;




        // Initialize hardware
        initHw();
        initUart0();
        initI2c0();

        // Setup UART0 baud rate
        setUart0BaudRate(115200, 40e6);

        putsUart0("I2C0 Utility\n");

        uint8_t currentConfig[20];
        uint8_t newConfig[20];
        uint8_t dataOut[20];
        float decodedTmp = 0;

        float tmpMv = 0;
        float thermoMv = 0;

        float voltSum = 0;

        // read TMP 36

        readI2c0Registers(0x48, 0x1, currentConfig, 2);

        newConfig[1] = currentConfig[1];

        // configure mux to AIN 0 positive
        newConfig[0] = currentConfig[0] & ~(0x7 << 4);
        newConfig[0] |= 64;

        // configure pga to 2.048 (default value)
        newConfig[0] &= ~(0x7 << 1);
        newConfig[0] |= 1 << 2;

        // write the new configuration to the adc register
        writeI2c0Registers(0x48, 0x1, newConfig, 2);


        waitMicrosecond(10000);

        // read the raw output
        readI2c0Registers(0x48, 0x0, dataOut, 2);

        _delay_cycles(100);


        float rawTmp = dataOut[1] + (dataOut[0] << 8);

        _delay_cycles(100);

        // convert the raw output to mV
        rawTmp = rawTmp / 32768 * 2.048 * 1000;
        _delay_cycles(100);

        tmpMv = rawTmp;

        decodedTmp = (rawTmp - 750) / 10 + 25;


        float tempOut = 0;
        float mvInput = 0;


        // read Thermocouple

        readI2c0Registers(0x48, 0x1, currentConfig, 2);

        newConfig[1] = currentConfig[1];

        // configure mux to AIN 2 positive and AIN 3 negative
        newConfig[0] = currentConfig[0] & ~(0x7 << 4);
        newConfig[0] |= 16 | 32;

        // configure pga to 0.256
        newConfig[0] |= (0x7 << 1);

        // write the new configuration to the adc register
        writeI2c0Registers(0x48, 0x1, newConfig, 2);

        waitMicrosecond(10000);

        // read the raw output
        readI2c0Registers(0x48, 0x0, dataOut, 2);

        rawTmp = dataOut[1] + (dataOut[0] << 8);

        // convert the raw output to mV
        thermoMv = 0.256 / 65536 * rawTmp * 2 * 1000;





        float tempIn = decodedTmp;
        float mvOut = 0;


        if (    tempIn >=   -270    &&  tempIn <=   -260    )   {   mvOut =     0.00170000000000003 *   (   tempIn  -   -270    )   +   -6.458  ;   }
        if (    tempIn >=   -260    &&  tempIn <=   -250    )   {   mvOut =     0.00369999999999999 *   (   tempIn  -   -260    )   +   -6.441  ;   }
        if (    tempIn >=   -250    &&  tempIn <=   -240    )   {   mvOut =     0.00599999999999996 *   (   tempIn  -   -250    )   +   -6.404  ;   }
        if (    tempIn >=   -240    &&  tempIn <=   -230    )   {   mvOut =     0.00820000000000007 *   (   tempIn  -   -240    )   +   -6.344  ;   }
        if (    tempIn >=   -230    &&  tempIn <=   -220    )   {   mvOut =     0.0103999999999999  *   (   tempIn  -   -230    )   +   -6.262  ;   }
        if (    tempIn >=   -220    &&  tempIn <=   -210    )   {   mvOut =     0.0123  *   (   tempIn  -   -220    )   +   -6.158  ;   }
        if (    tempIn >=   -210    &&  tempIn <=   -200    )   {   mvOut =     0.0144  *   (   tempIn  -   -210    )   +   -6.035  ;   }
        if (    tempIn >=   -200    &&  tempIn <=   -190    )   {   mvOut =     0.0161  *   (   tempIn  -   -200    )   +   -5.891  ;   }
        if (    tempIn >=   -190    &&  tempIn <=   -180    )   {   mvOut =     0.0180000000000001  *   (   tempIn  -   -190    )   +   -5.73   ;   }
        if (    tempIn >=   -180    &&  tempIn <=   -170    )   {   mvOut =     0.0196  *   (   tempIn  -   -180    )   +   -5.55   ;   }
        if (    tempIn >=   -170    &&  tempIn <=   -160    )   {   mvOut =     0.0213  *   (   tempIn  -   -170    )   +   -5.354  ;   }
        if (    tempIn >=   -160    &&  tempIn <=   -150    )   {   mvOut =     0.0228  *   (   tempIn  -   -160    )   +   -5.141  ;   }
        if (    tempIn >=   -150    &&  tempIn <=   -140    )   {   mvOut =     0.0244000000000001  *   (   tempIn  -   -150    )   +   -4.913  ;   }
        if (    tempIn >=   -140    &&  tempIn <=   -130    )   {   mvOut =     0.0258  *   (   tempIn  -   -140    )   +   -4.669  ;   }
        if (    tempIn >=   -130    &&  tempIn <=   -120    )   {   mvOut =     0.0273  *   (   tempIn  -   -130    )   +   -4.411  ;   }
        if (    tempIn >=   -120    &&  tempIn <=   -110    )   {   mvOut =     0.0286  *   (   tempIn  -   -120    )   +   -4.138  ;   }
        if (    tempIn >=   -110    &&  tempIn <=   -100    )   {   mvOut =     0.0298  *   (   tempIn  -   -110    )   +   -3.852  ;   }
        if (    tempIn >=   -100    &&  tempIn <=   -90 )   {   mvOut =     0.0311  *   (   tempIn  -   -100    )   +   -3.554  ;   }
        if (    tempIn >=   -90 &&  tempIn <=   -80 )   {   mvOut =     0.0323  *   (   tempIn  -   -90 )   +   -3.243  ;   }
        if (    tempIn >=   -80 &&  tempIn <=   -70 )   {   mvOut =     0.0333  *   (   tempIn  -   -80 )   +   -2.92   ;   }
        if (    tempIn >=   -70 &&  tempIn <=   -60 )   {   mvOut =     0.0344  *   (   tempIn  -   -70 )   +   -2.587  ;   }
        if (    tempIn >=   -60 &&  tempIn <=   -50 )   {   mvOut =     0.0354  *   (   tempIn  -   -60 )   +   -2.243  ;   }
        if (    tempIn >=   -50 &&  tempIn <=   -40 )   {   mvOut =     0.0362  *   (   tempIn  -   -50 )   +   -1.889  ;   }
        if (    tempIn >=   -40 &&  tempIn <=   -30 )   {   mvOut =     0.0371  *   (   tempIn  -   -40 )   +   -1.527  ;   }
        if (    tempIn >=   -30 &&  tempIn <=   -20 )   {   mvOut =     0.0378  *   (   tempIn  -   -30 )   +   -1.156  ;   }
        if (    tempIn >=   -20 &&  tempIn <=   -10 )   {   mvOut =     0.0386  *   (   tempIn  -   -20 )   +   -0.778  ;   }
        if (    tempIn >=   -10 &&  tempIn <=   0   )   {   mvOut =     0.0392  *   (   tempIn  -   -10 )   +   -0.392  ;   }
        if (    tempIn >=   0   &&  tempIn <=   10  )   {   mvOut =     0.0397  *   (   tempIn  -   0   )   +   0   ;   }
        if (    tempIn >=   10  &&  tempIn <=   20  )   {   mvOut =     0.0401  *   (   tempIn  -   10  )   +   0.397   ;   }
        if (    tempIn >=   20  &&  tempIn <=   30  )   {   mvOut =     0.0405  *   (   tempIn  -   20  )   +   0.798   ;   }
        if (    tempIn >=   30  &&  tempIn <=   40  )   {   mvOut =     0.0409  *   (   tempIn  -   30  )   +   1.203   ;   }
        if (    tempIn >=   40  &&  tempIn <=   50  )   {   mvOut =     0.0411  *   (   tempIn  -   40  )   +   1.612   ;   }
        if (    tempIn >=   50  &&  tempIn <=   60  )   {   mvOut =     0.0413  *   (   tempIn  -   50  )   +   2.023   ;   }
        if (    tempIn >=   60  &&  tempIn <=   70  )   {   mvOut =     0.0415  *   (   tempIn  -   60  )   +   2.436   ;   }
        if (    tempIn >=   70  &&  tempIn <=   80  )   {   mvOut =     0.0416  *   (   tempIn  -   70  )   +   2.851   ;   }
        if (    tempIn >=   80  &&  tempIn <=   90  )   {   mvOut =     0.0415  *   (   tempIn  -   80  )   +   3.267   ;   }
        if (    tempIn >=   90  &&  tempIn <=   100 )   {   mvOut =     0.0414  *   (   tempIn  -   90  )   +   3.682   ;   }
        if (    tempIn >=   100 &&  tempIn <=   110 )   {   mvOut =     0.0413  *   (   tempIn  -   100 )   +   4.096   ;   }
        if (    tempIn >=   110 &&  tempIn <=   120 )   {   mvOut =     0.0411  *   (   tempIn  -   110 )   +   4.509   ;   }
        if (    tempIn >=   120 &&  tempIn <=   130 )   {   mvOut =     0.0408  *   (   tempIn  -   120 )   +   4.92    ;   }
        if (    tempIn >=   130 &&  tempIn <=   140 )   {   mvOut =     0.0407  *   (   tempIn  -   130 )   +   5.328   ;   }
        if (    tempIn >=   140 &&  tempIn <=   150 )   {   mvOut =     0.0403  *   (   tempIn  -   140 )   +   5.735   ;   }
        if (    tempIn >=   150 &&  tempIn <=   160 )   {   mvOut =     0.0402  *   (   tempIn  -   150 )   +   6.138   ;   }
        if (    tempIn >=   160 &&  tempIn <=   170 )   {   mvOut =     0.0401  *   (   tempIn  -   160 )   +   6.54    ;   }
        if (    tempIn >=   170 &&  tempIn <=   180 )   {   mvOut =     0.0399  *   (   tempIn  -   170 )   +   6.941   ;   }
        if (    tempIn >=   180 &&  tempIn <=   190 )   {   mvOut =     0.0399  *   (   tempIn  -   180 )   +   7.34    ;   }
        if (    tempIn >=   190 &&  tempIn <=   200 )   {   mvOut =     0.0399  *   (   tempIn  -   190 )   +   7.739   ;   }
        if (    tempIn >=   200 &&  tempIn <=   210 )   {   mvOut =     0.0401  *   (   tempIn  -   200 )   +   8.138   ;   }
        if (    tempIn >=   210 &&  tempIn <=   220 )   {   mvOut =     0.0401  *   (   tempIn  -   210 )   +   8.539   ;   }
        if (    tempIn >=   220 &&  tempIn <=   230 )   {   mvOut =     0.0403000000000001  *   (   tempIn  -   220 )   +   8.94    ;   }
        if (    tempIn >=   230 &&  tempIn <=   240 )   {   mvOut =     0.0404  *   (   tempIn  -   230 )   +   9.343   ;   }
        if (    tempIn >=   240 &&  tempIn <=   250 )   {   mvOut =     0.0406000000000001  *   (   tempIn  -   240 )   +   9.747   ;   }
        if (    tempIn >=   250 &&  tempIn <=   260 )   {   mvOut =     0.0408  *   (   tempIn  -   250 )   +   10.153  ;   }
        if (    tempIn >=   260 &&  tempIn <=   270 )   {   mvOut =     0.041   *   (   tempIn  -   260 )   +   10.561  ;   }
        if (    tempIn >=   270 &&  tempIn <=   280 )   {   mvOut =     0.0411  *   (   tempIn  -   270 )   +   10.971  ;   }
        if (    tempIn >=   280 &&  tempIn <=   290 )   {   mvOut =     0.0413  *   (   tempIn  -   280 )   +   11.382  ;   }
        if (    tempIn >=   290 &&  tempIn <=   300 )   {   mvOut =     0.0414  *   (   tempIn  -   290 )   +   11.795  ;   }
        if (    tempIn >=   300 &&  tempIn <=   310 )   {   mvOut =     0.0415000000000001  *   (   tempIn  -   300 )   +   12.209  ;   }
        if (    tempIn >=   310 &&  tempIn <=   320 )   {   mvOut =     0.0415999999999999  *   (   tempIn  -   310 )   +   12.624  ;   }
        if (    tempIn >=   320 &&  tempIn <=   330 )   {   mvOut =     0.0417000000000002  *   (   tempIn  -   320 )   +   13.04   ;   }
        if (    tempIn >=   330 &&  tempIn <=   340 )   {   mvOut =     0.0417  *   (   tempIn  -   330 )   +   13.457  ;   }
        if (    tempIn >=   340 &&  tempIn <=   350 )   {   mvOut =     0.0418999999999999  *   (   tempIn  -   340 )   +   13.874  ;   }
        if (    tempIn >=   350 &&  tempIn <=   360 )   {   mvOut =     0.042   *   (   tempIn  -   350 )   +   14.293  ;   }
        if (    tempIn >=   360 &&  tempIn <=   370 )   {   mvOut =     0.042   *   (   tempIn  -   360 )   +   14.713  ;   }
        if (    tempIn >=   370 &&  tempIn <=   380 )   {   mvOut =     0.0421000000000001  *   (   tempIn  -   370 )   +   15.133  ;   }
        if (    tempIn >=   380 &&  tempIn <=   390 )   {   mvOut =     0.0420999999999999  *   (   tempIn  -   380 )   +   15.554  ;   }
        if (    tempIn >=   390 &&  tempIn <=   400 )   {   mvOut =     0.0421999999999999  *   (   tempIn  -   390 )   +   15.975  ;   }
        if (    tempIn >=   400 &&  tempIn <=   410 )   {   mvOut =     0.0423000000000002  *   (   tempIn  -   400 )   +   16.397  ;   }
        if (    tempIn >=   410 &&  tempIn <=   420 )   {   mvOut =     0.0422999999999998  *   (   tempIn  -   410 )   +   16.82   ;   }
        if (    tempIn >=   420 &&  tempIn <=   430 )   {   mvOut =     0.0424000000000003  *   (   tempIn  -   420 )   +   17.243  ;   }
        if (    tempIn >=   430 &&  tempIn <=   440 )   {   mvOut =     0.0424  *   (   tempIn  -   430 )   +   17.667  ;   }
        if (    tempIn >=   440 &&  tempIn <=   450 )   {   mvOut =     0.0424999999999997  *   (   tempIn  -   440 )   +   18.091  ;   }
        if (    tempIn >=   450 &&  tempIn <=   460 )   {   mvOut =     0.0425000000000001  *   (   tempIn  -   450 )   +   18.516  ;   }
        if (    tempIn >=   460 &&  tempIn <=   470 )   {   mvOut =     0.0425000000000001  *   (   tempIn  -   460 )   +   18.941  ;   }
        if (    tempIn >=   470 &&  tempIn <=   480 )   {   mvOut =     0.0426000000000002  *   (   tempIn  -   470 )   +   19.366  ;   }
        if (    tempIn >=   480 &&  tempIn <=   490 )   {   mvOut =     0.0425999999999998  *   (   tempIn  -   480 )   +   19.792  ;   }
        if (    tempIn >=   490 &&  tempIn <=   500 )   {   mvOut =     0.0425999999999998  *   (   tempIn  -   490 )   +   20.218  ;   }
        if (    tempIn >=   500 &&  tempIn <=   510 )   {   mvOut =     0.0427000000000003  *   (   tempIn  -   500 )   +   20.644  ;   }
        if (    tempIn >=   510 &&  tempIn <=   520 )   {   mvOut =     0.0425999999999998  *   (   tempIn  -   510 )   +   21.071  ;   }
        if (    tempIn >=   520 &&  tempIn <=   530 )   {   mvOut =     0.0427  *   (   tempIn  -   520 )   +   21.497  ;   }
        if (    tempIn >=   530 &&  tempIn <=   540 )   {   mvOut =     0.0426000000000002  *   (   tempIn  -   530 )   +   21.924  ;   }
        if (    tempIn >=   540 &&  tempIn <=   550 )   {   mvOut =     0.0425999999999998  *   (   tempIn  -   540 )   +   22.35   ;   }
        if (    tempIn >=   550 &&  tempIn <=   560 )   {   mvOut =     0.0427  *   (   tempIn  -   550 )   +   22.776  ;   }
        if (    tempIn >=   560 &&  tempIn <=   570 )   {   mvOut =     0.0426000000000002  *   (   tempIn  -   560 )   +   23.203  ;   }
        if (    tempIn >=   570 &&  tempIn <=   580 )   {   mvOut =     0.0425999999999998  *   (   tempIn  -   570 )   +   23.629  ;   }
        if (    tempIn >=   580 &&  tempIn <=   590 )   {   mvOut =     0.0425000000000001  *   (   tempIn  -   580 )   +   24.055  ;   }
        if (    tempIn >=   590 &&  tempIn <=   600 )   {   mvOut =     0.0425000000000001  *   (   tempIn  -   590 )   +   24.48   ;   }
        if (    tempIn >=   600 &&  tempIn <=   610 )   {   mvOut =     0.0424999999999997  *   (   tempIn  -   600 )   +   24.905  ;   }
        if (    tempIn >=   610 &&  tempIn <=   620 )   {   mvOut =     0.0425000000000001  *   (   tempIn  -   610 )   +   25.33   ;   }
        if (    tempIn >=   620 &&  tempIn <=   630 )   {   mvOut =     0.0424  *   (   tempIn  -   620 )   +   25.755  ;   }
        if (    tempIn >=   630 &&  tempIn <=   640 )   {   mvOut =     0.0423000000000002  *   (   tempIn  -   630 )   +   26.179  ;   }
        if (    tempIn >=   640 &&  tempIn <=   650 )   {   mvOut =     0.0422999999999998  *   (   tempIn  -   640 )   +   26.602  ;   }
        if (    tempIn >=   650 &&  tempIn <=   660 )   {   mvOut =     0.0422000000000001  *   (   tempIn  -   650 )   +   27.025  ;   }
        if (    tempIn >=   660 &&  tempIn <=   670 )   {   mvOut =     0.0422000000000001  *   (   tempIn  -   660 )   +   27.447  ;   }
        if (    tempIn >=   670 &&  tempIn <=   680 )   {   mvOut =     0.0420000000000002  *   (   tempIn  -   670 )   +   27.869  ;   }
        if (    tempIn >=   680 &&  tempIn <=   690 )   {   mvOut =     0.0420999999999999  *   (   tempIn  -   680 )   +   28.289  ;   }
        if (    tempIn >=   690 &&  tempIn <=   700 )   {   mvOut =     0.0419000000000001  *   (   tempIn  -   690 )   +   28.71   ;   }
        if (    tempIn >=   700 &&  tempIn <=   710 )   {   mvOut =     0.0418999999999997  *   (   tempIn  -   700 )   +   29.129  ;   }
        if (    tempIn >=   710 &&  tempIn <=   720 )   {   mvOut =     0.0417000000000002  *   (   tempIn  -   710 )   +   29.548  ;   }
        if (    tempIn >=   720 &&  tempIn <=   730 )   {   mvOut =     0.0417000000000002  *   (   tempIn  -   720 )   +   29.965  ;   }
        if (    tempIn >=   730 &&  tempIn <=   740 )   {   mvOut =     0.0415999999999997  *   (   tempIn  -   730 )   +   30.382  ;   }
        if (    tempIn >=   740 &&  tempIn <=   750 )   {   mvOut =     0.0415000000000003  *   (   tempIn  -   740 )   +   30.798  ;   }
        if (    tempIn >=   750 &&  tempIn <=   760 )   {   mvOut =     0.0414999999999999  *   (   tempIn  -   750 )   +   31.213  ;   }
        if (    tempIn >=   760 &&  tempIn <=   770 )   {   mvOut =     0.0412999999999997  *   (   tempIn  -   760 )   +   31.628  ;   }
        if (    tempIn >=   770 &&  tempIn <=   780 )   {   mvOut =     0.0412000000000006  *   (   tempIn  -   770 )   +   32.041  ;   }
        if (    tempIn >=   780 &&  tempIn <=   790 )   {   mvOut =     0.0411999999999999  *   (   tempIn  -   780 )   +   32.453  ;   }
        if (    tempIn >=   790 &&  tempIn <=   800 )   {   mvOut =     0.0409999999999997  *   (   tempIn  -   790 )   +   32.865  ;   }
        if (    tempIn >=   800 &&  tempIn <=   810 )   {   mvOut =     0.0410000000000004  *   (   tempIn  -   800 )   +   33.275  ;   }
        if (    tempIn >=   810 &&  tempIn <=   820 )   {   mvOut =     0.0408000000000001  *   (   tempIn  -   810 )   +   33.685  ;   }
        if (    tempIn >=   820 &&  tempIn <=   830 )   {   mvOut =     0.0407999999999994  *   (   tempIn  -   820 )   +   34.093  ;   }
        if (    tempIn >=   830 &&  tempIn <=   840 )   {   mvOut =     0.0407000000000004  *   (   tempIn  -   830 )   +   34.501  ;   }
        if (    tempIn >=   840 &&  tempIn <=   850 )   {   mvOut =     0.0405000000000001  *   (   tempIn  -   840 )   +   34.908  ;   }
        if (    tempIn >=   850 &&  tempIn <=   860 )   {   mvOut =     0.0405000000000001  *   (   tempIn  -   850 )   +   35.313  ;   }
        if (    tempIn >=   860 &&  tempIn <=   870 )   {   mvOut =     0.0402999999999999  *   (   tempIn  -   860 )   +   35.718  ;   }
        if (    tempIn >=   870 &&  tempIn <=   880 )   {   mvOut =     0.0402999999999999  *   (   tempIn  -   870 )   +   36.121  ;   }
        if (    tempIn >=   880 &&  tempIn <=   890 )   {   mvOut =     0.0400999999999996  *   (   tempIn  -   880 )   +   36.524  ;   }
        if (    tempIn >=   890 &&  tempIn <=   900 )   {   mvOut =     0.0401000000000003  *   (   tempIn  -   890 )   +   36.925  ;   }
        if (    tempIn >=   900 &&  tempIn <=   910 )   {   mvOut =     0.0399000000000001  *   (   tempIn  -   900 )   +   37.326  ;   }
        if (    tempIn >=   910 &&  tempIn <=   920 )   {   mvOut =     0.0399000000000001  *   (   tempIn  -   910 )   +   37.725  ;   }
        if (    tempIn >=   920 &&  tempIn <=   930 )   {   mvOut =     0.0397999999999996  *   (   tempIn  -   920 )   +   38.124  ;   }
        if (    tempIn >=   930 &&  tempIn <=   940 )   {   mvOut =     0.0396000000000001  *   (   tempIn  -   930 )   +   38.522  ;   }
        if (    tempIn >=   940 &&  tempIn <=   950 )   {   mvOut =     0.0396000000000001  *   (   tempIn  -   940 )   +   38.918  ;   }
        if (    tempIn >=   950 &&  tempIn <=   960 )   {   mvOut =     0.0393999999999998  *   (   tempIn  -   950 )   +   39.314  ;   }
        if (    tempIn >=   960 &&  tempIn <=   970 )   {   mvOut =     0.0393000000000001  *   (   tempIn  -   960 )   +   39.708  ;   }
        if (    tempIn >=   970 &&  tempIn <=   980 )   {   mvOut =     0.0393000000000001  *   (   tempIn  -   970 )   +   40.101  ;   }
        if (    tempIn >=   980 &&  tempIn <=   990 )   {   mvOut =     0.0390999999999998  *   (   tempIn  -   980 )   +   40.494  ;   }
        if (    tempIn >=   990 &&  tempIn <=   1000    )   {   mvOut =     0.0391000000000005  *   (   tempIn  -   990 )   +   40.885  ;   }
        if (    tempIn >=   1000    &&  tempIn <=   1010    )   {   mvOut =     0.0388999999999996  *   (   tempIn  -   1000    )   +   41.276  ;   }
        if (    tempIn >=   1010    &&  tempIn <=   1020    )   {   mvOut =     0.0387999999999998  *   (   tempIn  -   1010    )   +   41.665  ;   }
        if (    tempIn >=   1020    &&  tempIn <=   1030    )   {   mvOut =     0.0387  *   (   tempIn  -   1020    )   +   42.053  ;   }
        if (    tempIn >=   1030    &&  tempIn <=   1040    )   {   mvOut =     0.0386000000000003  *   (   tempIn  -   1030    )   +   42.44   ;   }
        if (    tempIn >=   1040    &&  tempIn <=   1050    )   {   mvOut =     0.0384999999999998  *   (   tempIn  -   1040    )   +   42.826  ;   }
        if (    tempIn >=   1050    &&  tempIn <=   1060    )   {   mvOut =     0.0384  *   (   tempIn  -   1050    )   +   43.211  ;   }
        if (    tempIn >=   1060    &&  tempIn <=   1070    )   {   mvOut =     0.0383000000000003  *   (   tempIn  -   1060    )   +   43.595  ;   }
        if (    tempIn >=   1070    &&  tempIn <=   1080    )   {   mvOut =     0.0381  *   (   tempIn  -   1070    )   +   43.978  ;   }
        if (    tempIn >=   1080    &&  tempIn <=   1090    )   {   mvOut =     0.0381  *   (   tempIn  -   1080    )   +   44.359  ;   }
        if (    tempIn >=   1090    &&  tempIn <=   1100    )   {   mvOut =     0.0378999999999998  *   (   tempIn  -   1090    )   +   44.74   ;   }
        if (    tempIn >=   1100    &&  tempIn <=   1110    )   {   mvOut =     0.0378  *   (   tempIn  -   1100    )   +   45.119  ;   }
        if (    tempIn >=   1110    &&  tempIn <=   1120    )   {   mvOut =     0.0375999999999998  *   (   tempIn  -   1110    )   +   45.497  ;   }
        if (    tempIn >=   1120    &&  tempIn <=   1130    )   {   mvOut =     0.0376000000000005  *   (   tempIn  -   1120    )   +   45.873  ;   }
        if (    tempIn >=   1130    &&  tempIn <=   1140    )   {   mvOut =     0.0373999999999995  *   (   tempIn  -   1130    )   +   46.249  ;   }
        if (    tempIn >=   1140    &&  tempIn <=   1150    )   {   mvOut =     0.0372  *   (   tempIn  -   1140    )   +   46.623  ;   }
        if (    tempIn >=   1150    &&  tempIn <=   1160    )   {   mvOut =     0.0372  *   (   tempIn  -   1150    )   +   46.995  ;   }
        if (    tempIn >=   1160    &&  tempIn <=   1170    )   {   mvOut =     0.0370000000000005  *   (   tempIn  -   1160    )   +   47.367  ;   }
        if (    tempIn >=   1170    &&  tempIn <=   1180    )   {   mvOut =     0.0367999999999995  *   (   tempIn  -   1170    )   +   47.737  ;   }
        if (    tempIn >=   1180    &&  tempIn <=   1190    )   {   mvOut =     0.0368000000000002  *   (   tempIn  -   1180    )   +   48.105  ;   }
        if (    tempIn >=   1190    &&  tempIn <=   1200    )   {   mvOut =     0.0365000000000002  *   (   tempIn  -   1190    )   +   48.473  ;   }
        if (    tempIn >=   1200    &&  tempIn <=   1210    )   {   mvOut =     0.0363999999999997  *   (   tempIn  -   1200    )   +   48.838  ;   }
        if (    tempIn >=   1210    &&  tempIn <=   1220    )   {   mvOut =     0.0363  *   (   tempIn  -   1210    )   +   49.202  ;   }
        if (    tempIn >=   1220    &&  tempIn <=   1230    )   {   mvOut =     0.0361000000000004  *   (   tempIn  -   1220    )   +   49.565  ;   }
        if (    tempIn >=   1230    &&  tempIn <=   1240    )   {   mvOut =     0.0359999999999999  *   (   tempIn  -   1230    )   +   49.926  ;   }
        if (    tempIn >=   1240    &&  tempIn <=   1250    )   {   mvOut =     0.0357999999999997  *   (   tempIn  -   1240    )   +   50.286  ;   }
        if (    tempIn >=   1250    &&  tempIn <=   1260    )   {   mvOut =     0.0356000000000002  *   (   tempIn  -   1250    )   +   50.644  ;   }
        if (    tempIn >=   1260    &&  tempIn <=   1270    )   {   mvOut =     0.0354999999999997  *   (   tempIn  -   1260    )   +   51  ;   }
        if (    tempIn >=   1270    &&  tempIn <=   1280    )   {   mvOut =     0.0353000000000001  *   (   tempIn  -   1270    )   +   51.355  ;   }
        if (    tempIn >=   1280    &&  tempIn <=   1290    )   {   mvOut =     0.0352000000000004  *   (   tempIn  -   1280    )   +   51.708  ;   }
        if (    tempIn >=   1290    &&  tempIn <=   1300    )   {   mvOut =     0.0349999999999994  *   (   tempIn  -   1290    )   +   52.06   ;   }
        if (    tempIn >=   1300    &&  tempIn <=   1310    )   {   mvOut =     0.0349000000000004  *   (   tempIn  -   1300    )   +   52.41   ;   }
        if (    tempIn >=   1310    &&  tempIn <=   1320    )   {   mvOut =     0.0347000000000001  *   (   tempIn  -   1310    )   +   52.759  ;   }
        if (    tempIn >=   1320    &&  tempIn <=   1330    )   {   mvOut =     0.0344999999999999  *   (   tempIn  -   1320    )   +   53.106  ;   }
        if (    tempIn >=   1330    &&  tempIn <=   1340    )   {   mvOut =     0.0344000000000001  *   (   tempIn  -   1330    )   +   53.451  ;   }
        if (    tempIn >=   1340    &&  tempIn <=   1350    )   {   mvOut =     0.0342999999999996  *   (   tempIn  -   1340    )   +   53.795  ;   }
        if (    tempIn >=   1350    &&  tempIn <=   1360    )   {   mvOut =     0.0341000000000001  *   (   tempIn  -   1350    )   +   54.138  ;   }
        if (    tempIn >=   1360    &&  tempIn <=   1370    )   {   mvOut =     0.0340000000000003  *   (   tempIn  -   1360    )   +   54.479  ;   }
        if (    tempIn >=   1370    &&  tempIn <=   1382    )   {   mvOut =     0.00558333333333335 *   (   tempIn  -   1370    )   +   54.819  ;   }


        mvInput = mvOut + thermoMv;



        if (    mvInput >=  -6.458  &&  mvInput <=  -6.441  )   {   tempOut =   588.235294117635    *   (   mvInput -   -6.458  )   +   -270    ;   }
        if (    mvInput >=  -6.441  &&  mvInput <=  -6.404  )   {   tempOut =   270.270270270271    *   (   mvInput -   -6.441  )   +   -260    ;   }
        if (    mvInput >=  -6.404  &&  mvInput <=  -6.344  )   {   tempOut =   166.666666666668    *   (   mvInput -   -6.404  )   +   -250    ;   }
        if (    mvInput >=  -6.344  &&  mvInput <=  -6.262  )   {   tempOut =   121.951219512194    *   (   mvInput -   -6.344  )   +   -240    ;   }
        if (    mvInput >=  -6.262  &&  mvInput <=  -6.158  )   {   tempOut =   96.1538461538469    *   (   mvInput -   -6.262  )   +   -230    ;   }
        if (    mvInput >=  -6.158  &&  mvInput <=  -6.035  )   {   tempOut =   81.3008130081299    *   (   mvInput -   -6.158  )   +   -220    ;   }
        if (    mvInput >=  -6.035  &&  mvInput <=  -5.891  )   {   tempOut =   69.4444444444444    *   (   mvInput -   -6.035  )   +   -210    ;   }
        if (    mvInput >=  -5.891  &&  mvInput <=  -5.73   )   {   tempOut =   62.1118012422362    *   (   mvInput -   -5.891  )   +   -200    ;   }
        if (    mvInput >=  -5.73   &&  mvInput <=  -5.55   )   {   tempOut =   55.5555555555554    *   (   mvInput -   -5.73   )   +   -190    ;   }
        if (    mvInput >=  -5.55   &&  mvInput <=  -5.354  )   {   tempOut =   51.0204081632654    *   (   mvInput -   -5.55   )   +   -180    ;   }
        if (    mvInput >=  -5.354  &&  mvInput <=  -5.141  )   {   tempOut =   46.9483568075117    *   (   mvInput -   -5.354  )   +   -170    ;   }
        if (    mvInput >=  -5.141  &&  mvInput <=  -4.913  )   {   tempOut =   43.8596491228071    *   (   mvInput -   -5.141  )   +   -160    ;   }
        if (    mvInput >=  -4.913  &&  mvInput <=  -4.669  )   {   tempOut =   40.9836065573769    *   (   mvInput -   -4.913  )   +   -150    ;   }
        if (    mvInput >=  -4.669  &&  mvInput <=  -4.411  )   {   tempOut =   38.7596899224806    *   (   mvInput -   -4.669  )   +   -140    ;   }
        if (    mvInput >=  -4.411  &&  mvInput <=  -4.138  )   {   tempOut =   36.6300366300367    *   (   mvInput -   -4.411  )   +   -130    ;   }
        if (    mvInput >=  -4.138  &&  mvInput <=  -3.852  )   {   tempOut =   34.965034965035     *   (   mvInput -   -4.138  )   +   -120    ;   }
        if (    mvInput >=  -3.852  &&  mvInput <=  -3.554  )   {   tempOut =   33.5570469798658    *   (   mvInput -   -3.852  )   +   -110    ;   }
        if (    mvInput >=  -3.554  &&  mvInput <=  -3.243  )   {   tempOut =   32.1543408360129    *   (   mvInput -   -3.554  )   +   -100    ;   }
        if (    mvInput >=  -3.243  &&  mvInput <=  -2.92   )   {   tempOut =   30.9597523219814    *   (   mvInput -   -3.243  )   +   -90     ;   }
        if (    mvInput >=  -2.92   &&  mvInput <=  -2.587  )   {   tempOut =   30.0300300300301    *   (   mvInput -   -2.92   )   +   -80     ;   }
        if (    mvInput >=  -2.587  &&  mvInput <=  -2.243  )   {   tempOut =   29.0697674418604    *   (   mvInput -   -2.587  )   +   -70     ;   }
        if (    mvInput >=  -2.243  &&  mvInput <=  -1.889  )   {   tempOut =   28.2485875706215    *   (   mvInput -   -2.243  )   +   -60     ;   }
        if (    mvInput >=  -1.889  &&  mvInput <=  -1.527  )   {   tempOut =   27.6243093922652    *   (   mvInput -   -1.889  )   +   -50     ;   }
        if (    mvInput >=  -1.527  &&  mvInput <=  -1.156  )   {   tempOut =   26.9541778975741    *   (   mvInput -   -1.527  )   +   -40     ;   }
        if (    mvInput >=  -1.156  &&  mvInput <=  -0.778  )   {   tempOut =   26.4550264550265    *   (   mvInput -   -1.156  )   +   -30     ;   }
        if (    mvInput >=  -0.778  &&  mvInput <=  -0.392  )   {   tempOut =   25.9067357512953    *   (   mvInput -   -0.778  )   +   -20     ;   }
        if (    mvInput >=  -0.392  &&  mvInput <=  0       )   {   tempOut =   25.5102040816327    *   (   mvInput -   -0.392  )   +   -10     ;   }
        if (    mvInput >=  0       &&  mvInput <=  0.397   )   {   tempOut =   25.1889168765743    *   (   mvInput -   0       )   +   0       ;   }
        if (    mvInput >=  0.397   &&  mvInput <=  0.798   )   {   tempOut =   24.9376558603491    *   (   mvInput -   0.397   )   +   10      ;   }
        if (    mvInput >=  0.798   &&  mvInput <=  1.203   )   {   tempOut =   24.6913580246914    *   (   mvInput -   0.798   )   +   20      ;   }
        if (    mvInput >=  1.203   &&  mvInput <=  1.612   )   {   tempOut =   24.4498777506112    *   (   mvInput -   1.203   )   +   30      ;   }
        if (    mvInput >=  1.612   &&  mvInput <=  2.023   )   {   tempOut =   24.330900243309     *   (   mvInput -   1.612   )   +   40      ;   }
        if (    mvInput >=  2.023   &&  mvInput <=  2.436   )   {   tempOut =   24.2130750605327    *   (   mvInput -   2.023   )   +   50      ;   }
        if (    mvInput >=  2.436   &&  mvInput <=  2.851   )   {   tempOut =   24.0963855421687    *   (   mvInput -   2.436   )   +   60      ;   }
        if (    mvInput >=  2.851   &&  mvInput <=  3.267   )   {   tempOut =   24.0384615384615    *   (   mvInput -   2.851   )   +   70      ;   }
        if (    mvInput >=  3.267   &&  mvInput <=  3.682   )   {   tempOut =   24.0963855421687    *   (   mvInput -   3.267   )   +   80      ;   }
        if (    mvInput >=  3.682   &&  mvInput <=  4.096   )   {   tempOut =   24.1545893719807    *   (   mvInput -   3.682   )   +   90      ;   }
        if (    mvInput >=  4.096   &&  mvInput <=  4.509   )   {   tempOut =   24.2130750605327    *   (   mvInput -   4.096   )   +   100     ;   }
        if (    mvInput >=  4.509   &&  mvInput <=  4.92    )   {   tempOut =   24.330900243309 *   (   mvInput -   4.509       )   +   110 ;   }
        if (    mvInput >=  4.92    &&  mvInput <=  5.328   )   {   tempOut =   24.5098039215686    *   (   mvInput -   4.92    )   +   120 ;   }
        if (    mvInput >=  5.328   &&  mvInput <=  5.735   )   {   tempOut =   24.5700245700246    *   (   mvInput -   5.328   )   +   130 ;   }
        if (    mvInput >=  5.735   &&  mvInput <=  6.138   )   {   tempOut =   24.8138957816377    *   (   mvInput -   5.735   )   +   140 ;   }
        if (    mvInput >=  6.138   &&  mvInput <=  6.54    )   {   tempOut =   24.8756218905473    *   (   mvInput -   6.138   )   +   150 ;   }
        if (    mvInput >=  6.54    &&  mvInput <=  6.941   )   {   tempOut =   24.9376558603491    *   (   mvInput -   6.54    )   +   160 ;   }
        if (    mvInput >=  6.941   &&  mvInput <=  7.34    )   {   tempOut =   25.062656641604 *   (   mvInput -   6.941   )   +   170 ;   }
        if (    mvInput >=  7.34    &&  mvInput <=  7.739   )   {   tempOut =   25.062656641604 *   (   mvInput -   7.34    )   +   180 ;   }
        if (    mvInput >=  7.739   &&  mvInput <=  8.138   )   {   tempOut =   25.062656641604 *   (   mvInput -   7.739   )   +   190 ;   }
        if (    mvInput >=  8.138   &&  mvInput <=  8.539   )   {   tempOut =   24.9376558603491    *   (   mvInput -   8.138   )   +   200 ;   }
        if (    mvInput >=  8.539   &&  mvInput <=  8.94    )   {   tempOut =   24.9376558603491    *   (   mvInput -   8.539   )   +   210 ;   }
        if (    mvInput >=  8.94    &&  mvInput <=  9.343   )   {   tempOut =   24.8138957816377    *   (   mvInput -   8.94    )   +   220 ;   }
        if (    mvInput >=  9.343   &&  mvInput <=  9.747   )   {   tempOut =   24.7524752475248    *   (   mvInput -   9.343   )   +   230 ;   }
        if (    mvInput >=  9.747   &&  mvInput <=  10.153  )   {   tempOut =   24.6305418719211    *   (   mvInput -   9.747   )   +   240 ;   }
        if (    mvInput >=  10.153  &&  mvInput <=  10.561  )   {   tempOut =   24.5098039215687    *   (   mvInput -   10.153  )   +   250 ;   }
        if (    mvInput >=  10.561  &&  mvInput <=  10.971  )   {   tempOut =   24.390243902439 *   (   mvInput -   10.561  )   +   260 ;   }
        if (    mvInput >=  10.971  &&  mvInput <=  11.382  )   {   tempOut =   24.330900243309 *   (   mvInput -   10.971  )   +   270 ;   }
        if (    mvInput >=  11.382  &&  mvInput <=  11.795  )   {   tempOut =   24.2130750605327    *   (   mvInput -   11.382  )   +   280 ;   }
        if (    mvInput >=  11.795  &&  mvInput <=  12.209  )   {   tempOut =   24.1545893719807    *   (   mvInput -   11.795  )   +   290 ;   }
        if (    mvInput >=  12.209  &&  mvInput <=  12.624  )   {   tempOut =   24.0963855421686    *   (   mvInput -   12.209  )   +   300 ;   }
        if (    mvInput >=  12.624  &&  mvInput <=  13.04   )   {   tempOut =   24.0384615384616    *   (   mvInput -   12.624  )   +   310 ;   }
        if (    mvInput >=  13.04   &&  mvInput <=  13.457  )   {   tempOut =   23.9808153477217    *   (   mvInput -   13.04   )   +   320 ;   }
        if (    mvInput >=  13.457  &&  mvInput <=  13.874  )   {   tempOut =   23.9808153477218    *   (   mvInput -   13.457  )   +   330 ;   }
        if (    mvInput >=  13.874  &&  mvInput <=  14.293  )   {   tempOut =   23.8663484486874    *   (   mvInput -   13.874  )   +   340 ;   }
        if (    mvInput >=  14.293  &&  mvInput <=  14.713  )   {   tempOut =   23.8095238095238    *   (   mvInput -   14.293  )   +   350 ;   }
        if (    mvInput >=  14.713  &&  mvInput <=  15.133  )   {   tempOut =   23.8095238095238    *   (   mvInput -   14.713  )   +   360 ;   }
        if (    mvInput >=  15.133  &&  mvInput <=  15.554  )   {   tempOut =   23.7529691211401    *   (   mvInput -   15.133  )   +   370 ;   }
        if (    mvInput >=  15.554  &&  mvInput <=  15.975  )   {   tempOut =   23.7529691211402    *   (   mvInput -   15.554  )   +   380 ;   }
        if (    mvInput >=  15.975  &&  mvInput <=  16.397  )   {   tempOut =   23.696682464455 *   (   mvInput -   15.975  )   +   390 ;   }
        if (    mvInput >=  16.397  &&  mvInput <=  16.82   )   {   tempOut =   23.6406619385342    *   (   mvInput -   16.397  )   +   400 ;   }
        if (    mvInput >=  16.82   &&  mvInput <=  17.243  )   {   tempOut =   23.6406619385344    *   (   mvInput -   16.82   )   +   410 ;   }
        if (    mvInput >=  17.243  &&  mvInput <=  17.667  )   {   tempOut =   23.5849056603772    *   (   mvInput -   17.243  )   +   420 ;   }
        if (    mvInput >=  17.667  &&  mvInput <=  18.091  )   {   tempOut =   23.5849056603774    *   (   mvInput -   17.667  )   +   430 ;   }
        if (    mvInput >=  18.091  &&  mvInput <=  18.516  )   {   tempOut =   23.529411764706 *   (   mvInput -   18.091  )   +   440 ;   }
        if (    mvInput >=  18.516  &&  mvInput <=  18.941  )   {   tempOut =   23.5294117647058    *   (   mvInput -   18.516  )   +   450 ;   }
        if (    mvInput >=  18.941  &&  mvInput <=  19.366  )   {   tempOut =   23.5294117647058    *   (   mvInput -   18.941  )   +   460 ;   }
        if (    mvInput >=  19.366  &&  mvInput <=  19.792  )   {   tempOut =   23.4741784037558    *   (   mvInput -   19.366  )   +   470 ;   }
        if (    mvInput >=  19.792  &&  mvInput <=  20.218  )   {   tempOut =   23.474178403756 *   (   mvInput -   19.792  )   +   480 ;   }
        if (    mvInput >=  20.218  &&  mvInput <=  20.644  )   {   tempOut =   23.474178403756 *   (   mvInput -   20.218  )   +   490 ;   }
        if (    mvInput >=  20.644  &&  mvInput <=  21.071  )   {   tempOut =   23.4192037470724    *   (   mvInput -   20.644  )   +   500 ;   }
        if (    mvInput >=  21.071  &&  mvInput <=  21.497  )   {   tempOut =   23.474178403756 *   (   mvInput -   21.071  )   +   510 ;   }
        if (    mvInput >=  21.497  &&  mvInput <=  21.924  )   {   tempOut =   23.4192037470726    *   (   mvInput -   21.497  )   +   520 ;   }
        if (    mvInput >=  21.924  &&  mvInput <=  22.35   )   {   tempOut =   23.4741784037558    *   (   mvInput -   21.924  )   +   530 ;   }
        if (    mvInput >=  22.35   &&  mvInput <=  22.776  )   {   tempOut =   23.474178403756 *   (   mvInput -   22.35   )   +   540 ;   }
        if (    mvInput >=  22.776  &&  mvInput <=  23.203  )   {   tempOut =   23.4192037470726    *   (   mvInput -   22.776  )   +   550 ;   }
        if (    mvInput >=  23.203  &&  mvInput <=  23.629  )   {   tempOut =   23.4741784037558    *   (   mvInput -   23.203  )   +   560 ;   }
        if (    mvInput >=  23.629  &&  mvInput <=  24.055  )   {   tempOut =   23.474178403756 *   (   mvInput -   23.629  )   +   570 ;   }
        if (    mvInput >=  24.055  &&  mvInput <=  24.48   )   {   tempOut =   23.5294117647058    *   (   mvInput -   24.055  )   +   580 ;   }
        if (    mvInput >=  24.48   &&  mvInput <=  24.905  )   {   tempOut =   23.5294117647058    *   (   mvInput -   24.48   )   +   590 ;   }
        if (    mvInput >=  24.905  &&  mvInput <=  25.33   )   {   tempOut =   23.529411764706 *   (   mvInput -   24.905  )   +   600 ;   }
        if (    mvInput >=  25.33   &&  mvInput <=  25.755  )   {   tempOut =   23.5294117647058    *   (   mvInput -   25.33   )   +   610 ;   }
        if (    mvInput >=  25.755  &&  mvInput <=  26.179  )   {   tempOut =   23.5849056603774    *   (   mvInput -   25.755  )   +   620 ;   }
        if (    mvInput >=  26.179  &&  mvInput <=  26.602  )   {   tempOut =   23.6406619385342    *   (   mvInput -   26.179  )   +   630 ;   }
        if (    mvInput >=  26.602  &&  mvInput <=  27.025  )   {   tempOut =   23.6406619385344    *   (   mvInput -   26.602  )   +   640 ;   }
        if (    mvInput >=  27.025  &&  mvInput <=  27.447  )   {   tempOut =   23.6966824644549    *   (   mvInput -   27.025  )   +   650 ;   }
        if (    mvInput >=  27.447  &&  mvInput <=  27.869  )   {   tempOut =   23.6966824644549    *   (   mvInput -   27.447  )   +   660 ;   }
        if (    mvInput >=  27.869  &&  mvInput <=  28.289  )   {   tempOut =   23.8095238095237    *   (   mvInput -   27.869  )   +   670 ;   }
        if (    mvInput >=  28.289  &&  mvInput <=  28.71   )   {   tempOut =   23.7529691211402    *   (   mvInput -   28.289  )   +   680 ;   }
        if (    mvInput >=  28.71   &&  mvInput <=  29.129  )   {   tempOut =   23.8663484486873    *   (   mvInput -   28.71   )   +   690 ;   }
        if (    mvInput >=  29.129  &&  mvInput <=  29.548  )   {   tempOut =   23.8663484486875    *   (   mvInput -   29.129  )   +   700 ;   }
        if (    mvInput >=  29.548  &&  mvInput <=  29.965  )   {   tempOut =   23.9808153477217    *   (   mvInput -   29.548  )   +   710 ;   }
        if (    mvInput >=  29.965  &&  mvInput <=  30.382  )   {   tempOut =   23.9808153477217    *   (   mvInput -   29.965  )   +   720 ;   }
        if (    mvInput >=  30.382  &&  mvInput <=  30.798  )   {   tempOut =   24.0384615384617    *   (   mvInput -   30.382  )   +   730 ;   }
        if (    mvInput >=  30.798  &&  mvInput <=  31.213  )   {   tempOut =   24.0963855421685    *   (   mvInput -   30.798  )   +   740 ;   }
        if (    mvInput >=  31.213  &&  mvInput <=  31.628  )   {   tempOut =   24.0963855421687    *   (   mvInput -   31.213  )   +   750 ;   }
        if (    mvInput >=  31.628  &&  mvInput <=  32.041  )   {   tempOut =   24.2130750605329    *   (   mvInput -   31.628  )   +   760 ;   }
        if (    mvInput >=  32.041  &&  mvInput <=  32.453  )   {   tempOut =   24.2718446601938    *   (   mvInput -   32.041  )   +   770 ;   }
        if (    mvInput >=  32.453  &&  mvInput <=  32.865  )   {   tempOut =   24.2718446601942    *   (   mvInput -   32.453  )   +   780 ;   }
        if (    mvInput >=  32.865  &&  mvInput <=  33.275  )   {   tempOut =   24.3902439024392    *   (   mvInput -   32.865  )   +   790 ;   }
        if (    mvInput >=  33.275  &&  mvInput <=  33.685  )   {   tempOut =   24.3902439024388    *   (   mvInput -   33.275  )   +   800 ;   }
        if (    mvInput >=  33.685  &&  mvInput <=  34.093  )   {   tempOut =   24.5098039215685    *   (   mvInput -   33.685  )   +   810 ;   }
        if (    mvInput >=  34.093  &&  mvInput <=  34.501  )   {   tempOut =   24.509803921569 *   (   mvInput -   34.093  )   +   820 ;   }
        if (    mvInput >=  34.501  &&  mvInput <=  34.908  )   {   tempOut =   24.5700245700244    *   (   mvInput -   34.501  )   +   830 ;   }
        if (    mvInput >=  34.908  &&  mvInput <=  35.313  )   {   tempOut =   24.6913580246913    *   (   mvInput -   34.908  )   +   840 ;   }
        if (    mvInput >=  35.313  &&  mvInput <=  35.718  )   {   tempOut =   24.6913580246913    *   (   mvInput -   35.313  )   +   850 ;   }
        if (    mvInput >=  35.718  &&  mvInput <=  36.121  )   {   tempOut =   24.8138957816378    *   (   mvInput -   35.718  )   +   860 ;   }
        if (    mvInput >=  36.121  &&  mvInput <=  36.524  )   {   tempOut =   24.8138957816378    *   (   mvInput -   36.121  )   +   870 ;   }
        if (    mvInput >=  36.524  &&  mvInput <=  36.925  )   {   tempOut =   24.9376558603494    *   (   mvInput -   36.524  )   +   880 ;   }
        if (    mvInput >=  36.925  &&  mvInput <=  37.326  )   {   tempOut =   24.9376558603489    *   (   mvInput -   36.925  )   +   890 ;   }
        if (    mvInput >=  37.326  &&  mvInput <=  37.725  )   {   tempOut =   25.062656641604 *   (   mvInput -   37.326  )   +   900 ;   }
        if (    mvInput >=  37.725  &&  mvInput <=  38.124  )   {   tempOut =   25.062656641604 *   (   mvInput -   37.725  )   +   910 ;   }
        if (    mvInput >=  38.124  &&  mvInput <=  38.522  )   {   tempOut =   25.1256281407038    *   (   mvInput -   38.124  )   +   920 ;   }
        if (    mvInput >=  38.522  &&  mvInput <=  38.918  )   {   tempOut =   25.2525252525252    *   (   mvInput -   38.522  )   +   930 ;   }
        if (    mvInput >=  38.918  &&  mvInput <=  39.314  )   {   tempOut =   25.2525252525252    *   (   mvInput -   38.918  )   +   940 ;   }
        if (    mvInput >=  39.314  &&  mvInput <=  39.708  )   {   tempOut =   25.3807106598986    *   (   mvInput -   39.314  )   +   950 ;   }
        if (    mvInput >=  39.708  &&  mvInput <=  40.101  )   {   tempOut =   25.4452926208651    *   (   mvInput -   39.708  )   +   960 ;   }
        if (    mvInput >=  40.101  &&  mvInput <=  40.494  )   {   tempOut =   25.4452926208651    *   (   mvInput -   40.101  )   +   970 ;   }
        if (    mvInput >=  40.494  &&  mvInput <=  40.885  )   {   tempOut =   25.5754475703326    *   (   mvInput -   40.494  )   +   980 ;   }
        if (    mvInput >=  40.885  &&  mvInput <=  41.276  )   {   tempOut =   25.5754475703321    *   (   mvInput -   40.885  )   +   990 ;   }
        if (    mvInput >=  41.276  &&  mvInput <=  41.665  )   {   tempOut =   25.7069408740363    *   (   mvInput -   41.276  )   +   1000    ;   }
        if (    mvInput >=  41.665  &&  mvInput <=  42.053  )   {   tempOut =   25.7731958762888    *   (   mvInput -   41.665  )   +   1010    ;   }
        if (    mvInput >=  42.053  &&  mvInput <=  42.44   )   {   tempOut =   25.8397932816537    *   (   mvInput -   42.053  )   +   1020    ;   }
        if (    mvInput >=  42.44   &&  mvInput <=  42.826  )   {   tempOut =   25.9067357512951    *   (   mvInput -   42.44   )   +   1030    ;   }
        if (    mvInput >=  42.826  &&  mvInput <=  43.211  )   {   tempOut =   25.9740259740261    *   (   mvInput -   42.826  )   +   1040    ;   }
        if (    mvInput >=  43.211  &&  mvInput <=  43.595  )   {   tempOut =   26.0416666666666    *   (   mvInput -   43.211  )   +   1050    ;   }
        if (    mvInput >=  43.595  &&  mvInput <=  43.978  )   {   tempOut =   26.1096605744124    *   (   mvInput -   43.595  )   +   1060    ;   }
        if (    mvInput >=  43.978  &&  mvInput <=  44.359  )   {   tempOut =   26.246719160105 *   (   mvInput -   43.978  )   +   1070    ;   }
        if (    mvInput >=  44.359  &&  mvInput <=  44.74   )   {   tempOut =   26.246719160105 *   (   mvInput -   44.359  )   +   1080    ;   }
        if (    mvInput >=  44.74   &&  mvInput <=  45.119  )   {   tempOut =   26.3852242744065    *   (   mvInput -   44.74   )   +   1090    ;   }
        if (    mvInput >=  45.119  &&  mvInput <=  45.497  )   {   tempOut =   26.4550264550264    *   (   mvInput -   45.119  )   +   1100    ;   }
        if (    mvInput >=  45.497  &&  mvInput <=  45.873  )   {   tempOut =   26.5957446808512    *   (   mvInput -   45.497  )   +   1110    ;   }
        if (    mvInput >=  45.873  &&  mvInput <=  46.249  )   {   tempOut =   26.5957446808507    *   (   mvInput -   45.873  )   +   1120    ;   }
        if (    mvInput >=  46.249  &&  mvInput <=  46.623  )   {   tempOut =   26.7379679144388    *   (   mvInput -   46.249  )   +   1130    ;   }
        if (    mvInput >=  46.623  &&  mvInput <=  46.995  )   {   tempOut =   26.8817204301075    *   (   mvInput -   46.623  )   +   1140    ;   }
        if (    mvInput >=  46.995  &&  mvInput <=  47.367  )   {   tempOut =   26.8817204301075    *   (   mvInput -   46.995  )   +   1150    ;   }
        if (    mvInput >=  47.367  &&  mvInput <=  47.737  )   {   tempOut =   27.0270270270267    *   (   mvInput -   47.367  )   +   1160    ;   }
        if (    mvInput >=  47.737  &&  mvInput <=  48.105  )   {   tempOut =   27.1739130434786    *   (   mvInput -   47.737  )   +   1170    ;   }
        if (    mvInput >=  48.105  &&  mvInput <=  48.473  )   {   tempOut =   27.1739130434781    *   (   mvInput -   48.105  )   +   1180    ;   }
        if (    mvInput >=  48.473  &&  mvInput <=  48.838  )   {   tempOut =   27.3972602739725    *   (   mvInput -   48.473  )   +   1190    ;   }
        if (    mvInput >=  48.838  &&  mvInput <=  49.202  )   {   tempOut =   27.4725274725277    *   (   mvInput -   48.838  )   +   1200    ;   }
        if (    mvInput >=  49.202  &&  mvInput <=  49.565  )   {   tempOut =   27.5482093663912    *   (   mvInput -   49.202  )   +   1210    ;   }
        if (    mvInput >=  49.565  &&  mvInput <=  49.926  )   {   tempOut =   27.7008310249304    *   (   mvInput -   49.565  )   +   1220    ;   }
        if (    mvInput >=  49.926  &&  mvInput <=  50.286  )   {   tempOut =   27.7777777777778    *   (   mvInput -   49.926  )   +   1230    ;   }
        if (    mvInput >=  50.286  &&  mvInput <=  50.644  )   {   tempOut =   27.932960893855 *   (   mvInput -   50.286  )   +   1240    ;   }
        if (    mvInput >=  50.644  &&  mvInput <=  51  )   {   tempOut =   28.0898876404493    *   (   mvInput -   50.644  )   +   1250    ;   }
        if (    mvInput >=  51  &&  mvInput <=  51.355  )   {   tempOut =   28.1690140845073    *   (   mvInput -   51  )   +   1260    ;   }
        if (    mvInput >=  51.355  &&  mvInput <=  51.708  )   {   tempOut =   28.3286118980169    *   (   mvInput -   51.355  )   +   1270    ;   }
        if (    mvInput >=  51.708  &&  mvInput <=  52.06   )   {   tempOut =   28.4090909090906    *   (   mvInput -   51.708  )   +   1280    ;   }
        if (    mvInput >=  52.06   &&  mvInput <=  52.41   )   {   tempOut =   28.571428571429 *   (   mvInput -   52.06   )   +   1290    ;   }
        if (    mvInput >=  52.41   &&  mvInput <=  52.759  )   {   tempOut =   28.6532951289395    *   (   mvInput -   52.41   )   +   1300    ;   }
        if (    mvInput >=  52.759  &&  mvInput <=  53.106  )   {   tempOut =   28.8184438040345    *   (   mvInput -   52.759  )   +   1310    ;   }
        if (    mvInput >=  53.106  &&  mvInput <=  53.451  )   {   tempOut =   28.9855072463769    *   (   mvInput -   53.106  )   +   1320    ;   }
        if (    mvInput >=  53.451  &&  mvInput <=  53.795  )   {   tempOut =   29.0697674418604    *   (   mvInput -   53.451  )   +   1330    ;   }
        if (    mvInput >=  53.795  &&  mvInput <=  54.138  )   {   tempOut =   29.1545189504376    *   (   mvInput -   53.795  )   +   1340    ;   }
        if (    mvInput >=  54.138  &&  mvInput <=  54.479  )   {   tempOut =   29.3255131964808    *   (   mvInput -   54.138  )   +   1350    ;   }
        if (    mvInput >=  54.479  &&  mvInput <=  54.819  )   {   tempOut =   29.4117647058821    *   (   mvInput -   54.479  )   +   1360    ;   }
        if (    mvInput >=  54.819  &&  mvInput <=  54.886  )   {   tempOut =   179.10447761194 *   (   mvInput -   54.819  )   +   1370    ;   }


        char strOut[50] = "";

        sprintf(strOut, "\nTMP Temp: %0.1f C\nThermocouple Temp: %0.1f\n", decodedTmp, tempOut);

        putsUart0(strOut);


        waitMicrosecond(1000000);

    }
}
