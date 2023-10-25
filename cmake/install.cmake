# This "exports" all targets which have been put into the export set

if(WIN32 AND NOT CYGWIN)
  set(DEF_INSTALL_CMAKE_DIR CMake)
else()
  set(DEF_INSTALL_CMAKE_DIR lib/cmake/${PROJECT_NAME})
endif()
set(INSTALL_CMAKE_DIR ${DEF_INSTALL_CMAKE_DIR}
    CACHE PATH "Relative instalation path for CMake files")

# Path of the CMake files generated
set(PROJECT_CMAKE_FILES ${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY})

install(EXPORT ${PROJECT_NAME}-export
  DESTINATION ${INSTALL_CMAKE_DIR}
  FILE ${PROJECT_NAME}Targets.cmake)

include(CMakePackageConfigHelpers)
# generate the config file that includes the exports
configure_package_config_file(${CMAKE_SOURCE_DIR}/cmake/Config.cmake.in
  "${PROJECT_CMAKE_FILES}/${PROJECT_NAME}Config.cmake"
  INSTALL_DESTINATION ${INSTALL_CMAKE_DIR}
  NO_SET_AND_CHECK_MACRO
  NO_CHECK_REQUIRED_COMPONENTS_MACRO
  )

# Create the <package>Config.cmake.in
#configure_file(${CMAKE_SOURCE_DIR}/cmake/Config.cmake.in
#  "${PROJECT_CMAKE_FILES}/${PROJECT_NAME}Config.cmake" @ONLY)

# Create the <package>ConfigVersion.cmake.in
configure_file(${CMAKE_SOURCE_DIR}/cmake/ConfigVersion.cmake.in
  "${PROJECT_CMAKE_FILES}/${PROJECT_NAME}ConfigVersion.cmake" @ONLY)

# Install <package>Config.cmake and <package>ConfigVersion.cmake files
install(FILES
  "${PROJECT_CMAKE_FILES}/${PROJECT_NAME}Config.cmake"
  "${PROJECT_CMAKE_FILES}/${PROJECT_NAME}ConfigVersion.cmake"
  DESTINATION "${INSTALL_CMAKE_DIR}" COMPONENT dev)

# Uninstall targets
configure_file("${CMAKE_SOURCE_DIR}/cmake/Uninstall.cmake.in"
  "${PROJECT_CMAKE_FILES}/Uninstall.cmake"
  IMMEDIATE @ONLY)

add_custom_target(uninstall
  COMMAND ${CMAKE_COMMAND} -P ${PROJECT_CMAKE_FILES}/Uninstall.cmake)
