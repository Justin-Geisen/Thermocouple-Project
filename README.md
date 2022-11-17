# Thermocouple Project

## Items
* Thermocouple Type K 
* TMP36
* TM4C123GH6PM Microcontroller made by Texas Instruments

## Summary
Typically, a person would measure the voltage of two thermocouples, one with dry ice (0 degrees C) and the other in the part they would like to measure. Then, they would add the voltage together (in mV) and use a lookup table to find the voltage. Howevever, this can be very cumbersome. Dry ice can evaporate and depending on the circumstances, can be a hazard. That is where this project shines. This project uses one thermocouple and one external temperature sensor(TMP36). The temperature sensor inputs the temperature into a reverse lookup table to find the voltage output that a thermocouple would have at room temperature. 

## Measuring the TMP36
The TMP36 outputs a voltage linear to temperature. 

It is 750mV at 25C and increase by 10mV / deg C.
