cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3") # Optimization level 3
elseif(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /O2") # Optimization level 2
endif()

project (volumepid)

add_executable (volumepid "ProcessAudioCaptureWin.cpp" "LoopbackCapture.cpp")

target_include_directories(volumepid PRIVATE "./packages/Microsoft.Windows.ImplementationLibrary.1.0.210204.1/include" "./")

target_link_libraries(volumepid wsock32 ws2_32 mmdevapi Mfplat mfuuid)
