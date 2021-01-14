file(GLOB PROJECT_TEST_CPPS *.cpp)

foreach(_SOURCE ${PROJECT_TEST_CPPS})
   get_filename_component(_NAME ${_SOURCE} NAME_WE)
   add_executable        (${_NAME} ${_SOURCE})
   target_link_libraries (${_NAME} cvx)
endforeach()


