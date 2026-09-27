// Compile the solar math implementation as part of THIS test suite only.
// It is dependency-free, so including it here (rather than in the env-wide
// build_src_filter) keeps it out of other suites' builds — avoiding forcing
// unrelated sources (e.g. timing_manager.cpp and its suite-local mocks) into
// this suite's compilation.
#include "../../src/display/solar_math.cpp"
