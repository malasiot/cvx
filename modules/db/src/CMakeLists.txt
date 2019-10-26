SET( SOURCES
    
    connection.cpp
    driver_factory.cpp
    exception.cpp
    statement.cpp
    transaction.cpp
    query.cpp
    query_result.cpp
    statement_handle.cpp

    drivers/sqlite/driver.cpp
    drivers/sqlite/connection.cpp
    drivers/sqlite/exceptions.cpp
    drivers/sqlite/statement.cpp
    drivers/sqlite/query_result.cpp

    3rdparty/sqlite3/sqlite3.c
)

set_source_files_properties(3rdparty/sqlite3/sqlite3.c PROPERTIES COMPILE_FLAGS "-DSQLITE_ENABLE_FTS3 -DSQLITE_ENABLE_ICU -DSQLITE_ENABLE_RTREE")

IF ( PostgreSQL_FOUND )
    ADD_DEFINITIONS("-DHAS_PGSQL_DRIVER")
    LIST(APPEND UTIL_SOURCES
        drivers/pgsql/driver.cpp
        drivers/pgsql/connection.cpp
        drivers/pgsql/exceptions.cpp
        drivers/pgsql/statement.cpp
        drivers/pgsql/query_result.cpp
        drivers/pgsql/parameters.cpp
)
ENDIF ( PostgreSQL_FOUND )

ADD_LIBRARY(cvx_db SHARED ${SOURCES})
TARGET_LINK_LIBRARIES(cvx_db ${SQLITE3_LIBRARY} ${PostgreSQL_LIBRARIES} ${ICU_LIBRARIES} ICU::uc ICU::i18n ICU::data dl z pthread )


