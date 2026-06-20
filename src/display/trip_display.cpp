#include "display/trip_display.h"
#include "display/text_utils.h"
#include "config/config_manager.h"
#include "util/util.h"
#include "global_instances.h"
#include <esp_log.h>
#include <time.h>

static const char* TAG = "TRIP_DISPLAY";

static constexpr int16_t ROW_HEIGHT = 18;
static constexpr int16_t CONNECTION_PADDING = 4;
static constexpr int16_t MARGIN = 5;

// Column positions (relative to x)
static constexpr int16_t COL_DEP_TIME = 0;
static constexpr int16_t COL_LINES = 70;
static constexpr int16_t COL_DURATION = -55; // from right edge

String TripDisplay::extractStopName(const char* stopId) {
    // Extract name from "A=1@O=Name@X=..."
    String id = String(stopId);
    int oStart = id.indexOf("@O=");
    if (oStart < 0) return id;
    oStart += 3;
    int oEnd = id.indexOf('@', oStart);
    if (oEnd < 0) return id.substring(oStart);
    return id.substring(oStart, oEnd);
}

int16_t TripDisplay::getConnectionHeight(const TripConnection& conn) {
    // Row 1: times + lines
    // Row 2+: "in X min" + transfer stations (one per transfer)
    int transfers = conn.legCount > 1 ? conn.legCount - 1 : 0;
    int rows = 1 + max(1, transfers); // at least 2 rows per connection
    return rows * ROW_HEIGHT + CONNECTION_PADDING;
}

void TripDisplay::drawTripConnections(const TripData& tripData, int16_t x, int16_t y, int16_t w, int16_t h) {
    ESP_LOGI(TAG, "Drawing %d trip connections at (%d,%d) %dx%d", tripData.connectionCount, x, y, w, h);

    RTCConfigData& config = ConfigManager::getConfig();

    // Header: Origin → Destination
    TextUtils::setFont12px_margin15px();
    String origin = Util::shortenStationName(config.selectedStopName);
    String dest = extractStopName(config.tripDestId);
    dest = Util::shortenStationName(dest);
    String header = origin + " -> " + dest;
    header = TextUtils::shortenTextToFit(header, w - MARGIN * 2);
    TextUtils::printTextAtTopMargin(x + MARGIN, y, header);
    y += ROW_HEIGHT + 2;

    // Separator line
    display.drawFastHLine(x, y, w, GxEPD_BLACK);
    y += 4;

    // Get current time for "in X min" calculation
    time_t now;
    time(&now);
    uint32_t currentTime = (uint32_t)now;

    // Draw each connection
    TextUtils::setFont10px_margin12px();
    for (int i = 0; i < tripData.connectionCount; i++) {
        const TripConnection& conn = tripData.connections[i];

        int16_t connHeight = getConnectionHeight(conn);
        if (y + connHeight > h) {
            ESP_LOGI(TAG, "Stopping at connection %d — no space left", i);
            break;
        }

        drawSingleConnection(conn, x, y, w, h - y, currentTime);
        y += connHeight;

        // Separator line between connections
        display.drawFastHLine(x + MARGIN, y - 2, w - MARGIN * 2, GxEPD_BLACK);
    }

    // No connections found
    if (tripData.connectionCount == 0) {
        TextUtils::printTextAtTopMargin(x + MARGIN, y, "Keine Verbindungen gefunden");
    }
}

int16_t TripDisplay::drawSingleConnection(const TripConnection& conn, int16_t x, int16_t y,
                                           int16_t w, int16_t maxH, uint32_t currentTime) {
    int16_t startY = y;
    int16_t rightEdge = x + w - MARGIN;

    // === ROW 1: dep_time [delay] [line1] ─O─ [line2] ... arr_time [delay] duration ===

    // Departure time
    TextUtils::printTextAtTopMargin(x + MARGIN + COL_DEP_TIME, y, conn.legs[0].departureTime);

    // Departure delay
    int16_t currentX = x + MARGIN + 35;
    if (conn.legs[0].rtDepartureTime[0] != '\0' &&
        strcmp(conn.legs[0].rtDepartureTime, conn.legs[0].departureTime) != 0) {
        // Calculate delay in minutes
        int schedMin = atoi(conn.legs[0].departureTime) * 60 + atoi(conn.legs[0].departureTime + 3);
        int rtMin = atoi(conn.legs[0].rtDepartureTime) * 60 + atoi(conn.legs[0].rtDepartureTime + 3);
        int delay = rtMin - schedMin;
        if (delay > 0) {
            String delayStr = "+" + String(delay);
            TextUtils::printTextAtTopMargin(currentX, y, delayStr.c_str());
        }
    }

    // Line numbers with ─O─ between them
    currentX = x + MARGIN + COL_LINES;
    for (int leg = 0; leg < conn.legCount; leg++) {
        if (leg > 0) {
            // Draw ─O─ transfer symbol
            TextUtils::printTextAtTopMargin(currentX, y, "-O-");
            currentX += TextUtils::getTextWidth("-O-") + 2;
        }
        // Draw [line] box
        String line = String(conn.legs[leg].line);
        int16_t lineW = TextUtils::getTextWidth(line) + 6;
        display.drawRect(currentX, y - 1, lineW, ROW_HEIGHT - 4, GxEPD_BLACK);
        TextUtils::printTextAtTopMargin(currentX + 3, y, line.c_str());
        currentX += lineW + 4;
    }

    // Arrival time (right-aligned)
    const TripLeg& lastLeg = conn.legs[conn.legCount - 1];
    String arrStr = String(lastLeg.arrivalTime);
    int16_t arrW = TextUtils::getTextWidth(arrStr);

    // Duration (far right)
    String durStr = String(conn.durationMinutes) + " min";
    int16_t durW = TextUtils::getTextWidth(durStr);
    TextUtils::printTextAtTopMargin(rightEdge - durW, y, durStr.c_str());

    // Arrival time (before duration)
    TextUtils::printTextAtTopMargin(rightEdge - durW - arrW - 10, y, arrStr.c_str());

    // Arrival delay
    if (lastLeg.rtArrivalTime[0] != '\0' &&
        strcmp(lastLeg.rtArrivalTime, lastLeg.arrivalTime) != 0) {
        int schedMin = atoi(lastLeg.arrivalTime) * 60 + atoi(lastLeg.arrivalTime + 3);
        int rtMin = atoi(lastLeg.rtArrivalTime) * 60 + atoi(lastLeg.rtArrivalTime + 3);
        int delay = rtMin - schedMin;
        if (delay > 0) {
            String delayStr = "+" + String(delay);
            TextUtils::printTextAtTopMargin(rightEdge - durW - arrW - 10 + arrW + 2, y, delayStr.c_str());
        }
    }

    y += ROW_HEIGHT;

    // === ROW 2+: "in X min" + transfer stations ===

    // "in X min" under departure time
    int depHour = atoi(conn.legs[0].departureTime);
    int depMin = atoi(conn.legs[0].departureTime + 3);
    // Use rtTime if available
    if (conn.legs[0].rtDepartureTime[0] != '\0') {
        depHour = atoi(conn.legs[0].rtDepartureTime);
        depMin = atoi(conn.legs[0].rtDepartureTime + 3);
    }
    struct tm* nowTm = localtime((time_t*)&currentTime);
    int nowMinutes = nowTm->tm_hour * 60 + nowTm->tm_min;
    int depMinutes = depHour * 60 + depMin;
    int minutesUntil = depMinutes - nowMinutes;
    if (minutesUntil < 0) minutesUntil += 24 * 60; // next day

    String inStr = "in " + String(minutesUntil) + " min";
    TextUtils::printTextAtTopMargin(x + MARGIN + COL_DEP_TIME, y, inStr.c_str());

    // Transfer stations (one per line)
    if (conn.legCount > 1) {
        int16_t transferX = x + MARGIN + COL_LINES;
        for (int leg = 0; leg < conn.legCount - 1; leg++) {
            // Transfer station = destination of current leg
            String station = Util::shortenStationName(String(conn.legs[leg].direction));

            // Calculate transfer time (next leg departure - this leg arrival)
            int arrMin = atoi(conn.legs[leg].arrivalTime) * 60 + atoi(conn.legs[leg].arrivalTime + 3);
            int nextDepMin = atoi(conn.legs[leg + 1].departureTime) * 60 +
                             atoi(conn.legs[leg + 1].departureTime + 3);
            int transferTime = nextDepMin - arrMin;
            if (transferTime < 0) transferTime += 24 * 60;

            String transferStr = "Umst: " + station + " (" + String(transferTime) + " min)";
            transferStr = TextUtils::shortenTextToFit(transferStr, w - COL_LINES - MARGIN);
            TextUtils::printTextAtTopMargin(transferX, y, transferStr.c_str());
            y += ROW_HEIGHT;
        }
    } else {
        y += ROW_HEIGHT; // Empty row 2 for direct connections
    }

    return y - startY;
}
