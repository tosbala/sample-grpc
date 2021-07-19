# Copyright 2018 gRPC authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# cmake build file for C++ helloworld example.
# Assumes protobuf and gRPC have been installed using cmake.
# See cmake_externalproject/CMakeLists.txt for all-in-one cmake build
# that automatically builds all the dependencies before building helloworld.

cmake_minimum_required(VERSION 3.5.1)

project(BookStore C CXX)

include(cmake/common.cmake)

# Process all proto files
foreach(_protofile
  common bookstore warehouse)
  # Proto file
  get_filename_component(bkstr_proto "../protos/${_protofile}.proto" ABSOLUTE)
  get_filename_component(bkstr_proto_path "${bkstr_proto}" PATH)

  # Generated sources
  set(proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/${_protofile}.pb.cc")
  set(proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/${_protofile}.pb.h")
  set(grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/${_protofile}.grpc.pb.cc")
  set(grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/${_protofile}.grpc.pb.h")

  add_custom_command(
        OUTPUT "${proto_srcs}" "${proto_hdrs}" "${grpc_srcs}" "${grpc_hdrs}"
        COMMAND ${_PROTOBUF_PROTOC}
        ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
          --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
          -I "${bkstr_proto_path}"
          --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
          "${bkstr_proto}"
        DEPENDS "${bkstr_proto}")

  list(APPEND bkstr_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/${_protofile}.pb.cc")
  list(APPEND bkstr_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/${_protofile}.pb.h")
  list(APPEND bkstr_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/${_protofile}.grpc.pb.cc")
  list(APPEND bkstr_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/${_protofile}.grpc.pb.h")
endforeach()

# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

# hw_grpc_proto
add_library(bkstr_grpc_proto
  ${bkstr_grpc_srcs}
  ${bkstr_grpc_hdrs}
  ${bkstr_proto_srcs}
  ${bkstr_proto_hdrs})
target_link_libraries(bkstr_grpc_proto
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})

# warehouse client
add_library(warehouse_client warehouse_client.cc)
target_link_libraries(warehouse_client
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})

# bookstore server
add_executable(bookstore_server bookstore_server.cc)
target_link_libraries(bookstore_server
  warehouse_client
  bkstr_grpc_proto
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})

# bookstore client
add_executable(bookstore_client bookstore_client.cc)
target_link_libraries(bookstore_client
  bkstr_grpc_proto
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})

# Targets greeter_[async_](client|server)
# foreach(_target
#   bookstore_client bookstore_server)
#   add_executable(${_target} "${_target}.cc")
#   target_link_libraries(${_target}
#     bkstr_grpc_proto
#     ${_REFLECTION}
#     ${_GRPC_GRPCPP}
#     ${_PROTOBUF_LIBPROTOBUF})
# endforeach()
