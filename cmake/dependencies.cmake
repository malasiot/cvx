#openmp

find_package(OpenMP)

if (OPENMP_FOUND)
	set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

find_package(OpenCV 4 REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(ZLIB REQUIRED)

find_package(F2C REQUIRED)
find_package(LAPACK REQUIRED)

find_package(SQLite3)
