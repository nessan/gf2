cmake_minimum_required(VERSION 3.31)

# C++ module target for gf2
add_library(${PROJECT_NAME}_module)

target_sources(${PROJECT_NAME}_module
    PUBLIC FILE_SET CXX_MODULES FILES  gf2.cppm
)

target_compile_features(${PROJECT_NAME}_module PUBLIC cxx_std_20)

target_include_directories(${PROJECT_NAME}_module PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

add_library(${PROJECT_NAME}::module ALIAS ${PROJECT_NAME}_module)

# Installation
install(TARGETS ${PROJECT_NAME}_module
    EXPORT ${PROJECT_NAME}Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    FILE_SET CXX_MODULES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/modules
)
