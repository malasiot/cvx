
if ( NOT LAPACK_FOUND )
    message("Lapack not found. levmar will be build without lapack")
else ( NOT LAPACK_FOUND)
    add_definitions(-DHAVE_LAPACK)
endif ( NOT LAPACK_FOUND )

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# levmar library source files
add_library(levmar STATIC
  lm.c Axb.c misc.c lmlec.c lmbc.c lmblec.c lmbleic.c
  levmar.h misc.h compiler.h
)
set_property(TARGET levmar PROPERTY POSITION_INDEPENDENT_CODE ON)

install(TARGETS levmar
  EXPORT ${PROJECT_NAME}-export
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  COMPONENT dev)

#link_libraries(levmar ${LAPACK_LIBRARIES} ${F2C_LIBRARIES})
