    cmake_minimum_required (VERSION 3.17 FATAL_ERROR)

#-------------------------------------------------------------------------------
#  Setup languages to use. Only enable CUDA if GPU's are in use.
#-------------------------------------------------------------------------------
    project (kronmult LANGUAGES CXX)

    option (USE_GPU "Use CUDA for gpu support" OFF)
    if (USE_GPU)
        enable_language (CUDA)
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
            target_compile_features (${target} PUBLIC cuda_std_14)
            get_target_property (SOURCES ${target} SOURCES)
            set_source_files_properties (${SOURCES} PROPERTIES LANGUAGE CUDA)
            set_target_properties (${target} PROPERTIES CUDA_ARCHITECTURES 60)
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
                            $<$<COMPILE_LANGUAGE:CUDA>:--compiler-options -fPIC --keep-device-functions>
    )

    target_compile_definitions (kron
                                PUBLIC
                                $<$<COMPILE_LANGUAGE:CUDA>:USE_GPU>
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
