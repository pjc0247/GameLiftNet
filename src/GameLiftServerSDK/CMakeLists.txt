cmake_minimum_required(VERSION 2.8)
include(ExternalProject)

# Welcome to the GameLift Native SDK build process.
# You will need:
#    - Cmake
# -----------------------------
# If you want to switch between static and dynamic library builds while
# in Visual Studio, uncomment one of the following lines, then rebuild
# the solution.  Uncommenting this hard-codes the decision and ignores
# any value set at the command-line.

# SET(BUILD_SHARED_LIBS "0")  # Static libraries
# SET(BUILD_SHARED_LIBS "1")  # Dynamic libraries

# -----------------------------
# Initialize build and link variables
# -----------------------------

if (WIN32)
  SET(OS_DIR windows)
else()
  SET(OS_DIR linux)
  add_definitions(-std=c++11)
endif()

IF(CMAKE_BUILD_TYPE MATCHES DEBUG OR CMAKE_BUILD_TYPE MATCHES Debug)
   SET(BUILD_TYPE "debug")
ELSE()
   SET(BUILD_TYPE "release")
ENDIF()

message(STATUS "Build type is ${BUILD_TYPE}")

SET(DEPS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/dependencies)

SET(SOCKET_IO_CLIENT_INCLUDE_DIR ${DEPS_DIR}/socket.io-client-cpp/include)
SET(PROTOBUF_INCLUDE_DIR ${DEPS_DIR}/protobuf/include)

SET(GAMELIFT_PROTOBUF_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/protocol_buffer/generatedFiles)

SET(SOCKET_IO_CLIENT_LIB_DIR ${DEPS_DIR}/socket.io-client-cpp/lib/${OS_DIR}/${BUILD_TYPE})
SET(PROTOBUF_LIB_DIR ${DEPS_DIR}/protobuf/lib/${OS_DIR}/${BUILD_TYPE})

if(BUILD_SHARED_LIBS)
    message(STATUS "Configuring project to build Shared Objects/DLLs.")
    SET(LIBTYPE SHARED)
    add_definitions("-DUSE_IMPORT_EXPORT=1")
    set_target_properties(${LIBRARY_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
    SET(BOOST_LIB_DIR ${DEPS_DIR}/boost/lib/${OS_DIR}/${BUILD_TYPE})

    # Link to libs in these folders
    message(STATUS "linking to Boost in ${BOOST_LIB_DIR}")
    message(STATUS "linking to SocketIO in ${SOCKET_IO_CLIENT_LIB_DIR}")
    message(STATUS "linking to Protobuf in ${PROTOBUF_LIB_DIR}")
    link_directories(${BOOST_LIB_DIR})
    link_directories(${SOCKET_IO_CLIENT_LIB_DIR})
    link_directories(${PROTOBUF_LIB_DIR})

    if(WIN32)
        SET(SUFFIX dll)
        add_definitions("-DAWS_GAMELIFT_EXPORTS")
    else()
        SET(SUFFIX so)
    endif()
else()
    message(STATUS "Configuring project to build Static Libraries.")
    SET(LIBTYPE STATIC)

    if(WIN32)
        SET(SUFFIX lib)
    else()
        SET(SUFFIX a)
    endif()
endif()

project(GameLiftNativeSDK)

# -----------------------------
# Set up source directories
# -----------------------------
file(GLOB AWS_GAMELIFT_COMMON_HEADERS "" "${CMAKE_SOURCE_DIR}/include/aws/gamelift/common/*.h*")
file(GLOB AWS_GAMELIFT_COMMON_SOURCE  "" "${CMAKE_SOURCE_DIR}/source/aws/gamelift/common/*.cpp")

file(GLOB AWS_GAMELIFT_SERVER_HEADERS "" "${CMAKE_SOURCE_DIR}/include/aws/gamelift/server/*.h*")
file(GLOB AWS_GAMELIFT_SERVER_SOURCE  "" "${CMAKE_SOURCE_DIR}/source/aws/gamelift/server/*.cpp")

file(GLOB AWS_GAMELIFT_MODEL_HEADERS "" "${CMAKE_SOURCE_DIR}/include/aws/gamelift/server/model/*.h*")

file(GLOB AWS_GAMELIFT_INTERNAL_HEADERS "" "${CMAKE_SOURCE_DIR}/include/aws/gamelift/internal/*.h")
file(GLOB AWS_GAMELIFT_INTERNAL_NETWORK_HEADERS "" "${CMAKE_SOURCE_DIR}/include/aws/gamelift/internal/network/*.h")

file(GLOB AWS_GAMELIFT_SERVER_PROTOCOLS_SOURCE "" "${CMAKE_SOURCE_DIR}/source/aws/gamelift/server/protocols/*.cc")

set(GAMELIFT_SERVER_SRC
    ${AWS_GAMELIFT_INTERNAL_NETWORK_HEADERS}
    ${AWS_GAMELIFT_INTERNAL_HEADERS}
    ${AWS_GAMELIFT_COMMON_HEADERS}
    ${AWS_GAMELIFT_MODEL_HEADERS}
    ${AWS_GAMELIFT_SERVER_HEADERS}
    ${AWS_GAMELIFT_COMMON_SOURCE}
    ${AWS_GAMELIFT_SERVER_SOURCE}
    ${AWS_GAMELIFT_SERVER_PROTOCOLS_SOURCE}
)

add_definitions(-DAWS_CUSTOM_MEMORY_MANAGEMENT=1)

# Search for header files in these paths
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${SOCKET_IO_CLIENT_INCLUDE_DIR})
include_directories(${PROTOBUF_INCLUDE_DIR})
include_directories(${GAMELIFT_PROTOBUF_INCLUDE_DIR})

# Create library sub-projects
add_library(aws-cpp-sdk-gamelift-server ${LIBTYPE} ${GAMELIFT_SERVER_SRC})

if(BUILD_SHARED_LIBS)
    # Setup link targets
    if (MSVC)
        # Visual Studio looks for .lib files by their literal names.
        target_link_libraries(aws-cpp-sdk-gamelift-server
                              boost_date_time boost_random boost_system
                              sioclient
                              libprotobuf)
    else()
        target_link_libraries(aws-cpp-sdk-gamelift-server
                              boost_date_time boost_random boost_system
                              sioclient
                              protobuf)
    endif(MSVC)
endif(BUILD_SHARED_LIBS)

# If we are compiling for visual studio, create a sane directory tree.
if(MSVC)
    source_group("Header Files\\aws\\gamelift\\common" FILES ${AWS_GAMELIFT_COMMON_HEADERS})
    source_group("Header Files\\aws\\gamelift\\server" FILES ${AWS_GAMELIFT_SERVER_HEADERS})
    source_group("Header Files\\aws\\gamelift\\internal" FILES ${AWS_GAMELIFT_INTERNAL_HEADERS})
    source_group("Header Files\\aws\\gamelift\\internal\\network" FILES ${AWS_GAMELIFT_INTERNAL_NETWORK_HEADERS})
    source_group("Header Files\\aws\\gamelift\\server\\model" FILES ${AWS_GAMELIFT_MODEL_HEADERS})
    source_group("Source Files\\common" FILES ${AWS_GAMELIFT_COMMON_SOURCE})
    source_group("Source Files\\server" FILES ${AWS_GAMELIFT_SERVER_SOURCE})
    source_group("Source Files\\server\\protocols" FILES ${AWS_GAMELIFT_SERVER_PROTOCOLS_SOURCE})
    add_definitions("/Zi")
    # Make the Release version create a PDB
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
    # Enable Generate Debug Info for both debug and release configure
    set_property(TARGET aws-cpp-sdk-gamelift-server APPEND PROPERTY LINK_FLAGS /DEBUG)
    # Minimize the size of the resulting DLLs
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /OPT:REF")
else()
    add_definitions(-std=c++11)
endif(MSVC)

# Add unit test sub-project.
# add_subdirectory("unittest")

# TODO: Make these tests run and give meaningful output to the console when they run.
# We may want to move this into the unittest sub-project to take advantage of the
# target-replacement logic for add_executable() there.
# --------------
# enable_testing()
# add_test(NAME APIUnitTests
#          WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIGURATION>
#          COMMAND gamelift-unittest.exe)

install (TARGETS aws-cpp-sdk-gamelift-server DESTINATION lib)
install (DIRECTORY ${PROJECT_BINARY_DIR}/Debug DESTINATION lib CONFIGURATIONS Debug FILES_MATCHING PATTERN *.pdb)
install (DIRECTORY ${PROJECT_BINARY_DIR}/Release DESTINATION lib CONFIGURATIONS Release FILES_MATCHING PATTERN *.pdb)
install (FILES ${AWS_GAMELIFT_SERVER_HEADERS} DESTINATION include/aws/gamelift/server)
