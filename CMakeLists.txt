    cmake_minimum_required (VERSION 3.17 FATAL_ERROR)

#-------------------------------------------------------------------------------
#  Setup languages to use. Only enable CUDA if GPU's are in use.
#-------------------------------------------------------------------------------
    project (kronmult LANGUAGES CXX)

    option (USE_GPU "Use HIP for gpu support" OFF)
    set(GPU_ARCH "86" CACHE STRING "GPU architecture code for AMD/NVIDIA")
    if (USE_GPU)
        set(KRONMULT_PLATFORM_AMD 0)
        set(KRONMULT_PLATFORM_NVCC 0)

        # search for HIP and libraries
        if(NOT DEFINED HIP_PATH)
            if(NOT DEFINED ENV{HIP_PATH})
                set(HIP_PATH "/opt/rocm/hip" CACHE PATH "Path to which HIP has been installed")
            else()
                set(HIP_PATH $ENV{HIP_PATH} CACHE PATH "Path to which HIP has been installed")
            endif()
        endif()

        # set HIP_CLANG_PATH for potential installs in non-standard locations (such as rocm with spack)
        if (NOT DEFINED HIP_CLANG_PATH)
            if(NOT DEFINED ENV{HIP_CLANG_PATH})
                set(HIP_CLANG_PATH "${ROCM_PATH}/llvm/bin" CACHE PATH "Path to HIP clang binaries")
            else()
                set(HIP_CLANG_PATH $ENV{HIP_CLANG_PATH} CACHE PATH "Path to HIP clang binaries")
            endif()
        endif()

        # note: could probably grab this path directly using hipconfig?
        if (NOT DEFINED HIP_CLANG_INCLUDE_PATH)
            if(NOT DEFINED ENV{HIP_CLANG_INCLUDE_PATH})
                # probably need a better way to get the compiler version.. this will cause non-existent paths for non-clang compilers
                set(HIP_CLANG_INCLUDE_PATH "${HIP_CLANG_PATH}/../lib/clang/${CMAKE_CXX_COMPILER_VERSION}/include" CACHE PATH "Path to HIP clang include directory")
            else()
                set(HIP_CLANG_INCLUDE_PATH $ENV{HIP_CLANG_INCLUDE_PATH} CACHE PATH "Path to HIP clang include directory")
            endif()
        endif()

        # try to find hipconfig executable which can help detect platforms and include dirs
        find_program(HIPCONFIG_PATH hipconfig HINTS "${HIP_PATH}/bin")
        if(HIPCONFIG_PATH)
            execute_process(COMMAND ${HIPCONFIG_PATH} --platform OUTPUT_VARIABLE KRONMULT_HIP_PLATFORM)
        elseif(DEFINED ENV{HIP_PLATFORM})
            set(KRONMULT_HIP_PLATFORM "$ENV{HIP_PLATFORM}")
        else()
            message(FATAL_ERROR "Could not determine HIP platform, make sure HIP_PLATFORM is set")
        endif()

        message(STATUS "HIP platform has been detected as ${KRONMULT_HIP_PLATFORM}")
        # hip >= 4.2 is now using "amd" to identify platform
        if(KRONMULT_HIP_PLATFORM STREQUAL "hcc" OR KRONMULT_HIP_PLATFORM STREQUAL "amd")
            set(KRONMULT_PLATFORM_AMD 1)
        # hip <= 4.1 uses "nvcc" to identify nvidia platforms, >= 4.2 uses "nvidia"
        elseif(KRONMULT_HIP_PLATFORM STREQUAL "nvcc" OR KRONMULT_HIP_PLATFORM STREQUAL "nvidia")
            set(KRONMULT_PLATFORM_NVCC 1)
        endif()

        set(HIP_VERBOSE_BUILD ON CACHE STRING "Verbose compilation for HIP")

        # look for HIP cmake configs in different locations
        list(APPEND CMAKE_MODULE_PATH "${HIP_PATH}/cmake" "${HIP_PATH}/lib/cmake" "${ROCM_PATH}")
        list(APPEND CMAKE_PREFIX_PATH "${HIP_PATH}/cmake" "${HIP_PATH}/lib/cmake" "${ROCM_PATH}")
        #set(CMAKE_HIP_ARCHITECTURES OFF) # enable if using CMake 3.21
        find_package(HIP REQUIRED 4.0)
        #enable_language (HIP) # enable if using CMake 3.21

        message(STATUS "HIP PLATFORM: ${HIP_PLATFORM}")
        message(STATUS "HIP COMPILER: ${HIP_COMPILER}")
        message(STATUS "HIP RUNTIME: ${HIP_RUNTIME}")
        message(STATUS "HIP Includes: ${HIP_INCLUDE_DIRS}")
        message(STATUS "HIP Libraries: ${HIP_LIBRARIES}")

        if(KRONMULT_PLATFORM_NVCC)
            find_package(CUDA 9.0 REQUIRED)
            include_directories(${CUDA_INCLUDE_DIRS})
            enable_language(CUDA)
            set (CMAKE_CUDA_STANDARD 14)
            set (CMAKE_CUDA_STANDARD_REQUIRED ON)
            add_compile_definitions(__HIP_PLATFORM_NVCC__ __HIP_PLATFORM_NVIDIA__)
        else()
            add_compile_definitions(__HIP_PLATFORM_HCC__ __HIP_PLATFORM_AMD__)
        endif()

    endif ()

#-------------------------------------------------------------------------------
#  Setup a menu of build types.
#-------------------------------------------------------------------------------
    set_property (CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
                  Debug
                  Release
                  MinSizeRel
                  RelWithDebInfo
                  Sanitized
    )

#-------------------------------------------------------------------------------
#  Sanitizer options
#-------------------------------------------------------------------------------
    set (CMAKE_CXX_FLAGS_SANITIZED -fno-omit-frame-pointer)
    set (CMAKE_CUDA_FLAGS_SANITIZED -fno-omit-frame-pointer)
    set (CMAKE_EXE_LINKER_FLAGS_SANITIZED "")
    set (CMAKE_SHARED_LINKER_FLAGS_SANITIZED "")

    macro (register_sanitizer_option name default)
        string (TOUPPER ${name} upper_name)

        option (SANITIZE_${upper_name} "Enable the ${name} sanitizer" ${default})

        if (${SANITIZE_${upper_name}})
            set (CMAKE_CXX_FLAGS_SANITIZED "${CMAKE_CXX_FLAGS_SANITIZED} -fsanitize=${name}")
            set (CMAKE_CUDA_FLAGS_SANITIZED "${CMAKE_CUDA_FLAGS_SANITIZED} -fsanitize=${name}")
            set (CMAKE_EXE_LINKER_FLAGS_SANITIZED "${CMAKE_EXE_LINKER_FLAGS_SANITIZED} -fsanitize=${name}")
            set (CMAKE_SHARED_LINKER_FLAGS_SANITIZED "${CMAKE_SHARED_LINKER_FLAGS_SANITIZED} -fsanitize=${name}")
        endif ()
    endmacro ()

    register_sanitizer_option (address ON)
    register_sanitizer_option (leak OFF)
    register_sanitizer_option (memory OFF)
    register_sanitizer_option (thread OFF)
    register_sanitizer_option (undefined ON)

#-------------------------------------------------------------------------------
#  Define a macro function to set a targets source files to the CUDA language.
#
#  Can't use $<$<COMPILE_LANGUAGE:LANG>:foo> generator expressions for
#  target_compile_features
#
#  Need to handle this with an explicit if statement.
#-------------------------------------------------------------------------------
    macro (target_set_cuda target)
        if (USE_GPU)
            get_target_property (SOURCES ${target} SOURCES)
            #set_source_files_properties (${SOURCES} PROPERTIES LANGUAGE HIP) # enable if using CMake 3.21

            if(KRONMULT_PLATFORM_NVCC)
                target_compile_features (${target} PUBLIC cuda_std_14)
                set_source_files_properties (${SOURCES} PROPERTIES LANGUAGE CUDA)
                set_target_properties (${target} PROPERTIES CUDA_ARCHITECTURES ${GPU_ARCH})
            else()
                set_source_files_properties (${SOURCES} PROPERTIES HIP_SOURCE_PROPERTY_FORMAT 1)
                set_target_properties (${target} PROPERTIES HIP_ARCHITECTURES ${GPU_ARCH})
                set_target_properties (${target} PROPERTIES LINKER_LANGUAGE HIP)
            endif()
            set_target_properties (${target} PROPERTIES HIP_SOURCE_PROPERTY_FORMAT 1)

            # HIP_INCLUDE_DIRS is not getting set for nvidia platforms
            target_include_directories(${target} SYSTEM PRIVATE ${HIP_INCLUDE_DIRS} ${HIP_PATH}/include)
        endif ()
    endmacro ()

#-------------------------------------------------------------------------------
#  Find 3rd party libraries.
#-------------------------------------------------------------------------------
    find_package (OpenMP)

#-------------------------------------------------------------------------------
#  Configure kron target.
#-------------------------------------------------------------------------------
    add_library (kron SHARED)

#  Note can't use generator expressions on the source files since it interfers
#  with setting the source property to target CUDA.
    target_sources (kron

                    PRIVATE
                    kgemm_nn_batched.cpp
                    kgemm_nn.cpp
                    kgemm_nt_batched.cpp
                    kgemm_nt.cpp
                    kronmult1_batched.cpp
                    kronmult1_pbatched.cpp
                    kronmult1_xbatched.cpp
                    kronmult2_batched.cpp
                    kronmult2_pbatched.cpp
                    kronmult2_xbatched.cpp
                    kronmult3_batched.cpp
                    kronmult3_pbatched.cpp
                    kronmult3_xbatched.cpp
                    kronmult4_batched.cpp
                    kronmult4_pbatched.cpp
                    kronmult4_xbatched.cpp
                    kronmult5_batched.cpp
                    kronmult5_pbatched.cpp
                    kronmult5_xbatched.cpp
                    kronmult6_batched.cpp
                    kronmult6_pbatched.cpp
                    kronmult6_xbatched.cpp
    )
    target_include_directories (kron
                                PUBLIC
                                ${CMAKE_CURRENT_SOURCE_DIR}
    )

    target_compile_features (kron PUBLIC cxx_std_17)
    target_set_cuda (kron)

    target_compile_options (kron
                            PUBLIC
                            $<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra -Wpedantic>
                            $<$<BOOL:${KRONMULT_PLATFORM_NVCC}>:--compiler-options -fPIC --keep-device-functions>
                            $<$<BOOL:${KRONMULT_PLATFORM_AMD}>:--offload-arch=${GPU_ARCH}>
    )

    target_compile_definitions (kron
                                PUBLIC
                                $<$<BOOL:${USE_GPU}>:USE_GPU>
    )

    if (OpenMP_CXX_FOUND)
        target_link_libraries (kron PUBLIC OpenMP::OpenMP_CXX)
    endif ()

#-------------------------------------------------------------------------------
#  Setup testing
#-------------------------------------------------------------------------------
    enable_testing ()

#-------------------------------------------------------------------------------
#  Define a macro function to regester tests.
#-------------------------------------------------------------------------------
    macro (add_test_target target source)
        add_executable (${target})
        target_sources (${target}
                        PRIVATE
                        ${source}
        )
        target_set_cuda (${target})
        target_link_libraries (${target} PUBLIC kron)

        add_test (NAME ${target}
                  COMMAND ${target})
    endmacro ()

#-------------------------------------------------------------------------------
#  Configure test targets.
#-------------------------------------------------------------------------------
    add_test_target (kgemm_nn_test test_kgemm_nn_batched.cpp)
    add_test_target (kgemm_nt_test test_kgemm_nt_batched.cpp)
    add_test_target (test_kronmult6_batched test_kronmult6_batched.cpp)
    add_test_target (test_kronmult6_pbatched test_kronmult6_pbatched.cpp)
    add_test_target (test_kronmult6_xbatched test_kronmult6_xbatched.cpp)
