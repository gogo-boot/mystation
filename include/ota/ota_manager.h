#pragma once

/**
 * OTA Update Manager
 *
 * Handles automatic OTA (Over-The-Air) firmware updates:
 * - Checks if current time matches configured OTA check time
 * - Executes OTA update process
 * - Tracks last OTA check to prevent duplicates
 */
namespace OTAManager {
    /**
     * Check if OTA update should run based on configuration
     * - Verifies OTA is enabled in config
     * - Checks if current time is within 1 minute of configured check time
     *
     * @return true if OTA update should run
     */
    bool shouldCheckForUpdate();

    /**
     * Check for and apply OTA update if available
     * - Runs OTA update task
     * - Updates last OTA check timestamp
     * - Device will restart if update is found and installed
     */
    void checkAndApplyUpdate();

    /**
     * Mark that the user has explicitly requested an OTA update via button.
     * When set, checkAndApplyUpdate() will bypass the scheduled time check
     * and the otaEnabled config flag, and run OTA immediately.
     * The flag is reset after use and does not persist across deep sleep.
     */
    void requestUpdateByUser();
}

