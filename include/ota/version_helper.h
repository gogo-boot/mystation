// include/ota/version_helper.h
#ifndef VERSION_HELPER_H
#define VERSION_HELPER_H

#include <string>

struct SemanticVersion {
    int major;
    int minor;
    int patch;

    SemanticVersion(int maj = 0, int min = 0, int pat = 0)
        : major(maj), minor(min), patch(pat) {}

    // Parse "v0.3.0" or "0.3.0" format
    static SemanticVersion parse(const char* version_str);

    // Compare versions: returns true if this > other
    bool isNewerThan(const SemanticVersion& other) const;

    // Convert to string
    std::string toString() const;
};

#endif // VERSION_HELPER_H
