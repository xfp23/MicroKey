
file(GLOB_RECURSE MICROKEY_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

add_library(MICROKEY STATIC
    ${MICROKEY_SOURCES}
)


target_include_directories(MICROKEY PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(MICROKEY PRIVATE
    "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>"
)