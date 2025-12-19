// src/ota/version_helper.cpp
#include "ota/version_helper.h"
#include <cstring>
#include <cstdio>

SemanticVersion SemanticVersion::parse(const char* version_str) {
    SemanticVersion version;

    // Skip 'v' prefix if present
    const char* str = version_str;
    if (str[0] == 'v' || str[0] == 'V') {
        str++;
    }

    // Parse major.minor.patch
    sscanf(str, "%d.%d.%d", &version.major, &version.minor, &version.patch);

    return version;
}

bool SemanticVersion::isNewerThan(const SemanticVersion& other) const {
    if (major != other.major) {
        return major > other.major;
    }
    if (minor != other.minor) {
        return minor > other.minor;
    }
    return patch > other.patch;
}

std::string SemanticVersion::toString() const {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%d.%d.%d", major, minor, patch);
    return std::string(buffer);
}
