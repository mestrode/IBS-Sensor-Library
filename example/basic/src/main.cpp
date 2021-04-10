#include <Arduino.h>
// LIN Bus Interface provided viy TJA1020
#include <TJA1020.hpp>
// IBS Batterie Sensor
#include <IBS_Sensor.hpp>

#define LIN_SERIAL_SPEED LIN_BAUDRATE_IBS_SENSOR /* Required by IBS Sensor */
#define PIN_NSLP 23

// utilize the TJA1020 by using UART2 for writing and reading Frames
Lin_TJA1020 LinBus(2, LIN_SERIAL_SPEED, PIN_NSLP); // UART_nr, Baudrate, /SLP

// Hella IBS 200x "Sensor 2"
IBS_Sensor BatSensor(2);

void setup()
{
  // Serial represents Serial(0)
  Serial.begin(115200);

  // configure slope rate
  Serial.print("configure low slope rate of TJA1020\n");
  LinBus.setSlope(LinBus.LowSlope);

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
    Serial.printf("Calibration done: %d &\n", BatSensor.CalibrationDone);
    Serial.printf("Voltage: %.3f Volt\n", BatSensor.Ubat);
    Serial.printf("Current: %.3f Ampere\n", BatSensor.Ibat);
    Serial.printf("State of Charge: %.1f %%\n", BatSensor.SOC);
    Serial.printf("State of Health: %.1f %%\n", BatSensor.SOH);
    Serial.printf("Available Capacity: %.1f &\n", BatSensor.Cap_Available);
}

void loop()
{
  showSensorData();

  delay(5000);
}