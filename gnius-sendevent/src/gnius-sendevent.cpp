/*
 * G-NIUS Send Event
 * Copyright (c) 2025 Scott Vincent
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#ifdef _WIN32
#include <WS2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <limits.h>
typedef int SOCKET;
#define SOCKET_ERROR -1
#define INVALID_SOCKET -1
#define SOCKADDR sockaddr
#define closesocket close
#define MAXINT INT_MAX
#endif
#include "simvarDefs.h"

extern WriteEvent WriteEvents[];

const char* sendeventVersion = "v1.0.3";
const bool Debug = false;

char gniusHost[256];
const int gniusPort = 53020;

SimVars simVars;
long writeDataSize = sizeof(WriteData);
long clientDataSize = sizeof(SimVars);


/// <summary>
/// Send event to Flight Sim with optional data value and read returned value
/// </summary>
EVENT_ID sendEvent(EVENT_ID eventId, double value, bool wantResponse)
{
    EVENT_ID retVal = EVENT_SIM_START;
    SOCKET sockfd;
    sockaddr_in writeAddr;
    Request writeRequest;

    if (eventId == EVENT_CLIENT) {
        writeRequest.requestedSize = clientDataSize;
    }
    else {
        writeRequest.requestedSize = writeDataSize;
    }
    writeRequest.writeData.eventId = eventId;
    writeRequest.writeData.value = value;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET) {
        printf("Failed to create UDP socket for writing\n");
        return retVal;
    }

    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    writeAddr.sin_family = AF_INET;
    writeAddr.sin_port = htons(gniusPort);
    inet_pton(AF_INET, gniusHost, &writeAddr.sin_addr);

    int bytes = sendto(sockfd, (char*)&writeRequest, sizeof(writeRequest), 0, (SOCKADDR*)&writeAddr, sizeof(writeAddr));
    if (bytes <= 0) {
        printf("Failed to write event %d\n", eventId);
        return retVal;
    }
    else {
        //printf("Sent %d bytes to %s\n", bytes, dataLinkHost);
    }

    if (wantResponse) {
        // Wait up to one second for a reply
        timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(sockfd, &fds);

        bytes = 0;
        int sel = select(FD_SETSIZE, &fds, 0, 0, &timeout);
        if (sel > 0) {
            if (eventId == EVENT_CLIENT) {
                bytes = recv(sockfd, (char*)&simVars, clientDataSize, 0);
                if (bytes == clientDataSize) {
                    retVal = EVENT_CLIENT;
                }
                else if (bytes > 0) {
                    printf("Received %d bytes instead of %d bytes\n", bytes, clientDataSize);
                }
            }
            else {
                bytes = recv(sockfd, (char*)&retVal, sizeof(int), 0);
            }
        }

        if (bytes <= 0) {
            printf("No response\n");
        }
    }

    closesocket(sockfd);
    return retVal;
}

/// <summary>
/// Initialise
/// </summary>
bool init()
{
#ifdef _WIN32
    WSADATA wsaData;
    int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (err != 0) {
        printf("Failed to initialise Windows Sockets: %d\n", err);
        return false;
    }
#endif

    return true;
}

/// <summary>
/// Clean up
/// </summary>
void cleanup()
{
#ifdef _WIN32
    WSACleanup();
#endif
}

char* aircraftStr(AircraftData* aircraft)
{
    static char aircraftStr[256];

    sprintf(aircraftStr, "%f,%f,%d,%d,%d",
        aircraft->lat, aircraft->lon, (int)round(aircraft->heading), (int)round(aircraft->alt), (int)round(aircraft->speed));

    return aircraftStr;
}

void outputSimVars()
{
    if (simVars.connected == 0 || simVars.self.lat == MAXINT) {
        printf("\n");
        return;
    }

    printf("#15,C172,%s\n", aircraftStr(&simVars.self));

    if (simVars.ai.lat != MAXINT) {
        printf("#16,P28A,%s\n", aircraftStr(&simVars.ai));
    }
}

void writeSimVars()
{
    FILE *outf = fopen("gnius.data", "w");
    if (!outf) {
        printf("Failed to write gnius.data\n");
        return;
    }

    if (simVars.connected == 0 || simVars.self.lat == MAXINT) {
        fprintf(outf, "\n");
    }
    else {
        fprintf(outf, "#15,C172,%s\n", aircraftStr(&simVars.self));

        if (simVars.ai.lat != MAXINT) {
            fprintf(outf, "#16,P28A,%s\n", aircraftStr(&simVars.ai));
        }
    }
    fclose(outf);
}

///
/// main
///
int main(int argc, char** argv)
{
    if (argc < 2) {
        printf("Please specify an event\n");
        exit(1);
    }

    printf("flightsim-sendevent %s\n", sendeventVersion);

    char filePath[256];
    strcpy(filePath, argv[0]);
    char *pos = strrchr(filePath, '/');
    if (!pos) {
        pos = strrchr(filePath, '\\');
    }
    if (!pos) {
        pos = filePath;
    }
    else {
        pos++;
    }
    *pos = '\0';
    strcat(filePath, "gnius.host");

    FILE* inf = fopen(filePath, "r");
    if (!inf) {
        printf("Failed to open file %s\n", filePath);
        exit(1);
    }

    int count = fscanf(inf, "%s", gniusHost);
    fclose(inf);

    if (count != 1) {
        printf("Failed to read file %s\n", filePath);
        exit(1);
    }

    //printf("G-NIUS host: %s\n", gniusHost);

    if (!init()) {
        printf("Init failed\n");
        exit(1);
    }

    if (strcmp(argv[1], "background") == 0) {
        while (true) {
            if (sendEvent(EVENT_CLIENT, 0, true) == EVENT_CLIENT) {
                writeSimVars();
            }
            else {
                simVars.connected = false;
                writeSimVars();
            }
            Sleep(1000);
        }
    }
    else if (strcmp(argv[1], "simvars") == 0) {
        //printf("Sending: EVENT_CLIENT\n");
        if (sendEvent(EVENT_CLIENT, 0, true) == EVENT_CLIENT) {
            outputSimVars();
        }
    }
    else if (strcmp(argv[1], "ai_start") == 0) {
        printf("Sending: EVENT_ADD_AI\n");
        sendEvent(EVENT_ADD_AI, 0, false);
    }
    else if (strcmp(argv[1], "ai_stop") == 0) {
        printf("Sending: EVENT_REMOVE_AI\n");
        sendEvent(EVENT_REMOVE_AI, 0, false);
    }
    else if (strcmp(argv[1], "quit") == 0) {
        printf("Sending: EVENT_QUIT\n");
        sendEvent(EVENT_QUIT, 0, false);
    }
    else {
        printf("Please specify a valid event, e.g. background, simvars, ai_start, ai_stop, quit\n");
    }

    cleanup();

    return 0;
}
