if(NOT TARGET etherdream)
    find_package(etherdream REQUIRED)
endif()

# Target filtered platform-specific source files
file(GLOB_RECURSE ETHERDREAM_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
filter_platform_specific_files(ETHERDREAM_SOURCES)
add_platform_specific_files("${WIN32_SOURCES}" "${LINUX_SOURCES}")

if(MSVC)
	target_sources(${PROJECT_NAME} PRIVATE ${WIN32_SOURCES})
else()
	target_sources(${PROJECT_NAME} PRIVATE ${LINUX_SOURCES})
endif()

target_include_directories(${PROJECT_NAME} PUBLIC ${ETHERDREAM_INCLUDE_DIR})

target_link_libraries(${PROJECT_NAME} etherdreamlib)

if(WIN32)
    # Copy etherdream DLL to build directory on Windows (plus into packaged app)
    copy_etherdream_dll()
endif()

if(NAP_BUILD_CONTEXT MATCHES "framework_release")
    if(UNIX)
        # Install etherdream lib into packaged app
        install(FILES $<TARGET_FILE:etherdreamlib> DESTINATION lib)
    endif()
endif()
