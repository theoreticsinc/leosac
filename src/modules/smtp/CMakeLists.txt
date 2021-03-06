set(SMTP_BIN smtp)

set(SMTP_SRCS
        init.cpp
        SMTPModule.cpp
        SMTPConfig.cpp
        SMTPServerInfoSerializer.cpp
        SMTPAudit.cpp
        SMTPAuditSerializer.cpp
        SMTPServiceImpl.cpp
        )

# Database support
set(OdbCMake_ODB_HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/SMTPConfig.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/SMTPAudit.hpp
        )


set(LEOSAC_ODB_INCLUDE_DIRS
        ${CMAKE_SOURCE_DIR}/deps/zmqpp/src/
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/deps/flagset
        ${CMAKE_SOURCE_DIR}/deps/date/include
        ${ZEROMQ_INCLUDE_DIR}
        ${CMAKE_SOURCE_DIR}/src/db
        )

set(OdbCMake_SOURCES "")
odb_compile(OdbCMake_SOURCES
        FILES ${OdbCMake_ODB_HEADERS}
        MULTI_DATABASE dynamic
        DB pgsql sqlite
        STANDARD "c++14"
        GENERATE_QUERY GENERATE_SESSION GENERATE_SCHEMA
        SCHEMA_FORMAT embedded
        SCHEMA_NAME module_smtp
        PROFILE "boost/date-time" "boost/optional" "leosac-profile"
        INCLUDE_PREFIX modules/smtp
        INCLUDE ${LEOSAC_ODB_INCLUDE_DIRS}
        DEFAULT_POINTER "std::shared_ptr"
        PGSQL_SERVER_VERSION "9.1"
        OUTPUT_DIR ${CMAKE_BINARY_DIR}/odb_gen/modules/smtp
        )

add_library(${SMTP_BIN} SHARED
        ${SMTP_SRCS}
        ${OdbCMake_SOURCES}
        )

set_target_properties(${SMTP_BIN} PROPERTIES
        COMPILE_FLAGS "${MODULE_COMPILE_FLAGS}"
        )

target_include_directories(${SMTP_BIN} PUBLIC
        ${CMAKE_SOURCE_DIR}/src
        ${ODB_INCLUDE_DIRS}
        ${ODB_COMPILE_OUTPUT_DIR}
        )

target_link_libraries(${SMTP_BIN} curl websock-api)

install(TARGETS ${SMTP_BIN} DESTINATION ${LEOSAC_MODULE_INSTALL_DIR})
