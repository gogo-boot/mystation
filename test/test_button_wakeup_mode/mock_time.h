#ifndef MOCK_TIME_H
#define MOCK_TIME_H
#include <ctime>

class MockTime {
private:
    static time_t mockCurrentTime;
    static bool useMock;
public:
    static void setMockTime(time_t time) { mockCurrentTime = time; useMock = true; }
    static void useRealTime() { useMock = false; }
    static time_t now() { return useMock ? mockCurrentTime : time(nullptr); }
};

#endif

