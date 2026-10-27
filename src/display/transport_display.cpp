#include "display/transport_display.h"
#include "display/text_utils.h"
#include "util/util.h"
#include "util/time_manager.h"
#include "util/battery_manager.h"
#include "display/common_footer.h"
#include <esp_log.h>
#include <vector>
#include <algorithm>
#include <climits>
#include <icons.h>
#include <WiFi.h>

#include "config/config_manager.h"
#include "global_instances.h"

static const char* TAG = "TRANSPORT_DISPLAY";

// Display layout constants
namespace TransportDisplayConstants {
    constexpr int16_t MARGIN = 10;
    constexpr int16_t STATION_NAME_HEIGHT = 17;
    constexpr int16_t STATION_NAME_SPACING = 20;
    constexpr int16_t COLUMN_HEADER_HEIGHT = 12;
    constexpr int16_t COLUMN_HEADER_SPACING = 5;
    constexpr int8_t SEPARATOR_PADDING = 9;
    constexpr int16_t ENTRY_HEIGHT = 42;
    constexpr int16_t ENTRY_TOP_PADDING = 3;
    constexpr int16_t ENTRY_LINE_HEIGHT = 17;
    constexpr int16_t ENTRY_BOTTOM_PADDING = 3;
    constexpr int8_t COLUMN_PADDING = 10;
    constexpr int8_t INFO_INDENT = 10;
    constexpr int16_t TRACK_RIGHT_PADDING = 15;
    constexpr int16_t FULL_SCREEN_STATION_SPACING = 25;
}


using namespace TransportDisplayConstants;

namespace {
    // Countdown threshold: departures further away than this (in minutes) show no countdown.
    constexpr int COUNTDOWN_THRESHOLD_MIN = 99;

    // Compute minutes from "now" (minutes-since-midnight) until an "HH:MM" time.
    // Returns -1 if the time string is invalid. "now" is captured once per render
    // by the caller, so this helper never reads the clock itself (avoids dozens
    // of redundant getCurrentLocalTime() calls during sort + draw).
    // Handles the midnight wrap (e.g. now 23:55, departure 00:10 -> 15 min).
    int minutesUntil(const String& hhmm, int nowMinutes) {
        if (hhmm.length() < 5) return -1;

        int depHour = hhmm.substring(0, 2).toInt();
        int depMin = hhmm.substring(3, 5).toInt();
        int depTotal = depHour * 60 + depMin;

        int diff = depTotal - nowMinutes;
        if (diff < 0) diff += 24 * 60; // wrap past midnight
        return diff;
    }

    // Read the clock once and return minutes-since-midnight, or -1 if unavailable.
    int currentMinutesSinceMidnight() {
        tm timeinfo;
        if (!TimeManager::getCurrentLocalTime(timeinfo)) return -1;
        return timeinfo.tm_hour * 60 + timeinfo.tm_min;
    }
} // namespace

void TransportDisplay::drawHalfScreenTransportSection(const DepartureData& departures, int16_t x, int16_t y, int16_t w,
                                                      int16_t h) {
    ESP_LOGI(TAG, "Drawing transport section at (%d, %d) with size %dx%d", x, y, w, h);
    int16_t currentY = y; // Start from actual top
    int16_t leftMargin = x + MARGIN;
    int16_t rightMargin = x + w - MARGIN;

    // Station name with TRUE 15px margin from top
    TextUtils::setFont14px_margin17px(); // Medium font for station name
    String stopName = ConfigManager::getStopNameFromId();
    stopName = Util::Util::shortenStationName(stopName);

    // Calculate available width and fit station name
    int stationMaxWidth = rightMargin - leftMargin;
    String fittedStopName = TextUtils::shortenTextToFit(stopName, stationMaxWidth);

    // Print station name at top margin
    TextUtils::printTextAtTopMargin(leftMargin, currentY, fittedStopName);

    currentY += STATION_NAME_HEIGHT;
    currentY += STATION_NAME_SPACING;

    drawHalfScreenTransports(departures, leftMargin, rightMargin, currentY, h - currentY);
}

void TransportDisplay::drawHalfScreenTransports(const DepartureData& departures, int16_t leftMargin,
                                                int16_t rightMargin, int16_t currentY, int16_t h) {
    // Half screen mode: Separate by direction flag
    ESP_LOGI(TAG, "Drawing transports separated by direction flag");

    std::vector<const DepartureInfo*> direction1Departures;
    std::vector<const DepartureInfo*> direction2Departures;
    getSeparatedTransportDirection(departures, direction1Departures, direction2Departures);

    ESP_LOGI(TAG, "Found %d transports for direction 1, %d for direction 2",
             direction1Departures.size(), direction2Departures.size());

    // Draw separator line between directions
    int16_t halfHeightY = currentY + h / 2;
    ESP_LOGI(TAG, "Drawing transport direction separator line at Y=%d", halfHeightY);

    display.drawLine(leftMargin, halfHeightY + SEPARATOR_PADDING, rightMargin, halfHeightY + SEPARATOR_PADDING,
                     GxEPD_BLACK);

    constexpr int maxPerDirection = 5;

    drawTransportList(direction1Departures, leftMargin, currentY, rightMargin - leftMargin, h - currentY,
                      true, maxPerDirection);

    currentY = halfHeightY + SEPARATOR_PADDING; // Reset currentY to halfHeightY for direction 2
    drawTransportList(direction2Departures, leftMargin, currentY, rightMargin - leftMargin, h - currentY,
                      false, maxPerDirection);
}

void TransportDisplay::drawTransportList(std::vector<const DepartureInfo*> departure, int16_t x, int16_t y, int16_t w,
                                         int16_t h, bool printLabel, int maxPerDirection) {
    // Capture "now" once for the whole list: it does not change during a render,
    // so reading the clock per sort-comparison and per row is wasteful.
    const int nowMinutes = currentMinutesSinceMidnight();

    // Sort by effective departure time (soonest first). RMV returns results in
    // scheduled-time order, so a heavily delayed train can be out of place
    // relative to its real-time departure. minutesUntil() handles midnight wrap;
    // invalid/unparseable times (-1) are pushed to the end.
    std::sort(departure.begin(), departure.end(),
              [nowMinutes](const DepartureInfo* a, const DepartureInfo* b) {
                  String ta = (a->rtTime.length() > 0) ? a->rtTime : a->time;
                  String tb = (b->rtTime.length() > 0) ? b->rtTime : b->time;
                  int ma = minutesUntil(ta, nowMinutes);
                  int mb = minutesUntil(tb, nowMinutes);
                  if (ma < 0) ma = INT_MAX;
                  if (mb < 0) mb = INT_MAX;
                  return ma < mb;
              });

    if (printLabel) {
        // Column headers with TRUE 12px margin from current position
        TextUtils::setFont10px_margin12px(); // Small font for column headers
        TextUtils::printTextAtTopMargin(x, y, "Zeit         In    Linie    Ziel");

        y += COLUMN_HEADER_HEIGHT;
        y += COLUMN_HEADER_SPACING;

        // Underline
        display.drawLine(x, y, x + w, y, GxEPD_BLACK);
    }

    // Check if there are no departures
    if (departure.empty()) {
        TextUtils::setFont10px_margin12px();
        TextUtils::printTextAtTopMargin(x, y, "Keine Abfahrten geplant");
        return;
    }

    // Direction 1 transports
    for (int i = 0; i < min(maxPerDirection, (int)departure.size()); i++) {
        const auto& dep = *departure[i];
        drawSingleTransport(dep, x, w, y, nowMinutes);
        y += ENTRY_HEIGHT;

        if (y > display.height()) {
            ESP_LOGW(TAG, "Reached end of section height while drawing transports");
            break; // Stop if we exceed the section height
        }
    }
}

void TransportDisplay::getSeparatedTransportDirection(const DepartureData& departures,
                                                      std::vector<const DepartureInfo*>& direction1Departures,
                                                      std::vector<const DepartureInfo*>& direction2Departures) {
    auto getDirectionNumber = [](const String& flag) -> int {
        if (flag == "1" || flag.toInt() == 1) return 1;
        if (flag == "2" || flag.toInt() == 2) return 2;
        return 0;
    };

    for (int i = 0; i < departures.departureCount; i++) {
        const auto& dep = departures.departures[i];
        int direction = getDirectionNumber(dep.directionFlag);

        if (direction == 1) {
            direction1Departures.push_back(&dep);
        } else if (direction == 2) {
            direction2Departures.push_back(&dep);
        }
    }
}

void TransportDisplay::drawFullScreenTransportSection(const DepartureData& departures, int16_t x, int16_t y, int16_t w,
                                                      int16_t h) {
    // Full screen mode: Separate by direction flag
    ESP_LOGI(TAG, "drawFullScreenTransportSection at (%d, %d) with size %dx%d", x, y, w, h);
    int16_t currentY = y; // Start from actual top
    const int16_t leftMargin = x + MARGIN;
    const int16_t rightMargin = x + w - MARGIN;

    // Station name with TRUE 15px margin from top
    TextUtils::setFont14px_margin17px(); // Medium font for station name
    const String stopName = ConfigManager::getStopNameFromId();

    // Calculate available width and fit station name
    int stationMaxWidth = rightMargin - leftMargin;
    String fittedStopName = TextUtils::shortenTextToFit(stopName, stationMaxWidth);

    // Print station name at top margin
    TextUtils::printTextAtTopMargin(leftMargin, currentY, fittedStopName);

    // Upper right corner: Time + Status icons (refresh, WiFi, battery)
    String statusText = "Stand ";
    String dateTime = TimeManager::getGermanDateTimeString();
    dateTime = statusText + dateTime;
    int16_t dateTimeWidth = TextUtils::getTextWidth(dateTime);

    const int16_t iconWidth = 24;
    const int16_t iconSpacing = 5; // Spacing between icons
    int16_t iconX = rightMargin; // Start from right edge and work backwards

    // Battery icon (rightmost)
    if (BatteryManager::isAvailable()) {
        int batteryLevel = BatteryManager::getBatteryIconLevel();
        if (batteryLevel > 0) {
            iconX -= iconWidth;
            icon_name batteryIcon = CommonFooter::getBatteryIcon();
            display.drawInvertedBitmap(iconX, currentY, getBitmap(batteryIcon, 24), 24, 24, GxEPD_BLACK);
            iconX -= iconSpacing;
        }
    }

    // WiFi icon
    iconX -= iconWidth;
    icon_name wifiIcon = CommonFooter::getWiFiIcon();
    display.drawInvertedBitmap(iconX, currentY, getBitmap(wifiIcon, 24), 24, 24, GxEPD_BLACK);
    iconX -= iconSpacing;

    // Refresh icon
    iconX -= iconWidth;
    display.drawInvertedBitmap(iconX, currentY, getBitmap(refresh, 24), 24, 24, GxEPD_BLACK);

    // Time (left of all icons)
    iconX -= (iconSpacing + dateTimeWidth);
    TextUtils::printTextAtTopMargin(iconX, currentY, dateTime);

    currentY += STATION_NAME_HEIGHT;
    currentY += FULL_SCREEN_STATION_SPACING;

    std::vector<const DepartureInfo*> direction1Departures;
    std::vector<const DepartureInfo*> direction2Departures;
    getSeparatedTransportDirection(departures, direction1Departures, direction2Departures);

    ESP_LOGI(TAG, "Found %d transports for direction 1, %d for direction 2",
             direction1Departures.size(), direction2Departures.size());

    // Draw first 4 transports from direction 1
    constexpr int maxPerDirection = 10;

    const int16_t halfWidth = display.width() / 2 - 1;
    drawTransportList(direction1Departures, x + MARGIN, currentY, halfWidth - MARGIN, h - currentY, true,
                      maxPerDirection);
    drawTransportList(direction2Departures, halfWidth + MARGIN, currentY, halfWidth - MARGIN, h - currentY,
                      true, maxPerDirection);
}

void TransportDisplay::drawSingleTransport(const DepartureInfo& dep, int16_t x, int16_t width,
                                           int16_t currentY, int nowMinutes) {
    // Log the transport position and size
    ESP_LOGI(TAG, "Drawing single transport at Y=%d", currentY);

    currentY += ENTRY_TOP_PADDING;
    // Half screen format with proper text positioning
    TextUtils::setFont10px_margin12px(); // Small font for transport entries

    // Clean up destination (remove "Frankfurt (Main)" prefix)
    const String stopName = ConfigManager::getStopNameFromId();
    String dest = Util::shortenDestination(stopName, dep.direction);

    // === Zeit column: effective departure time (+ plan-only marker) ===
    // rtTime present  -> live data, show the real (prognosed) departure.
    // rtTime empty    -> plan-only (no real-time feed), show scheduled time + "*".
    bool hasRealtime = (dep.rtTime.length() > 0);
    String effectiveTime = hasRealtime ? dep.rtTime : dep.time;
    String zeitText = effectiveTime.substring(0, 5); // "HH:MM"
    if (!hasRealtime) {
        zeitText += " *"; // mark as timetable-only, not live
    }

    // === In column: countdown in minutes, right-aligned, blank if > threshold ===
    String inText = "";
    int minsLeft = minutesUntil(effectiveTime, nowMinutes);
    if (minsLeft >= 0 && minsLeft <= COUNTDOWN_THRESHOLD_MIN) {
        inText = String(minsLeft) + "'";
    }

    // Fixed column widths. "In" is sized for the widest token ("888'").
    int16_t zeitColWidth = TextUtils::getTextWidth("88:88 *");
    int16_t inColWidth = TextUtils::getTextWidth("888'");

    int16_t currentX = x;

    // --- Zeit (left-aligned) ---
    if (dep.cancelled) {
        TextUtils::printStrikethroughTextAtTopMargin(currentX, currentY, zeitText.c_str());
    } else {
        TextUtils::printTextAtTopMargin(currentX, currentY, zeitText.c_str());
    }

    currentX += zeitColWidth + COLUMN_PADDING;

    // --- In (right-aligned within its column) ---
    if (inText.length() > 0) {
        int16_t inTextWidth = TextUtils::getTextWidth(inText);
        int16_t inX = currentX + inColWidth - inTextWidth; // right-align
        if (dep.cancelled) {
            TextUtils::printStrikethroughTextAtTopMargin(inX, currentY, inText.c_str());
        } else {
            TextUtils::printTextAtTopMargin(inX, currentY, inText.c_str());
        }
    }

    currentX += inColWidth + COLUMN_PADDING;

    // --- Linie ---
    int8_t lineWidth = TextUtils::getTextWidth("M888");
    TextUtils::printTextAtTopMargin(currentX, currentY, dep.line.c_str());
    currentX += COLUMN_PADDING + lineWidth;
    int16_t destMaxWidth = (x + width) - currentX;
    String fittedDest = TextUtils::shortenTextToFit(dest, destMaxWidth);
    TextUtils::printTextAtTopMargin(currentX, currentY, fittedDest.c_str());

    // Draw track info right-aligned
    int8_t trackWidth = TextUtils::getTextWidth(dep.track.c_str());
    currentX = x + width - trackWidth - TRACK_RIGHT_PADDING;
    TextUtils::printTextAtTopMargin(currentX, currentY, dep.track.c_str());

    currentY += ENTRY_LINE_HEIGHT;
    currentY += ENTRY_BOTTOM_PADDING;

    // Check if we have disruption information to display
    if (dep.cancelled) {
        TextUtils::printTextAtTopMargin(x + INFO_INDENT, currentY, "Fällt aus");
    } else if (dep.lead.length() > 0 || dep.text.length() > 0) {
        // Use the lead text if available, otherwise use text
        String disruptionInfo = dep.lead.length() > 0 ? dep.lead : dep.text;

        // Fit disruption text to available width
        int disruptionMaxWidth = width - INFO_INDENT;
        String fittedDisruption = TextUtils::shortenTextToFit(disruptionInfo, disruptionMaxWidth);

        // Display disruption information with proper positioning
        TextUtils::printTextAtTopMargin(x + INFO_INDENT, currentY, fittedDisruption);
    }
}

void TransportDisplay::drawTransportFooter(int16_t x, int16_t y, int16_t h) {
    // Use common footer with time and refresh icon
    CommonFooter::drawFooter(x, y, h, FOOTER_TIME | FOOTER_REFRESH | FOOTER_BATTERY);
}

