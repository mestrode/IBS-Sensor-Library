// IBS_Sensor.hpp
// Interfaces a Hella IBS-200X Battery Sensor via Lin-BUS
// Provides voltage, curret, State of Charge, etc.
//
// Copyright mestrode <ardlib@mestro.de>
// Original Source: "https://github.com/mestrode/IBS-Sensor-Library"
//
// Requires class Lin_Interface: https://github.com/mestrode/Lin-Interface-Library
//
// Includes informations provided by breezer
//  https://www.kastenwagenforum.de/forum/threads/diy-hella-ibs-batteriecomputer.31724/page-2
// Includes informations from the code created by Frank Schöniger
//  https://github.com/frankschoeniger/LIN_Interface

#pragma once

//#include <Arduino.h>
//This class needs a lin interface to read and write on
#include <Lin_Interface.hpp>

#define LIN_BAUDRATE_IBS_SENSOR 19200

class IBS_Sensor
{
public:
    // Batery types
    enum IBS_BatteryTypes
    {
        BAT_TYPE_STARTER,
        BAT_TYPE_GEL,
        BAT_TYPE_AGM
    };

    // constructor
    IBS_Sensor(int SensorNo);

    // Lin Interface will be used to send and request frames
    Lin_Interface *LinBus;

    // Frame "Status"
    bool StatusReady = false;  // ready for Data Request?
    uint8_t StatusByte = 0x00; // provides at least a StatusReady flag, but the is more encoded

    // Frame "Current"
    float Ibat = 0.0; // Current in Ampere
    float Ubat = 0.0; // Battery Voltage in Volt
    float Tbat = 0.0; // Battery Temperature in Celsius
    uint8_t unknown1 = 0x00;

    // Frame "Error", flags / code unknown
    uint8_t ErrorByte = 0x00; // Error code/flags?

    // Frame "tb3", data unknown
    uint16_t unknown2 = 0;
    uint16_t unknown3 = 0;

    // Frame "SOx", mayby includes State of Functon?
    float SOH = -1.0; // State of Health in percent (need replace?)
    float SOC = -1.0; // State of Charge in percent (need charge?)
    uint8_t unknown4 = 0;
    uint8_t unknown5 = 0;
    uint16_t unknown6 = 0;

    // Frame "Capacity"
    float Cap_Max = 0.0;          // max seen capacity (eq. SOH)
    float Cap_Available = 0.0;    // max available capacity (eq. SOC)
    uint8_t Cap_Configured = 0;   // configured battery capacity
    bool CalibrationDone = false; // data may plausible, but sensor has still some doubts
    uint8_t CalibByte = 0x00;     // Capacity flag byte, may only contains CalibrationDone flag

    //-----------------------------------------------------------------------------
    // Request current valus form sensor

    // regular read
    bool readFrames();

    // specific read of Frame
    bool readFrameStatus();
    bool readFrameCurrent();
    bool readFrameError();
    bool readFrameSOx();
    bool readFrameCapacity();
    bool readFrameTB3();

    //-----------------------------------------------------------------------------
    // configuration of Sensor - only needed once: data will be stored in sensor

    //configuration of sensor
    void writeConfiguration(IBS_BatteryTypes BatType, uint8_t BatCapacity);

    // all known configuraton Frames, used by original Panel
    void writeUnknownParam();
    void writeBatCapacity(uint8_t BatCapacity);
    void writeBatType(IBS_BatteryTypes BatType);

private:
    // SensorNo = 0   --> Hella IBS 200 labeled "Sensor 1"
    // SensorNo = 1   --> Hella IBS 200 labeled "Sensor 2"
    int _SensorNo;
};
