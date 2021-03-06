# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

file(GLOB_RECURSE onnxruntime_framework_srcs
    "${ONNXRUNTIME_INCLUDE_DIR}/core/framework/*.h"
    "${ONNXRUNTIME_ROOT}/core/framework/*.h"
    "${ONNXRUNTIME_ROOT}/core/framework/*.cc"
)

source_group(TREE ${REPO_ROOT} FILES ${onnxruntime_framework_srcs})

add_library(onnxruntime_framework ${onnxruntime_framework_srcs})

target_include_directories(onnxruntime_framework PRIVATE ${ONNXRUNTIME_ROOT} PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${eigen_INCLUDE_DIRS})
onnxruntime_add_include_to_target(onnxruntime_framework onnxruntime_common gsl onnx onnx_proto protobuf::libprotobuf)
set_target_properties(onnxruntime_framework PROPERTIES FOLDER "ONNXRuntime")
# need onnx to build to create headers that this project includes
add_dependencies(onnxruntime_framework ${onnxruntime_EXTERNAL_DEPENDENCIES})

target_link_libraries(onnxruntime_framework pthread rt config++)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/../include/onnxruntime/core/framework  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/onnxruntime/core)
if (WIN32)
    # Add Code Analysis properties to enable C++ Core checks. Have to do it via a props file include.
    set_target_properties(onnxruntime_framework PROPERTIES VS_USER_PROPS ${PROJECT_SOURCE_DIR}/ConfigureVisualStudioCodeAnalysis.props)
endif()

if(onnxruntime_USE_EIGEN_THREADPOOL)
    target_compile_definitions(onnxruntime_framework PUBLIC USE_EIGEN_THREADPOOL)
endif()
