cmake_minimum_required(VERSION 3.14)
project(structurator VERSION 1.0.0 LANGUAGES CXX)


if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set(MAIN_PROJECT TRUE)
else()
    set(MAIN_PROJECT FALSE)
endif()


# options
option(STRUCTURATOR_TESTS "Build tests" ${MAIN_PROJECT})
option(STRUCTURATOR_EXAMPLES "Build examples" ${MAIN_PROJECT})
option(STRUCTURATOR_INSTALL "Provide install target" FALSE)


# library
file(GLOB_RECURSE STRUCTURATOR_SRC_FILES CONFIGURE_DEPENDS src/*.cpp)
add_library(${PROJECT_NAME} ${STRUCTURATOR_SRC_FILES})
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)
target_include_directories(${PROJECT_NAME} INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:src>)


if(NOT ${STRUCTURATOR_INSTALL})
    # tests
    if(STRUCTURATOR_TESTS)
	    add_subdirectory(tests)
    endif()


    # examples
    if(STRUCTURATOR_EXAMPLES)
        add_subdirectory(examples)
    endif()
endif()


# library installation
if(STRUCTURATOR_INSTALL)
	include(CMakePackageConfigHelpers)
	write_basic_package_version_file(${PROJECT_NAME}ConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion
        ARCH_INDEPENDENT)

    install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}Config)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake DESTINATION lib/cmake/${PROJECT_NAME})
    install(EXPORT ${PROJECT_NAME}Config NAMESPACE ${PROJECT_NAME}:: DESTINATION lib/cmake/${PROJECT_NAME})

    file(GLOB INSTALL_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/src/structurator/*.hpp)
    install(FILES ${INSTALL_HEADERS} DESTINATION include/${PROJECT_NAME})

    export(EXPORT ${PROJECT_NAME}Config NAMESPACE ${PROJECT_NAME}::)
endif()
