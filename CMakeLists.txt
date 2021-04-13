    cmake_minimum_required(VERSION 3.17 FATAL_ERROR)

#-------------------------------------------------------------------------------
#  Setup languages to use. Only enable CUDA if GPU's are in use.
#-------------------------------------------------------------------------------
    project(kronmult LANGUAGES CXX)

    option (USE_GPU "Use CUDA for gpu support" OFF)
    if (USE_GPU)
        enable_language(CUDA)
    endif ()

#-------------------------------------------------------------------------------
#  Find 3rd party libraries.
#-------------------------------------------------------------------------------
    find_package(OpenMP)

#-------------------------------------------------------------------------------
#  Configure kron target.
#-------------------------------------------------------------------------------
    add_library (kron SHARED)

    target_sources (kron
                    PUBLIC
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kgemm_nn_batched.hpp>
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kgemm_nn.hpp>
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kgemm_nt_batched.hpp>
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kgemm_nt.hpp>
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kroncommon.hpp>
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kronmult1_batched.hpp>
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kronmult1_pbatched.hpp>
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kronmult1_xbatched.hpp>
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kronmult2_batched.hpp>
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kronmult2_pbatched.hpp>
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kronmult2_xbatched.hpp>
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kronmult2.hpp>
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kronmult1.hpp>
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kronmult3_batched.hpp>
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kronmult3_pbatched.hpp>
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kronmult3_xbatched.hpp>
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kronmult3.hpp>
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kronmult4_batched.hpp>
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kronmult4_pbatched.hpp>
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kronmult4_xbatched.hpp>
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kronmult4.hpp>
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kronmult5_batched.hpp>
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kronmult5_pbatched.hpp>
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kronmult5_xbatched.hpp>
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kronmult5.hpp>
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kronmult6_batched.hpp>
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kronmult6_pbatched.hpp>
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kronmult6_xbatched.hpp>
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kronmult6.hpp>

                    PRIVATE
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kgemm_nn_batched.cpp>
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kgemm_nn.cpp>
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kgemm_nt_batched.cpp>
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kgemm_nt.cpp>
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kronmult1_batched.cpp>
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kronmult1_pbatched.cpp>
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kronmult1_xbatched.cpp>
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kronmult2_batched.cpp>
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kronmult2_pbatched.cpp>
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kronmult2_xbatched.cpp>
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kronmult3_batched.cpp>
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kronmult3_pbatched.cpp>
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kronmult3_xbatched.cpp>
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kronmult4_batched.cpp>
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kronmult4_pbatched.cpp>
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kronmult4_xbatched.cpp>
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kronmult5_batched.cpp>
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kronmult5_pbatched.cpp>
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kronmult5_xbatched.cpp>
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kronmult6_batched.cpp>
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kronmult6_pbatched.cpp>
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kronmult6_xbatched.cpp>
    )

    target_compile_options (kron
                            PRIVATE
                            $<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra -Wpedantic>
                            $<$<COMPILE_LANGUAGE:CUDA>:-x cu -arch sm_60 --compiler-options -fPIC --keep-device-functions>
    )

#  Can't use $<$<COMPILE_LANGUAGE:LANG>:foo> generator expressions for
#  target_compile_features
#
#  Need to handle this with an explicit if statement.
#
    target_compile_features (kron PUBLIC cxx_std_11)
    if (USE_GPU)
        target_compile_features (kron PUBLIC cuda_std_14)
    endif ()

    target_compile_definitions (kron
                                PRIVATE
                                $<$<COMPILE_LANGUAGE:CUDA>:USE_GPU>
    )

    target_link_libraries (kron INTERFACE OpenMP::OpenMP_CXX)

#-------------------------------------------------------------------------------
#  Setup testing
#-------------------------------------------------------------------------------
    include (CTest)

#-------------------------------------------------------------------------------
#  Configure kgemm_nn_test target.
#-------------------------------------------------------------------------------
    add_executable (kgemm_nn_test)
    target_sources (kgemm_nn_test
                    PRIVATE
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/test_kgemm_nn_batched.cpp>
    )
    target_link_libraries(kgemm_nn_test PRIVATE kron)

#-------------------------------------------------------------------------------
#  Configure kgemm_nt_test target.
#-------------------------------------------------------------------------------
    add_executable (kgemm_nt_test)
    target_sources (kgemm_nt_test
                    PRIVATE
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/test_kgemm_nt_batched.cpp>
    )
    target_link_libraries(kgemm_nt_test PRIVATE kron)

#-------------------------------------------------------------------------------
#  Configure test_kronmult6_batched target.
#-------------------------------------------------------------------------------
    add_executable (test_kronmult6_batched)
    target_sources (test_kronmult6_batched
                    PRIVATE
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/test_kronmult6_batched.cpp>
    )
    target_link_libraries(test_kronmult6_batched PRIVATE kron)

#-------------------------------------------------------------------------------
#  Configure (test_kronmult6_pbatched) target.
#-------------------------------------------------------------------------------
    add_executable (test_kronmult6_pbatched)
    target_sources (test_kronmult6_pbatched
                    PRIVATE
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/test_kronmult6_pbatched.cpp>
    )
    target_link_libraries(test_kronmult6_pbatched PRIVATE kron)

#-------------------------------------------------------------------------------
#  Configure test_kronmult6_xbatched target.
#-------------------------------------------------------------------------------
    add_executable (test_kronmult6_xbatched)
    target_sources (test_kronmult6_xbatched
                    PRIVATE
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/test_kronmult6_xbatched.cpp>
    )
    target_link_libraries(test_kronmult6_xbatched PRIVATE kron)
