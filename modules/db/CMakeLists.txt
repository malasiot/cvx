FIND_PACKAGE(ICU  COMPONENTS uc i18n data REQUIRED)

set(PostgreSQL_ADDITIONAL_VERSIONS "9.3")
FIND_PACKAGE(PostgreSQL)

INCLUDE_DIRECTORIES(
       ${CVX_INCLUDE_DIRS}
        include
        src/3rdparty/sqlite3
        ${PostgreSQL_INCLUDE_DIRS}
)

ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(test)


