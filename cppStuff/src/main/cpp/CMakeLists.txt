cmake_minimum_required(VERSION 3.30)
project(OpenCVStuff)

set(CMAKE_CXX_STANDARD 20)

# --- JSON ---
include(FetchContent)

FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
FetchContent_MakeAvailable(json)

#target_link_libraries(foo PRIVATE nlohmann_json::nlohmann_json)

# ---------------------- OpenCV Configuration ----------------------
# Explicitly set OpenCV include directory
if (WIN32)
    include_directories("C:/opencv/build/include")

    # Explicitly set OpenCV library path and name
    set(OpenCV_LIB_DIR "C:/opencv/build/x64/vc16/lib")

    # Choose Debug or Release libraries based on build type
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(OpenCV_LIBS "${OpenCV_LIB_DIR}/opencv_world4110d.lib")  # Debug library
    else()
        set(OpenCV_LIBS "${OpenCV_LIB_DIR}/opencv_world4110.lib")  # Release library
    endif()

    # Debug messages to verify OpenCV configuration
    message(STATUS "OpenCV library: ${OpenCV_LIBS}")
endif (WIN32)

if (UNIX)
    find_package( OpenCV REQUIRED )
    include_directories( ${OpenCV_INCLUDE_DIRS} )
endif (UNIX)

# ---------------------- JNI Configuration ----------------------
find_package(JNI REQUIRED)
if (JNI_FOUND)
    message(STATUS "JNI_INCLUDE_DIRS=${JNI_INCLUDE_DIRS}")
    message(STATUS "JNI_LIBRARIES=${JNI_LIBRARIES}")
endif()

# ---------------------- Source and Header Files ----------------------
set(SOURCES opencv/Otsu.cpp
        opencv/ColorIndex.cpp
        opencv/Strip.cpp
)
set(HEADERS
        jni/org_askolds_cmptest_opencv_OpenCVStuff.h
        opencv/headers/Strip.h
        opencv/headers/ColorIndex.h
        opencv/headers/Otsu.h
)

# ---------------------- Main Executable ----------------------
add_executable(OpenCVStuff ${SOURCES})

# Include directories
target_include_directories(OpenCVStuff PRIVATE
        ${JNI_INCLUDE_DIRS}
        # "C:/opencv/build/include"  # Explicit OpenCV include
        ${OpenCV_INCLUDE_DIRS}
        jni
)

# Link OpenCV and JNI libraries and JSON
target_link_libraries(OpenCVStuff PRIVATE
        ${OpenCV_LIBS}  # Explicit OpenCV library
        ${JNI_LIBRARIES}
        nlohmann_json::nlohmann_json
)

# ---------------------- Google Test Setup ----------------------
include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
FetchContent_MakeAvailable(googletest)

enable_testing()

# Test executable
add_executable(OpenCVTests
        ${SOURCES}
        tests/test_Otsu.cpp
        tests/test_ColorIndex.cpp
        tests/test_Strip.cpp
        tests/test_All.cpp
)

# Include directories for tests
target_include_directories(OpenCVTests PRIVATE
        ${JNI_INCLUDE_DIRS}
        #"C:/opencv/build/include"  # Explicit OpenCV include
        ${OpenCV_INCLUDE_DIRS}
        jni
)

# Link libraries for tests
target_link_libraries(OpenCVTests PRIVATE
        GTest::gtest_main
        ${OpenCV_LIBS}  # Explicit OpenCV library
        ${JNI_LIBRARIES}
        nlohmann_json::nlohmann_json
)

# Add tests to CMake
include(GoogleTest)
gtest_discover_tests(OpenCVTests)