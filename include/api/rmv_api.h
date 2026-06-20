#pragma once
#include <vector>
#include <Arduino.h>
#include <ArduinoJson.h>

struct Station {
    String id;
    String name;
    String type;
};

struct DepartureInfo {
    String line;
    String direction;
    String directionFlag;
    String time;
    String rtTime;
    bool cancelled;
    String track;
    String category;
    String lead; // Service disruption lead text
    String text; // Service disruption full text
};

struct DepartureData {
    String stopId;
    String stopName;
    std::vector<DepartureInfo> departures;
    int departureCount;
};

extern std::vector<Station> stations;

// === Trip/Connection data structures ===
struct TripLeg {
    char departureTime[6];   // "23:06"
    char arrivalTime[6];     // "23:25"
    char rtDepartureTime[6]; // "23:14" (real-time, empty if on-time)
    char rtArrivalTime[6];   // "23:33" (real-time, empty if on-time)
    char line[10];           // "RB68"
    char direction[48];      // "Heidelberg Bahnhof"
    char platform[6];        // "10"
};

struct TripConnection {
    TripLeg legs[4];         // Max 4 legs (3 transfers)
    int legCount;
    int durationMinutes;     // Total journey time
};

struct TripData {
    TripConnection connections[8]; // Next 8 connections
    int connectionCount;
};

void getNearbyStops(float lat, float lon);
bool getDepartureFromRMV(const char* stopId, DepartureData& departData);
bool getTripFromRMV(const char* originId, const char* destId, TripData& tripData);
bool populateDepartureData(const JsonDocument& doc, DepartureData& departData);
