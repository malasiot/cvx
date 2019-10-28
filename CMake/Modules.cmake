set(MODULES_SRC_DIR "${CMAKE_SOURCE_DIR}/modules")
file(GLOB MODULES RELATIVE ${MODULES_SRC_DIR} "${MODULES_SRC_DIR}/*")

macro(DeclareModule)
    cmake_parse_arguments(MODULE_ARGS "" "NAME;BUILD" "DEPENDS" ${ARGN} )

    if ( NOT MODULE_ARGS_NAME )
           message(FATAL_ERROR "You must provide a name")
    endif ( NOT MODULE_ARGS_NAME )

    string(TOUPPER ${MODULE_ARGS_NAME} module_uc)

    SET(module_depends "")
    foreach ( depends_on ${MODULE_ARGS_DEPENDS} )
        string(TOUPPER ${depends_on} depends_on_uc)
        list(APPEND module_depends "BUILD_MODULE_${depends_on_uc}")
    endforeach()

    If (NOT module_depends)
        set(module_depends "TRUE")
    endif()

    if ( EXISTS ${MODULES_SRC_DIR}/${MODULE_ARGS_NAME}/CMakeLists.txt )
        cmake_dependent_option(BUILD_MODULE_${module_uc} "Build module ${module}" ${MODULE_ARGS_BUILD} "${module_depends}" OFF)
        if ( BUILD_MODULE_${module_uc} )
            message("Configuring module ${MODULE_ARGS_NAME} ...")
            set(MODULE_${module_uc}_LOCATION ${MODULES_SRC_DIR}/${MODULE_ARGS_NAME})
            list(APPEND CVX_INCLUDE_DIRS ${MODULES_SRC_DIR}/${MODULE_ARGS_NAME}/include)
            list(APPEND BUILD_MODULES ${module_uc})
            add_subdirectory(${MODULE_${module_uc}_LOCATION})
        endif()
    endif()

endmacro(DeclareModule)
