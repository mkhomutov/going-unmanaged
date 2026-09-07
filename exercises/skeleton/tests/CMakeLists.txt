# The test binary: links the code under test, runs under CTest, and reads
# its verdict from the exit code (Chapter 28).
add_executable(session_test session_test.cpp)
target_link_libraries(session_test PRIVATE myplugin::myplugin)
if(MSVC)
    target_compile_options(session_test PRIVATE /W4)
else()
    target_compile_options(session_test PRIVATE -Wall -Wextra)
endif()
add_test(NAME session_test COMMAND session_test)
# Appendix J: a UBSan finding fails the test rather than printing and
# exiting 0, and a hang fails with a name instead of stopping the job.
set_tests_properties(session_test PROPERTIES
    ENVIRONMENT "UBSAN_OPTIONS=halt_on_error=1"
    TIMEOUT 60)
