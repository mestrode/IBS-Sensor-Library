# IBS-Sensor-Library
Hella IBS 200X Lead-Battery Sensor (Gen 1) via LIN-Bus

A IBS 200X sensors overserves the charge and health state of a Lead-Battery. Data can be read out via Lin interface, and this can easily be done with this library.

You can provide a LIN-Interface object to the IBS-Sensor object, wich is used to read and write on the bus. Therefore you can plug additional physical sensors on the Lin line and accordingly link additional devices on the software side to the Lin-Interface.

# dependencies
The Lin Interface utilizes a Hardware Serial (UART) of an ESP32. To make the code reusable, I provided a separate Library: https://github.com/mestrode/Lin-Interface-Library

I've used a TJA1020 Transceiver on HW side. The chip contains a statemachine, which needs to be controlled before you will be able to write or receive data. To keep thinks easy, I created a derived class which consider the statemachine every time using the bus: https://github.com/mestrode/Lin-Transceiver-Library

# example
This small example utilizes a hardware serial (UART) no 2 of an ESP32.

```cpp
// LIN Bus Interface provided viy TJA1020
#include "TJA1020.hpp"
// IBS Batterie Sensor
#include "IBS_Sensor.hpp"

#define LIN_SERIAL_SPEED LIN_BAUDRATE_IBS_SENSOR /* Required by IBS Sensor */
#define lin_NSLP_Pin 32

// utilize the TJA1020 by using UART2 for writing and reading Frames
Lin_TJA1020 LinBus(2, LIN_SERIAL_SPEED, lin_NSLP_Pin); // UART_nr, Baudrate, /SLP

// Hella IBS 200x "Sensor 2"
IBS_Sensor BatSensor(2);

void setup()
{
    // tell the BatSensor object which LinBus to be used
    BatSensor.LinBus = &LinBus;
}

void showSensorData() {
    // read data from sensor (method request data by using several
    // Lin-Frames)
    BatSensor.readFrames();

    // may you using a Bus-Transceiver which should go to sleep after 
    // transmission (depends on your HW)
    LinBus.setMode(LinBus.Sleep);

    // use received data
    Serial.printf("Calibration done: %d &\n",
                               BatSensor.CalibrationDone);
    Serial.printf("Voltage: %.3f Volt\n", BatSensor.Ubat);
    Serial.printf("Current: %.3f Ampere\n", BatSensor.Ibat);
    Serial.printf("State of Charge: %.1f %\n", BatSensor.SOC);
    Serial.printf("State of Health: %.1f &\n", BatSensor.SOH);
    Serial.printf("Available Capacity: %.1f &\n", BatSensor.Cap_Available);
}
```
There are mode data provided by the sensor, than shown in this small example.

# limitations
The manufacturer says the sensor will need aperiod of ~3h without current goint in or out of the battery to be calibratet (see CalibrationDone flag).

Not all Bytes and Frames are known to be interpreted, yet. But it's enough to use the sensor in an unrestricted way.

E.g. the main panel writes a configuration frame whose function is not known. But I decided to do configuration thinks like the mfg does.

Consider the Cap_Available tells you the theoretical available capacity. But the technology of lead batterys is limited, so you can use only ~50% of the labled capacity. Deeper discharge will damage the battery further.

# configuration of sensor
May you want to configure the sensor **once** to your specific battery.
Do not configure your sensor on every startup. This is also not needed, cause the data is stored in the sensor.
Actually you will only need to configure the sensor once, until you changed your battery physically (with different properties).

    // configure sensor and provide Battery Type (AGM) and labeled capacity in Ah
    BatSensor.writeConfiguration(BatSensor.BAT_TYPE_AGM, 180);

This will write three configuration Frames. The function for two of them is known. Anyway, I decided to write also the third, unknown frame since the original panel does this, too.

# hardware
## Productside of manufacturer
https://www.hella.com/microsite-electronics/en/Intelligent-battery-sensors-154.html
https://www.hella.com/microsite-electronics/en/Intelligent-battery-sensor-200X-491.html

In the meanwhile there is a second generation available. 
https://www.hella.com/caravan/en/Intelligent-battery-sensor-1564.html

## Pinning of connector
Pin 1 = 12V, Pin 2 = Lin

## Connector Housing (2 pin MCON-1.2 LL type)
Connector is has coding A
If available, try the mirrored version, if not available, try a cutter.
872-858-565 (Hirschmann)
? (Tyco)

## Single crimp pin
7-1452671-1 (Tyco, MCON-1.2 LL, single wire seal)

### Single wire seal
2098582-1 (Tyco, will be ok for small wires)

# safety
follow the users manual of the manufacturer and inclde a fuse between battery plus pole and the 12V pin of the sensor

# see also
This library was created by using additional information form several sides. Thank you to the contributors!

* discussion in a german forum, contains the bus log (see folder "etc" of this project)
https://www.kastenwagenforum.de/forum/threads/diy-hella-ibs-batteriecomputer.31724/

* HW-Project fom Frank Schöniger on Github
https://github.com/frankschoeniger/LIN_Interface
