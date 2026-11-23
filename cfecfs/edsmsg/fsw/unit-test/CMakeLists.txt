# Exercise the real dispatcher against the existing cFE, EDS, and missionlib stubs.
add_executable(edsmsg-dispatcher-test
    edsmsg_dispatcher_test.c
    ../src/edsmsg_dispatcher.c
)
target_include_directories(edsmsg-dispatcher-test PRIVATE
    $<TARGET_PROPERTY:edsmsg,INCLUDE_DIRECTORIES>
)
target_compile_options(edsmsg-dispatcher-test PRIVATE ${UT_COVERAGE_COMPILE_FLAGS})
target_link_libraries(edsmsg-dispatcher-test
    ${UT_COVERAGE_LINK_FLAGS}
    ut_core_api_stubs ut_missionlib_stubs ut_edslib_stubs
    cfe_missionlib_sb_dispatchdb_static cfe_edsdb_static ut_assert
)
add_test(edsmsg-dispatcher edsmsg-dispatcher-test)
