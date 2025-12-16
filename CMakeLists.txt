cmake_minimum_required(VERSION 3.14)
project(IQResampler)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable testing
enable_testing()

# Fetch Google Test
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Compiler flags for optimization
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -mtune=native")
endif()

# Option to enable Intel IPP
option(USE_IPP "Use Intel IPP for acceleration" OFF)

# Pure C++ version
add_executable(test_resampler_cpp test_resampler.cpp)
target_compile_options(test_resampler_cpp PRIVATE -Wall -Wextra)

if(USE_IPP)
    # Find Intel IPP
    if(DEFINED ENV{IPPROOT})
        set(IPP_ROOT $ENV{IPPROOT})
    elseif(EXISTS "/opt/intel/oneapi/ipp/latest")
        set(IPP_ROOT "/opt/intel/oneapi/ipp/latest")
    endif()

    if(IPP_ROOT)
        message(STATUS "Intel IPP found at: ${IPP_ROOT}")
        
        # IPP version with Intel IPP
        add_executable(test_resampler_ipp test_resampler.cpp)
        target_compile_definitions(test_resampler_ipp PRIVATE USE_IPP)
        target_compile_options(test_resampler_ipp PRIVATE -Wall -Wextra)
        
        target_include_directories(test_resampler_ipp PRIVATE 
            ${IPP_ROOT}/include
        )
        
        # Link IPP libraries
        find_library(IPP_CORE ippcore PATHS ${IPP_ROOT}/lib ${IPP_ROOT}/lib/intel64)
        find_library(IPP_S ipps PATHS ${IPP_ROOT}/lib ${IPP_ROOT}/lib/intel64)
        find_library(IPP_VM ippvm PATHS ${IPP_ROOT}/lib ${IPP_ROOT}/lib/intel64)
        
        if(IPP_CORE AND IPP_S)
            target_link_libraries(test_resampler_ipp PRIVATE 
                ${IPP_S}
                ${IPP_CORE}
            )
            if(IPP_VM)
                target_link_libraries(test_resampler_ipp PRIVATE ${IPP_VM})
            endif()
        else()
            message(WARNING "IPP libraries not found, building C++ version only")
        endif()
    else()
        message(WARNING "Intel IPP not found. Set IPPROOT environment variable or install Intel IPP.")
        message(STATUS "Building C++ version only")
    endif()
else()
    message(STATUS "Intel IPP disabled. Use -DUSE_IPP=ON to enable.")
endif()

# Link math library
target_link_libraries(test_resampler_cpp PRIVATE m)
if(TARGET test_resampler_ipp)
    target_link_libraries(test_resampler_ipp PRIVATE m)
endif()

# Google Test executable
add_executable(resampler_gtest test_resampler_gtest.cpp)
target_link_libraries(resampler_gtest PRIVATE
    GTest::gtest_main
    m
)
target_compile_options(resampler_gtest PRIVATE -Wall -Wextra)

include(GoogleTest)
gtest_discover_tests(resampler_gtest)
