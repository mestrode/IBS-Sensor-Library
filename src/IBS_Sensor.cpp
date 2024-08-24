// IBS_Sensor.cpp
// Interfaces a Hella IBS-200X Battery Sensor via Lin-BUS
// Provides voltage, curret, State of Charge, etc.
//
// Copyright mestrode <ardlib@mestro.de>
// Original Source: https://github.com/mestrode/IBS-Sensor-Library
//
// Requires class Lin_Interface: https://github.com/mestrode/Lin-Interface-Library
//
// Includes informations provided by breezer
//  https://www.kastenwagenforum.de/forum/threads/diy-hella-ibs-batteriecomputer.31724/page-2
// Includes informations from the code created by Frank Schöniger
//  https://github.com/frankschoeniger/LIN_Interface

#include "IBS_Sensor.hpp"

//-----------------------------------------------------------------------------
// Frame IDs depends on SensorNo ("Sensor 1" or "Sensor 2" is marked on the label)

constexpr auto IBS_FRM_STA = 0; /* 0x27 */
constexpr auto IBS_FRM_CUR = 1; /* 0x28 */
constexpr auto IBS_FRM_ERR = 2; /* 0x29 */
constexpr auto IBS_FRM_tb3 = 3; /* 0x2A */
constexpr auto IBS_FRM_SOx = 4; /* 0x2B */
constexpr auto IBS_FRM_CAP = 5; /* 0x2C */

//                                    0     1     2     3     4     5
//                                   STA   CUR   ERR   tb3   SOx   CAP
const uint8_t IBS_FrameID[2][6] = {{0x21, 0x22, 0x23, 0x24, 0x25, 0x26},  // "Sensor 1"
                                   {0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C}}; // "Sensor 2"


// Standard Diagnostic Service based on DTL Diagnostic Transport Layer
// Source: https://lipowsky.com/downloads/Software/LIN-Basics_for_Beginners-EN.pdf
// Slide-82
// List of SID
// 0x00 - 0xAF // Reserved
constexpr auto SID_ASSIGN_NAD = 0xB0; // Optional
constexpr auto SID_ASSIGN_FRAME_ID = 0xB1; // Obsolete
constexpr auto SID_READ_BY_ID = 0xB2; // Mandantory
constexpr auto SID_CONDITIONAL_CHANGE = 0xB3; // Optional
constexpr auto SID_DATA_DUMP = 0xB4; // Optional
constexpr auto SID_RESERVED = 0xB5; // Reserved - (BUT USED ON IBS)
constexpr auto SID_SAVE_CONFIG = 0xB6; // Optional
constexpr auto SID_ASSIGN_FRAME_IDENTIFIER_RANGE = 0xB7; // Mandatory
// 0xB8 - 0xFF // Reserved

//-----------------------------------------------------------------------------
// There are additional Frames, the Sensor is responding to

// If you send one of those Frames to "Sensor 2", it will response:
// ID 34h - Resonse: 20:4E:20:4E:00:00:00:00
// ID 35h - Resonse: 00:00:00:00:00:00:00:00
// ID 36h - Resonse: 00:00:08:00:F8:FF
// ID 37h - Resonse: F4:2E:2D:00:00:00:00:00
// What does it mean?

//-----------------------------------------------------------------------------
// constructor 

// constructor of class IBS_Sensor
IBS_Sensor::IBS_Sensor(int SensorNo) {

    // "Sensor 1" = 1     =>     _NodeId = 0;
    // "Sensor 2" = 2     =>     _NodeId = 1;
    _NodeId = SensorNo-1;
};

//-----------------------------------------------------------------------------
// Request current valus form sensor

/// Read current sensor data (at least all usefull frames)
/// @returns success of _all_ operations
bool IBS_Sensor::readFrames()
{
    //ensure not work with received data (in case of a chksum error at the first run)
    StatusReady = false;

    //Read max 10 times sensor status / wait for valid data
    for (int i=10; i>=0; i--) {
        // StatusReady Flag is included in Frame "Status"
        readFrameStatus();
        if (StatusReady)
          break;
    }

    if (!StatusReady) {
        return false;
    }

    bool success =       readFrameCurrent();
    success = success && readFrameSOx();
    success = success && readFrameCapacity();

//  Don't know that to do with the results of Frame Error and TB3
//    success = success && readFrameError();
//    success = success && readFrameTB3();

    return success;
}

// Request Frame "Status": only Ready Flag is asumed
/// @returns checksum of frame is valid, data accepted
bool IBS_Sensor::readFrameStatus()
{
// FrameID "Sensor 1" = 21h
//         "Sensor 2" = 27h

// ID 27h - STA - D1                        = identification, may state of sensor?
//                LL                        = status byte
//                01                        = Sensor ready, data available flag ?
// Sends usually data like 192 194 208=linked to Cap_Max?

    bool chkSumValid = LinBus->readFrame(IBS_FrameID[_NodeId][IBS_FRM_STA]);
    if (chkSumValid)
    {
        StatusByte = LinBus->LinMessage[0];
        // Bit 0 seemed to be a kind of Data Ready Flag
        StatusReady = ~(LinBus->LinMessage[0] & 0x01);
    }
    return chkSumValid;
}

/// Request Frame CUR: Ubat, Ibat, Tbat and unknown1
/// @returns checksum of frame is valid, data accepted
bool IBS_Sensor::readFrameCurrent()
{
// FrameID "Sensor 1" = 22h
//         "Sensor 2" = 28h

// ID 28h - CUR - AB:84:1E:F4:2E:84:7A
//                IL IM IH                  = Ibat (x-2000000)/1000 Ampere, positive Werte = Batterie laden
//                         UL UH            = Ubat x/1000 Volt
//                               TT         = Tbat x/2-40 °C
//                                  ??      = 0x7A and 0x7C observed, changes with event on unknown6?

    bool chkSumValid = LinBus->readFrame(IBS_FrameID[_NodeId][IBS_FRM_CUR]);
    if (chkSumValid)
    {
        Ibat = (float((long(LinBus->LinMessage[2]) << 16) + (long(LinBus->LinMessage[1]) << 8)
             + long(LinBus->LinMessage[0]) - 2000000L)) / 1000.0;
        Ubat = (float((LinBus->LinMessage[4] << 8) + LinBus->LinMessage[3])) / 1000.0;
        Tbat = float(LinBus->LinMessage[5]) / 2 - 40;
        unknown1 = LinBus->LinMessage[6];
    }
    return chkSumValid;
}

/// Request Frame "Error": only one Byte maybe with some Error Flags
/// @returns checksum of frame is valid, data accepted
bool IBS_Sensor::readFrameError()
{
// FrameID "Sensor 1" = 23h
//         "Sensor 2" = 29h

// ID 29h - ERR - 00
//                LL = Error flags or code

    bool chkSumValid = LinBus->readFrame(IBS_FrameID[_NodeId][IBS_FRM_ERR]);
    if (chkSumValid)
    {
        ErrorByte = LinBus->LinMessage[0]; // IBS_Error bit code?
    }
    return chkSumValid;
}

/// Request Frame tb3: 4 bytes, but unknown content
/// @returns checksum of frame is valid, data accepted
bool IBS_Sensor::readFrameTB3()
{
// FrameID "Sensor 1" = 24h
//         "Sensor 2" = 2Ah

// ID 2Ah - tb3 - 00:00:00:00
//                LL HH          = unknown2
//                      LL HH    = unknown3

    bool chkSumValid = LinBus->readFrame(IBS_FrameID[_NodeId][IBS_FRM_tb3]);
    if (chkSumValid)
    {
        unknown2 = (LinBus->LinMessage[1] << 8) + LinBus->LinMessage[0];
        unknown3 = (LinBus->LinMessage[3] << 8) + LinBus->LinMessage[2];
    }
    return chkSumValid;
}

/// Request Frame SOH: SOH, SOC and 2 unknown Bytes and a unknown Word
/// @returns checksum of frame is valid, data accepted
bool IBS_Sensor::readFrameSOx()
{
// FrameID "Sensor 1" = 25h
//         "Sensor 2" = 2Bh

// ID 2Bh - SOx - 2D:C8:FF:BB:00:00
//                SoC               = State Of Charge x/2 in Prozent
//                   SoH            = State Of Health x/2 in Prozent
//                      ??          = unknown4 / has correlation to Cap_Available or SOC?
//                         ??       = unknown5 / no direkt link to unkown4?
//                            L?:H? = unknown6 / maybe some corelation to Cap_Available or SOC?
// Mayby SOF (State of Function) is included

    bool chkSumValid = LinBus->readFrame(IBS_FrameID[_NodeId][IBS_FRM_SOx]);
    if (chkSumValid)
    {
        SOC = float(LinBus->LinMessage[0]) / 2; // state of health
        SOH = float(LinBus->LinMessage[1]) / 2; // state of charge
        unknown4 =  LinBus->LinMessage[2];      // seemed to be a byte value
        unknown5 =  LinBus->LinMessage[3];      // seemed to be a byte value
        unknown6 = (LinBus->LinMessage[5] << 8) + LinBus->LinMessage[4]; // word or 2 bytes, not verified
    }
    return chkSumValid;
}

/// Request Frame "Capacity": max seen, available and configured Capacity, Calibration
/// @returns checksum of frame is valid, data accepted
bool IBS_Sensor::readFrameCapacity()
{
// FrameID "Sensor 1" = 26h
//         "Sensor 2" = 2Ch

// ID 2Ch - CAP - 20:03:B4:00:50:FE
//                LL HH                     = Max seen Capacity x/10 Ah (=SOH ?)
//                      LL HH               = Available Capacity x/10 Ah (=SOC)
//                            CfgAh         = Configured Capacity
//                               FF         = CalibByte, maybe filled with stuffing bits?
//                               01         = CalibrationDone flag, 1=ok, 0=uncalibrated

    bool chkSumValid = LinBus->readFrame(IBS_FrameID[_NodeId][IBS_FRM_CAP]);
    if (chkSumValid)
    {
        Cap_Max = (float((LinBus->LinMessage[1] << 8) + LinBus->LinMessage[0])) / 10;       // max. seen available Cap
        Cap_Available = (float((LinBus->LinMessage[3] << 8) + LinBus->LinMessage[2])) / 10; // Available Capacity
        Cap_Configured = LinBus->LinMessage[4];                                             // configured Cap
        CalibByte = LinBus->LinMessage[5];                                                  // may only Bit0 keeps information?
        CalibrationDone = bitRead(LinBus->LinMessage[5], 0);                                // 1=calibration done 0=not yet finished
    }
    return chkSumValid;
}

//-----------------------------------------------------------------------------
// configuration of Sensor - only needed once: data will be stored in sensor

/// Configure BatSensor by sending 3 Configuration-Frames
void IBS_Sensor::writeConfiguration(IBS_BatteryTypes BatType, uint8_t BatCapacity)
{
    // snapshot
    readFrameCapacity();
    Serial.print("Old capacity: ");
    Serial.print(Cap_Configured);
    Serial.print("Ah, Calibration Flag: ");
    Serial.print(CalibrationDone ? "true" : "false");
    Serial.println();

    // configure
    Serial.print(" (1/3) Read unknown Param...\n");
    readUnknownParam();               // Not sure why
    Serial.print(" (2/3) Write capacity...\n");
    writeBatCapacity(BatCapacity);    // nominal capacity (Ah)
    Serial.print(" (3/3) Write battery type...\n");
    writeBatType(BatType);            // battery type (AGM, GEL or STARTER)

    // verify
    readFrameCapacity();
    Serial.print("New capacity: ");
    Serial.print(Cap_Configured);
    Serial.print("Ah, Calibration Flag: ");
    Serial.print(CalibrationDone ? "true" : "false");
    Serial.println();
}

/// Read configuration Parameter "Unknown"
/// Parameter will be read by the configuration procedure used by the main panel
/// so I don't want to break tradition...
/// @returns no verification of success!!!
void IBS_Sensor::readUnknownParam()
{
// Function of ths configuration Frame is unknown!
// Frame is send in configuration procedure by control panel, so why miss this out?

// guess this frame does
// - general Reset of the sensor?
// - configure initial battery status? (0x7F = 50% charge state)

// Request for configuration by main panel
//  00005.731  3c  02   06  b2   3a  ff 7f ff ff  8b
//             PID Node PCI SID  CMD  0  1  2  3  CHK

// OK Response of sensor
//  00005.780  7d  02   02  f2   0a  ff ff ff ff  fe
//             PID Node LEN RSID  2  ff ff ff ff  CHK
//                                   ^^ ^^ ^^ ^^ = Stuffing
//                               ^^ = Data, but what does it mean?

    LinBus->LinMessage[0] = 0x01 + _NodeId;    // NAD = Node = Sensor No
    LinBus->LinMessage[1] = 0x06;              // PCI = Single Frame, 6 Bytes
    LinBus->LinMessage[2] = SID_READ_BY_ID;    // SID = Read by Identifier
    LinBus->LinMessage[3] = 0x3A;              // D1 = Config Type
    // the unknown content of 4 Bytes = reset configuration?
    LinBus->LinMessage[4] = 0xFF;              // D2 = Data 0 = Wildcard Supplier ID LSB
    LinBus->LinMessage[5] = 0x7F;              // D3 = Data 1 = Wildcard Supplier ID MSB
    LinBus->LinMessage[6] = 0xFF;              // D4 = Data 2 = Wildcard Function ID LSB
    LinBus->LinMessage[7] = 0xFF;              // D5 = Data 3 = Wildcard Function ID MSB

    LinBus->writeDiagnosticMasterRequest();
}

/// Write configuration Parameter "Capacity" = Value of Ah of the Battery, default factory value = "80"Ah
/// @param BatCapacity labeled capacity (in Ah) of the lead battery
/// @returns no verification of success!!!
void IBS_Sensor::writeBatCapacity(uint8_t BatCapacity)
{
// Configuration (Of Sensor Type 1)
// Battery capacity can be read back on 0x2C Byte 4
// ID 3Ch - Capacity       02   03  B5  39  BatCap :FF:FF:FF - BatCap in Ah
//                         Node PCI SID Cfg Value  STUFFING


    LinBus->LinMessage[0] = 0x01 + _NodeId;    // Sensor No
    LinBus->LinMessage[1] = 0x03;              // PCI = Single Frame, 3 Bytes
    LinBus->LinMessage[2] = SID_RESERVED;      // SID = Config Write
    LinBus->LinMessage[3] = 0x39;              // Config Adress
    LinBus->LinMessage[4] = BatCapacity;       // value e.g. 70 Ah

    LinBus->LinMessage[5] = 0xFF;              // stuffing bytes
    LinBus->LinMessage[6] = 0xFF;
    LinBus->LinMessage[7] = 0xFF;

    LinBus->writeDiagnosticMasterRequest();
}

/// Write configuration Parameter "Battery Type" and read answer back (not judged)
/// @param BatType 
/// @returns no verification of success!!!
void IBS_Sensor::writeBatType(IBS_BatteryTypes BatType)
{
// Battery Type can not be verified by using another Frame, since no Frame includes this data
// But recalibration is obviously needed, so CalibrationDone Flag should be an indicator for success
/* Procedure
00012.919  ec bc 02 dc 01 46 FF 30              CAP Sensor 2, is calibrated (FF)
00012.935  3c 02 03 b5 3a 1e ff ff ff ec ERR    Config Sensor 2 Len 3 Conf   =B5 Type 3a Data 1e
                    ^^ = SID
                          ^^ = AGM              parameter to be written
00012.988  7d 02 03 f5 3a 1e ff ff ff ac ERR    Answer Sensor 2 Len 3 Conf+40=F5 Type 3a Data 1e
                             ^^ ^^ ^^ = Stuffing
                          ^^ = AGM readback --> confirmation of parameter from device
                       ^^ = Config Adr
                    ^^ = SID+40 = RSID --> valid (command failed on 0x7F)
00013.115  ec bc 02 dc 01 46 FE 31              CAP Sensor 2, is not calibrated (FE)
                                 ^ need to calibrate
*/

// ID 3Ch - STARTER Bat    02:03:B5:3A: 0A :FF:FF:FF|01
//              GEL Bat    02:03:B5:3A: 14 :FF:FF:FF|F6
//              AGM Bat    02:03:B5:3A: 1E :FF:FF:FF|EC

    LinBus->LinMessage[0] = 0x01 + _NodeId;  // Sensor No
    LinBus->LinMessage[1] = 0x03;            // PCI = Single Frame, 3 Bytes
    LinBus->LinMessage[2] = SID_RESERVED;    // SID = Config Write
    LinBus->LinMessage[3] = 0x3A;            // Config Adress

    switch (BatType)
    {
    case BAT_TYPE_STARTER:
        LinBus->LinMessage[4] = 0x0a; // Starter Battery
        break;

    case BAT_TYPE_GEL:
        LinBus->LinMessage[4] = 0x14; // GEL Battery
        break;

    case BAT_TYPE_AGM:
        LinBus->LinMessage[4] = 0x1e; // AGM Battery
        break;
    }

    // stuffing bytes
    LinBus->LinMessage[5] = 0xFF;     // stuffing bytes
    LinBus->LinMessage[6] = 0xFF;
    LinBus->LinMessage[7] = 0xFF;

    LinBus->writeDiagnosticMasterRequest();
}
