include(${CMAKE_ROOT}/Modules/ExternalProject.cmake)

set(ExternalInstallDir "${PROJECT_BINARY_DIR}" CACHE INTERNAL "External install directory")	

ExternalProject_Add(
   Ext_PositionBasedDynamics
   PREFIX "${ExternalInstallDir}/PositionBasedDynamics"
   GIT_REPOSITORY https://github.com/InteractiveComputerGraphics/PositionBasedDynamics.git
   INSTALL_DIR ${ExternalInstallDir}/PositionBasedDynamics
   CMAKE_ARGS -DBUILD_SHARED_LIBS:BOOL=ON -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE} -DCMAKE_INSTALL_PREFIX:PATH=${ExternalInstallDir}/PositionBasedDynamics -DBUILD_CMD_EXECUTABLE:BOOL=0 -DPBD_NO_DEMOS:BOOL=ON -DEIGEN3_INCLUDE_DIR:PATH=${EIGEN3_INCLUDE_DIR}
   PATCH_COMMAND git apply ${CMAKE_CURRENT_LIST_DIR}/pbd_fix.patch
   UPDATE_COMMAND ""
) 


## Discregrid
ExternalProject_Add(
   Ext_Discregrid
   PREFIX "${ExternalInstallDir}/Discregrid"
   GIT_REPOSITORY https://github.com/InteractiveComputerGraphics/Discregrid.git
   GIT_TAG "c0fb5aeac4c8a83e9f37c720315f13a834409b81"
   INSTALL_DIR ${ExternalInstallDir}/Discregrid
   CMAKE_ARGS -DBUILD_SHARED_LIBS:BOOL=ON -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE} -DCMAKE_INSTALL_PREFIX:PATH=${ExternalInstallDir}/Discregrid -DBUILD_CMD_EXECUTABLE:BOOL=0 -DEIGEN3_INCLUDE_DIR:PATH=${EIGEN3_INCLUDE_DIR}
   UPDATE_COMMAND ""
)

## GenericParameters
ExternalProject_Add(
   Ext_GenericParameters
   PREFIX "${ExternalInstallDir}/GenericParameters"
   GIT_REPOSITORY https://github.com/InteractiveComputerGraphics/GenericParameters.git
   GIT_TAG "b1ad669fac8d106515f6aa8514a03598d5766a36"
   INSTALL_DIR ${ExternalInstallDir}/GenericParameters
   CMAKE_ARGS -DBUILD_SHARED_LIBS:BOOL=ON -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_INSTALL_PREFIX:PATH=${ExternalInstallDir}/GenericParameters -DGENERICPARAMETERS_NO_TESTS:BOOL=1
   UPDATE_COMMAND ""
)

ADD_LIBRARY( pbd SHARED IMPORTED )
SET_TARGET_PROPERTIES( pbd PROPERTIES
                                  IMPORTED_LOCATION "${ExternalInstallDir}/PositionBasedDynamics/lib/libPositionBasedDynamics_d.so"
                                  LINKER_LANGUAGE "CXX")

ADD_LIBRARY( pbdSimulation SHARED IMPORTED )
SET_TARGET_PROPERTIES( pbdSimulation PROPERTIES
              IMPORTED_LOCATION "${ExternalInstallDir}/PositionBasedDynamics/lib/libSimulation_d.so"
            LINKER_LANGUAGE "CXX")

        ADD_LIBRARY( pbdUtils SHARED IMPORTED )
        SET_TARGET_PROPERTIES( pbdUtils PROPERTIES
                      IMPORTED_LOCATION "${ExternalInstallDir}/PositionBasedDynamics/lib/libUtils_d.so"
                    LINKER_LANGUAGE "CXX")

include_directories(${ExternalInstallDir}/PositionBasedDynamics/include/
    ${ExternalInstallDir}/Discregrid/include/
    ${ExternalInstallDir}/GenericParameters/include/)

#link_directories( ${ExternalInstallDir}/PositionBasedDynamics/lib/
#    ${ExternalInstallDir}/Discregrid/lib/
#   )

SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_RPATH}:${ExternalInstallDir}/PositionBasedDynamics/lib/")

add_executable(test_pbd_cloth test_pbd_cloth.cpp )
add_dependencies(test_pbd_cloth Ext_PositionBasedDynamics Ext_Discregrid Ext_GenericParameters)
target_link_libraries(test_pbd_cloth cvx_viz pbd pbdSimulation pbdUtils
    "${ExternalInstallDir}/Discregrid/lib/libDiscregrid.a")




