set(TARGET MethaneDataAnimation)

include(MethaneModules)

get_module_dirs("Methane/Data")

set(HEADERS
    ${INCLUDE_DIR}/Animation.h
    ${INCLUDE_DIR}/AnimationsPool.h
    ${INCLUDE_DIR}/TimeAnimation.hpp
    ${INCLUDE_DIR}/ValueAnimation.hpp
)

set(SOURCES
    ${SOURCES_DIR}/Animation.cpp
    ${SOURCES_DIR}/AnimationsPool.cpp
)

add_library(${TARGET} STATIC
    ${HEADERS}
    ${SOURCES}
)

target_include_directories(${TARGET}
    PRIVATE
        Sources
    PUBLIC
        Include
)

if(METHANE_PRECOMPILED_HEADERS_ENABLED)
    target_precompile_headers(${TARGET} REUSE_FROM MethaneCommonPrecompiledHeaders)
endif()

target_link_libraries(${TARGET}
    PUBLIC
        MethaneInstrumentation
    PRIVATE
        MethaneBuildOptions
        MethaneCommonPrecompiledHeaders
        MethanePrimitives
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADERS} ${SOURCES})

set_target_properties(${TARGET}
    PROPERTIES
        FOLDER Modules/Data
        PUBLIC_HEADER "${HEADERS}"
)

install(TARGETS ${TARGET}
    PUBLIC_HEADER
        DESTINATION ${INCLUDE_DIR}
        COMPONENT Development
    ARCHIVE
        DESTINATION Lib
        COMPONENT Development
)
