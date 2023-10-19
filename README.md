# IBS Sensor Library
**Hella IBS 200X Lead-Battery Sensor (Gen 1) via LIN-Bus**

The IBS 200X sensor monitors the charge and health state of a lead-acid battery. Data can be accessed through the LIN interface, and this can be easily achieved using this library.

You can provide a LIN-Interface object to the IBS Sensor object, which is used to read and write on the bus. This allows you to connect additional physical sensors to the LIN bus and correspondingly link additional devices on the software side to the LIN-Interface.

# Dependencies
The LIN Interface utilizes the Hardware Serial (UART) of an ESP32. To ensure code reusability, I have provided a separate library, which can be found here: [Lin Interface Library](https://github.com/mestrode/Lin-Interface-Library).

On the hardware side, I have used a TJA1020 Transceiver. This chip contains a state machine that needs to be controlled before you can read or write data. To simplify this process, I have created a derived class that considers the state machine every time the bus is used. You can find this class in the [Lin Transceiver Library](https://github.com/mestrode/Lin-Transceiver-Library).

# Example
This small example utilizes hardware serial (UART) number 2 of an ESP32.

```cpp
// LIN Bus Interface provided by TJA1020
#include "TJA1020.hpp"
// IBS Batterie Sensor
#include "IBS_Sensor.hpp"

#define LIN_SERIAL_SPEED LIN_BAUDRATE_IBS_SENSOR /* Required by IBS Sensor */
#define lin_NSLP_Pin 32

// Utilize the TJA1020 by using UART2 for writing and reading frames
Lin_TJA1020 LinBus(2, LIN_SERIAL_SPEED, lin_NSLP_Pin); // UART_nr, Baudrate, /SLP

// Hella IBS 200X "Sensor 2"
IBS_Sensor BatSensor(2);

void setup()
{
    // Tell the BatSensor object which LinBus to be used
    BatSensor.LinBus = &LinBus;
}

void showSensorData() {
    // Read data from the sensor (request data using several Lin-Frames)
    BatSensor.readFrames();

    // If you're using a Bus Transceiver that should go to sleep after
    // transmission (depends on your hardware)
    LinBus.setMode(LinBus.Sleep);

    // Use received data
    Serial.printf("Calibration done: %d &\n",
                               BatSensor.CalibrationDone);
    Serial.printf("Voltage: %.3f Volt\n", BatSensor.Ubat);
    Serial.printf("Current: %.3f Ampere\n", BatSensor.Ibat);
    Serial.printf("State of Charge: %.1f %\n", BatSensor.SOC);
    Serial.printf("State of Health: %.1f &\n", BatSensor.SOH);
    Serial.printf("Available Capacity: %.1f &\n", BatSensor.Cap_Available);
}
```
Please note that the sensor provides more data than what is presented in this small example.

# Limitations
According to the manufacturer, the sensor requires a period of approximately 3 hours without current flowing in or out of the battery to be calibrated (indicated by the `CalibrationDone` flag).

Not all bytes and frames are known to be fully interpreted at this time. However, there is enough information to use the sensor without restrictions.

For example, the main panel writes a configuration frame whose function is not known. Still, it is advisable to handle configuration as the manufacturer does.

Keep in mind that the `Cap_Available` value provides you with the theoretical available capacity. However, lead-acid battery technology has limitations, so you can effectively use only around 50% of the labeled capacity. Deeper discharges may cause further damage to the battery.

# Configuration of Sensor
You may want to configure the sensor **once** to match the specifications of your specific battery. There's no need to reconfigure the sensor on every startup because the configuration data is stored within the sensor. In fact, you'll only need to configure the sensor once, unless you physically replace your battery with one having different properties.

To configure the sensor and provide information about your battery type (e.g., AGM) and its labeled capacity in Ah, use the following code:

```cpp
BatSensor.writeConfiguration(BatSensor.BAT_TYPE_AGM, 180);
```

This will write three configuration frames. While the functions of two of these frames are known, I have chosen to include the third, unknown frame, as the original panel also does so.

# Hardware
## Product Side of Manufacturer
For more information about the manufacturer's product, you can visit the following links:

- [Hella Intelligent Battery Sensors](https://www.hella.com/microsite-electronics/en/Intelligent-battery-sensors-154.html)
- [Hella Intelligent Battery Sensor 200X](https://www.hella.com/microsite-electronics/en/Intelligent-battery-sensor-200X-491.html)

In the meantime, a second generation of the sensor is available. You can find details about it [here](https://www.hella.com/caravan/en/Intelligent-battery-sensor-1564.html).

## Pinning of Connector
The pinning of the connector is as follows:
- Pin 1 = 12V
- Pin 2 = LIN

## Connector Housing (2 pin MCON-1.2 LL type)
The connector housing has coding A. If the mirrored version is available, it is recommended to use it. If the mirrored version is not available, you can consider using a cutter.
- 872-858-565 (Hirschmann)
- ? (Tyco)

## Single crimp pin
- 7-1452671-1 (Tyco, MCON-1.2 LL, single wire seal)

## Single wire seal
- 2098582-1 (Tyco, will be ok for small wires)

# Safety
Please ensure safety by following the user's manual provided by the manufacturer. Additionally, it is advisable to include a fuse between the battery's positive pole and the 12V pin of the sensor for added protection.

# See Also
This library was created with contributions and additional information from various sources. Special thanks to the contributors!

- [Discussion in a German forum](https://www.kastenwagenforum.de/forum/threads/diy-hella-ibs-batteriecomputer.31724/): This forum discussion contains a bus log (see the "etc" folder of this project) that can be helpful.

- [HW-Project by Frank Schöniger on Github](https://github.com/frankschoeniger/LIN_Interface): Frank Schöniger's HW-Project can provide further insights and resources related to LIN interfaces.