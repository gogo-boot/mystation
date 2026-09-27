// Compile the timing manager implementation as part of THIS test suite only.
// Kept out of the env-wide build_src_filter so it (and its suite-local
// mock_time.h) is not forced into other test suites' builds.
#include "../../src/util/timing_manager.cpp"
