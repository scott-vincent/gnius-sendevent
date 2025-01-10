
#ifndef _SIMVARDEFS_H_
#define _SIMVARDEFS_H_

#include <stdio.h>

struct AircraftData {
    double lat;
    double lon;
    double heading;
    double alt;
    double speed;
    double transponderCode;
};

struct SimVars
{
    double connected = 0;
    AircraftData self;
    AircraftData ai;
};

enum EVENT_ID {
    EVENT_SIM_START,
    EVENT_ADDED_AIRCRAFT,
    EVENT_REMOVED_AIRCRAFT,
    EVENT_CLIENT,
    EVENT_ADD_AI,
    EVENT_REMOVE_AI,
    EVENT_QUIT,
    EVENT_SIM_STOP,
};

enum REQUEST_ID {
    REQUEST_AI_AIRCRAFT_ADD,
    REQUEST_AI_AIRCRAFT_REMOVE,
    REQUEST_RELEASE_CONTROL,
    REQUEST_SELF,
    REQUEST_AI,
};

enum DEFINITION_ID {
    DEF_ATTITUDE,
    DEF_AIRCRAFT,
};

struct AttitudeData {
    double heading;
    double alt;
    double bank;
    double pitch;
    double speed;

    int dataSize;
};

struct WriteEvent {
    EVENT_ID id;
    const char* name;
};

struct WriteData {
    EVENT_ID eventId;
    double value;
};

struct Request {
    int requestedSize;
    int wantFullData;
    WriteData writeData;
};

#endif