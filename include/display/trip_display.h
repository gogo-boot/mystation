#pragma once
#include "api/rmv_api.h"
#include <Arduino.h>

class TripDisplay {
public:
    // Draw trip connections in the given area
    static void drawTripConnections(const TripData& tripData, int16_t x, int16_t y, int16_t w, int16_t h);

private:
    // Draw a single connection, returns height consumed (0 if didn't fit)
    static int16_t drawSingleConnection(const TripConnection& conn, int16_t x, int16_t y, int16_t w,
                                        int16_t maxH, uint32_t currentTime);

    // Calculate height needed for a connection
    static int16_t getConnectionHeight(const TripConnection& conn);

    // Extract station name from stop ID format "A=1@O=Name@X=...@Y=...@"
    static String extractStopName(const char* stopId);
};
