#include "display/trip_display.h"
#include "display/text_utils.h"
#include "config/config_manager.h"
#include "util/util.h"
#include "global_instances.h"
#include <esp_log.h>
#include <time.h>

static const char* TAG = "TRIP_DISPLAY";

static constexpr int16_t ROW_HEIGHT = 18;
static constexpr int16_t CONNECTION_HEIGHT = 75; // 480px - 28px header = 452px / 6 connections
static constexpr int16_t HEADER_HEIGHT = 30;
static constexpr int16_t MARGIN = 5;
static constexpr int16_t COL_LINES = 70;

String TripDisplay::extractStopName(const char* stopId) {
    String id = String(stopId);
    int oStart = id.indexOf("@O=");
    if (oStart < 0) return id;
    oStart += 3;
    int oEnd = id.indexOf('@', oStart);
    if (oEnd < 0) return id.substring(oStart);
    return id.substring(oStart, oEnd);
}

int16_t TripDisplay::getConnectionHeight(const TripConnection& conn) {
    return CONNECTION_HEIGHT; // Fixed 3-row height
}

void TripDisplay::drawTripConnections(const TripData& tripData, int16_t x, int16_t y, int16_t w, int16_t h) {
    ESP_LOGI(TAG, "Drawing %d trip connections at (%d,%d) %dx%d", tripData.connectionCount, x, y, w, h);

    RTCConfigData& config = ConfigManager::getConfig();

    // Header: Origin → Destination (strip city name)
    TextUtils::setFont12px_margin15px();
    String originFull = extractStopName(config.selectedStopId);
    String destFull = extractStopName(config.tripDestId);
    // Use the full destination as reference to strip city from origin, and vice versa
    String origin = Util::shortenDestination(destFull, originFull);
    if (origin.isEmpty()) origin = originFull;
    origin = Util::shortenStationName(origin);

    String dest = Util::shortenDestination(originFull, destFull);
    if (dest.isEmpty()) dest = destFull;
    dest = Util::shortenStationName(dest);
    String header = origin + " -> " + dest;
    header = TextUtils::shortenTextToFit(header, w - MARGIN * 2);
    TextUtils::printTextAtTopMargin(x + MARGIN, y, header);
    y += 24; // More spacing after header

    // Separator line
    display.drawFastHLine(x, y, w, GxEPD_BLACK);
    y += 4;

    // Current time for "in X min"
    time_t now;
    time(&now);
    uint32_t currentTime = (uint32_t)now;

    // Draw connections
    TextUtils::setFont10px_margin12px();
    for (int i = 0; i < tripData.connectionCount; i++) {
        if (y + CONNECTION_HEIGHT > h) break;

        drawSingleConnection(tripData.connections[i], x, y, w, CONNECTION_HEIGHT, currentTime);
        y += CONNECTION_HEIGHT;

        // Separator
        display.drawFastHLine(x + MARGIN, y - 4, w - MARGIN * 2, GxEPD_BLACK);
    }

    if (tripData.connectionCount == 0) {
        TextUtils::printTextAtTopMargin(x + MARGIN, y + 10, "Keine Verbindungen gefunden");
    }
}

int16_t TripDisplay::drawSingleConnection(const TripConnection& conn, int16_t x, int16_t y,
                                           int16_t w, int16_t maxH, uint32_t currentTime) {
    int16_t rightEdge = x + w - MARGIN;
    int16_t leftX = x + MARGIN;
    RTCConfigData& config = ConfigManager::getConfig();
    String originFull = extractStopName(config.selectedStopId);

    // === ROW 1: dep_time [+delay]  [line1] -O- [line2]  arr_time [+delay]  duration ===
    int16_t row1Y = y + 10;

    // Fixed column positions for vertical alignment across all connections
    int16_t colDepTime = leftX;
    int16_t colDelay = leftX + 38;  // gap after "23:06"
    int16_t colLines = leftX + COL_LINES;
    int16_t arrDelayWidth = TextUtils::getTextWidth("+00") + 3;
    int16_t arrTimeWidth = TextUtils::getTextWidth("23:06");
    int16_t colArrTime = rightEdge - arrDelayWidth - arrTimeWidth - 3;
    int16_t colArrDelay = colArrTime + arrTimeWidth + 3;

    // Check if any leg is cancelled
    bool hasCancelled = false;
    for (int leg = 0; leg < conn.legCount; leg++) {
        if (conn.legs[leg].cancelled) { hasCancelled = true; break; }
    }

    // Departure time (strikethrough if cancelled)
    if (hasCancelled) {
        TextUtils::printStrikethroughTextAtTopMargin(colDepTime, row1Y, conn.legs[0].departureTime);
    } else {
        TextUtils::printTextAtTopMargin(colDepTime, row1Y, conn.legs[0].departureTime);
    }

    // Departure delay (more space from time)
    if (conn.legs[0].rtDepartureTime[0] != '\0' &&
        strcmp(conn.legs[0].rtDepartureTime, conn.legs[0].departureTime) != 0) {
        int schedMin = atoi(conn.legs[0].departureTime) * 60 + atoi(conn.legs[0].departureTime + 3);
        int rtMin = atoi(conn.legs[0].rtDepartureTime) * 60 + atoi(conn.legs[0].rtDepartureTime + 3);
        int delay = rtMin - schedMin;
        if (delay > 0) {
            String delayStr = "+" + String(delay);
            TextUtils::printTextAtTopMargin(colDelay, row1Y, delayStr.c_str());
        }
    }

    // Line boxes with -O- transfer symbols
    int16_t currentX = colLines;
    for (int leg = 0; leg < conn.legCount; leg++) {
        if (leg > 0) {
            TextUtils::printTextAtTopMargin(currentX, row1Y, "-O-");
            currentX += TextUtils::getTextWidth("-O-") + 2;
        }
        String line = String(conn.legs[leg].line);
        int16_t lineW = TextUtils::getTextWidth(line) + 6;
        // Don't overflow into arrival time area
        if (currentX + lineW > colArrTime - 5) break;
        display.drawRect(currentX, row1Y - 1, lineW, ROW_HEIGHT - 4, GxEPD_BLACK);
        TextUtils::printTextAtTopMargin(currentX + 3, row1Y, line.c_str());
        currentX += lineW + 4;
    }

    // For single-leg connections, show direction after line box
    if (conn.legCount == 1 && conn.legs[0].direction[0] != '\0') {
        String dir = Util::shortenDestination(originFull, String(conn.legs[0].direction));
        if (dir.isEmpty()) dir = Util::shortenStationName(String(conn.legs[0].direction));
        int16_t dirMaxWidth = colArrTime - currentX - 8;
        if (dirMaxWidth > 20) {
            dir = TextUtils::shortenTextToFit(dir, dirMaxWidth);
            TextUtils::printTextAtTopMargin(currentX, row1Y, dir.c_str());
        }
    }

    // Arrival time (fixed position, right side of row 1)
    const TripLeg& lastLeg = conn.legs[conn.legCount - 1];
    if (hasCancelled) {
        TextUtils::printStrikethroughTextAtTopMargin(colArrTime, row1Y, lastLeg.arrivalTime);
    } else {
        TextUtils::printTextAtTopMargin(colArrTime, row1Y, lastLeg.arrivalTime);
    }

    // Arrival delay
    if (lastLeg.rtArrivalTime[0] != '\0' &&
        strcmp(lastLeg.rtArrivalTime, lastLeg.arrivalTime) != 0) {
        int schedMin = atoi(lastLeg.arrivalTime) * 60 + atoi(lastLeg.arrivalTime + 3);
        int rtMin = atoi(lastLeg.rtArrivalTime) * 60 + atoi(lastLeg.rtArrivalTime + 3);
        int delay = rtMin - schedMin;
        if (delay > 0) {
            String delayStr = "+" + String(delay);
            TextUtils::printTextAtTopMargin(colArrDelay, row1Y, delayStr.c_str());
        }
    }

    // === ROW 2: "in X min" + Transfer 1 ===
    int16_t row2Y = row1Y + ROW_HEIGHT;

    String inStr;
    if (hasCancelled) {
        inStr = "Fällt aus";
    } else {
        int depHour = atoi(conn.legs[0].departureTime);
        int depMin = atoi(conn.legs[0].departureTime + 3);
        if (conn.legs[0].rtDepartureTime[0] != '\0') {
            depHour = atoi(conn.legs[0].rtDepartureTime);
            depMin = atoi(conn.legs[0].rtDepartureTime + 3);
        }
        struct tm* nowTm = localtime((time_t*)&currentTime);
        int nowMinutes = nowTm->tm_hour * 60 + nowTm->tm_min;
        int depMinutes = depHour * 60 + depMin;
        int minutesUntil = depMinutes - nowMinutes;
        if (minutesUntil < 0) minutesUntil += 24 * 60;
        inStr = "in " + String(minutesUntil) + " min";
    }
    inStr = TextUtils::shortenTextToFit(inStr, COL_LINES - 5);
    TextUtils::printTextAtTopMargin(leftX, row2Y, inStr.c_str());
    int16_t transferColX = leftX + COL_LINES; // Align with line boxes above

    // Duration (right-aligned on row 2, always shown)
    String durStr = String(conn.durationMinutes) + " min";
    int16_t durW = TextUtils::getTextWidth(durStr);
    TextUtils::printTextAtTopMargin(rightEdge - durW, row2Y, durStr.c_str());

    // Transfer 1 (if exists)
    if (conn.legCount > 1) {
        int arrMin = atoi(conn.legs[0].arrivalTime) * 60 + atoi(conn.legs[0].arrivalTime + 3);
        int nextDepMin = atoi(conn.legs[1].departureTime) * 60 + atoi(conn.legs[1].departureTime + 3);
        int transferTime = nextDepMin - arrMin;
        if (transferTime < 0) transferTime += 24 * 60;

        String station = Util::shortenDestination(originFull, String(conn.legs[0].arrivalStation));
        String transferStr = "Umst: " + station + " (" + String(transferTime) + " min)";
        int16_t maxTransferW = (x + w) - transferColX - MARGIN;
        transferStr = TextUtils::shortenTextToFit(transferStr, maxTransferW);
        TextUtils::printTextAtTopMargin(transferColX, row2Y, transferStr.c_str());
    }

    // === ROW 3: Transfer 2+ (combined on one line if multiple) ===
    int16_t row3Y = row2Y + ROW_HEIGHT;

    if (conn.legCount > 2) {
        String combined = "";
        for (int leg = 1; leg < conn.legCount - 1; leg++) {
            int arrMin = atoi(conn.legs[leg].arrivalTime) * 60 + atoi(conn.legs[leg].arrivalTime + 3);
            int nextDepMin = atoi(conn.legs[leg + 1].departureTime) * 60 +
                             atoi(conn.legs[leg + 1].departureTime + 3);
            int transferTime = nextDepMin - arrMin;
            if (transferTime < 0) transferTime += 24 * 60;

            String station = Util::shortenDestination(originFull, String(conn.legs[leg].arrivalStation));
            if (combined.length() > 0) combined += ", ";
            combined += station + " (" + String(transferTime) + " min)";
        }
        String transferStr = "Umst: " + combined;
        int16_t maxTransferW = (x + w) - transferColX - MARGIN;
        transferStr = TextUtils::shortenTextToFit(transferStr, maxTransferW);
        TextUtils::printTextAtTopMargin(transferColX, row3Y, transferStr.c_str());
    }

    return CONNECTION_HEIGHT;
}
