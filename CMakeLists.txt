cmake_minimum_required(VERSION 3.14)
project(IQResampler)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable testing
enable_testing()

# Fetch Google Test and Google Benchmark
include(FetchContent)

# Google Test
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Google Benchmark
FetchContent_Declare(
  googlebenchmark
  URL https://github.com/google/benchmark/archive/refs/tags/v1.8.3.zip
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googlebenchmark)

# Compiler flags for optimization
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -mtune=native")
endif()

# Sanitizer options
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)

if(ENABLE_ASAN)
    message(STATUS "AddressSanitizer enabled")
    set(SANITIZER_FLAGS "${SANITIZER_FLAGS} -fsanitize=address -fsanitize=leak -fno-omit-frame-pointer")
    set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -fsanitize=address -fsanitize=leak")
endif()

if(ENABLE_UBSAN)
    message(STATUS "UndefinedBehaviorSanitizer enabled")
    set(SANITIZER_FLAGS "${SANITIZER_FLAGS} -fsanitize=undefined -fno-omit-frame-pointer")
    set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -fsanitize=undefined")
endif()

if(ENABLE_TSAN)
    message(STATUS "ThreadSanitizer enabled")
    set(SANITIZER_FLAGS "${SANITIZER_FLAGS} -fsanitize=thread -fno-omit-frame-pointer")
    set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -fsanitize=thread")
endif()

if(SANITIZER_FLAGS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZER_FLAGS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SANITIZER_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${SANITIZER_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${SANITIZER_FLAGS}")
endif()

# Option to enable Intel IPP
option(USE_IPP "Use Intel IPP for acceleration" OFF)

# Pure C++ version
add_executable(test_resampler_cpp test_resampler.cpp iq_resampler_cpp.cpp)
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
        add_executable(test_resampler_ipp test_resampler.cpp iq_resampler_cpp.cpp iq_resampler_ipp.cpp)
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

# Google Test executables

# Google Test for Pure C++ implementation
add_executable(resampler_cpp_gtest test_resampler_cpp_gtest.cpp iq_resampler_cpp.cpp)
target_link_libraries(resampler_cpp_gtest PRIVATE
    GTest::gtest_main
    m
)
target_compile_options(resampler_cpp_gtest PRIVATE -Wall -Wextra)

# Google Test for IPP implementation (if enabled)
if(USE_IPP AND IPP_ROOT)
    add_executable(resampler_ipp_gtest test_resampler_ipp_gtest.cpp iq_resampler_cpp.cpp iq_resampler_ipp.cpp)
    target_compile_definitions(resampler_ipp_gtest PRIVATE USE_IPP)
    target_link_libraries(resampler_ipp_gtest PRIVATE
        GTest::gtest_main
        m
    )
    target_compile_options(resampler_ipp_gtest PRIVATE -Wall -Wextra)

    target_include_directories(resampler_ipp_gtest PRIVATE
        ${IPP_ROOT}/include
    )

    # Set RPATH for IPP libraries
    set_target_properties(resampler_ipp_gtest PROPERTIES
        BUILD_RPATH "${IPP_ROOT}/lib/intel64"
        INSTALL_RPATH "${IPP_ROOT}/lib/intel64"
    )

    # Link IPP libraries
    if(IPP_CORE AND IPP_S)
        target_link_libraries(resampler_ipp_gtest PRIVATE
            ${IPP_S}
            ${IPP_CORE}
        )
        if(IPP_VM)
            target_link_libraries(resampler_ipp_gtest PRIVATE ${IPP_VM})
        endif()
    endif()
endif()

# Legacy combined test (for backward compatibility)
add_executable(resampler_gtest test_resampler_gtest.cpp iq_resampler_cpp.cpp)
target_link_libraries(resampler_gtest PRIVATE
    GTest::gtest_main
    m
)
target_compile_options(resampler_gtest PRIVATE -Wall -Wextra)

include(GoogleTest)
gtest_discover_tests(resampler_cpp_gtest)
# Disable automatic test discovery for IPP test (requires LD_LIBRARY_PATH set)
# Run manually with: export LD_LIBRARY_PATH=/opt/intel/oneapi/ipp/latest/lib/intel64:$LD_LIBRARY_PATH && ./resampler_ipp_gtest
gtest_discover_tests(resampler_gtest)

# Google Benchmark executables

# Benchmark for Pure C++ implementation
add_executable(benchmark_cpp benchmark_resampler.cpp iq_resampler_cpp.cpp)
target_link_libraries(benchmark_cpp PRIVATE
    benchmark::benchmark
    m
)
target_compile_options(benchmark_cpp PRIVATE -Wall -Wextra)

# Benchmark for IPP implementation (if enabled)
if(USE_IPP AND IPP_ROOT)
    add_executable(benchmark_ipp benchmark_resampler.cpp iq_resampler_cpp.cpp iq_resampler_ipp.cpp)
    target_compile_definitions(benchmark_ipp PRIVATE USE_IPP)
    target_link_libraries(benchmark_ipp PRIVATE
        benchmark::benchmark
        m
    )
    target_compile_options(benchmark_ipp PRIVATE -Wall -Wextra)

    target_include_directories(benchmark_ipp PRIVATE
        ${IPP_ROOT}/include
    )

    # Set RPATH for IPP libraries
    set_target_properties(benchmark_ipp PROPERTIES
        BUILD_RPATH "${IPP_ROOT}/lib/intel64"
        INSTALL_RPATH "${IPP_ROOT}/lib/intel64"
    )

    # Link IPP libraries
    if(IPP_CORE AND IPP_S)
        target_link_libraries(benchmark_ipp PRIVATE
            ${IPP_S}
            ${IPP_CORE}
        )
        if(IPP_VM)
            target_link_libraries(benchmark_ipp PRIVATE ${IPP_VM})
        endif()
    endif()
endif()
