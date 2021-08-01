/*
 *
 * Copyright 2015 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <fstream>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "bookstore.grpc.pb.h"
#include "warehouse.grpc.pb.h"
#include "common.grpc.pb.h"
#include "warehouse_client.h"
#include "bookstore_interceptor.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using bookstore::BookStore;
using bookstore::OrderReply;
using bookstore::OrderId;


bool read_file(const std::string& file_name, std::string& data) {
  std::ifstream ifs(file_name.c_str(), std::ios_base::binary);
  if(ifs.is_open()) {
    data.assign((std::istreambuf_iterator<char>(ifs)),
                 std::istreambuf_iterator<char>());
  }

  return ifs.good();
}

// return SSL creds if certs exists else insecure creds.
std::shared_ptr<grpc::ServerCredentials> server_credentials() {
  // read certs and keys
  std::string server_cert;
  bool server_cert_exists = read_file("../../../certs/server.crt", server_cert);
  std::string server_key;
  bool server_key_exists = read_file("../../../certs/server.key", server_key);
  std::string root_cert;
  bool root_cert_exists = read_file("../../../certs/ca.crt", root_cert);

  if (!server_cert_exists || !server_key_exists || !root_cert_exists) { 
    std::cout << "Certs not found, setting up insecure creds" << std::endl;
    return grpc::InsecureServerCredentials();
  }

  std::cout << "setting up SSL creds" << std::endl;
  grpc::SslServerCredentialsOptions::PemKeyCertPair key_cert_pair = {server_key, server_cert};
  grpc::SslServerCredentialsOptions ssl_opts(GRPC_SSL_REQUEST_AND_REQUIRE_CLIENT_CERTIFICATE_AND_VERIFY);
  ssl_opts.pem_root_certs = root_cert;
  ssl_opts.pem_key_cert_pairs.push_back(key_cert_pair);

  return grpc::SslServerCredentials(ssl_opts);
}


// Logic and data behind the server's behavior.
class BookStoreServiceImpl final : public BookStore::Service {
  Status Reserve(ServerContext* context, const Order* request, OrderReply* reply) override {
    WarehouseClient warehouse(grpc::CreateChannel("localhost:8001", grpc::InsecureChannelCredentials()));
    
    if (warehouse.ReservationExists(*request)) {
      reply->set_order_id("INVALID");
      reply->set_status("ALREADY_RESERVED"); 

      return Status::OK;
    }
    
    auto warehouse_reply = warehouse.Reserve(*request);

    if (warehouse_reply.second == "RESERVED") {
      order_count += 1;

      std::string order_count_str = std::to_string(order_count);

      std::string order_id = std::string(5 - order_count_str.length(), '0') + order_count_str;
      orders[order_id] = std::make_pair(warehouse_reply.first, warehouse_reply.second);

      reply->set_order_id(order_id);
      reply->set_status("RESERVED");
    }
    else {
      reply->set_order_id("INVALID");
      reply->set_status("FAILED");
    }

    return Status::OK;
  }

  Status Purchase(ServerContext* context, const OrderId* request,
                  OrderReply* reply) override {
    reply->set_order_id(request->id());

    if( orders.find(request->id()) == orders.end()) {
      reply->set_status("NOT_FOUND");
    }
    else {
      auto& order = orders[request->id()];

      if(order.second == "DISPATCHED") {
        reply->set_status("ALREADY_DISPATCHED");
      }

      WarehouseClient warehouse(grpc::CreateChannel("localhost:8001", grpc::InsecureChannelCredentials()));
      auto warehouse_reply = warehouse.Dispatch(order.first);

      orders[request->id()] = std::make_pair(order.first, warehouse_reply);

      if (warehouse_reply == "CONFIRMED") {
        reply->set_status("DISPATCHED");
      }
      else
        reply->set_status(warehouse_reply);
    }

    return Status::OK;
  }

  public:
    static long order_count;
    static std::map<std::string, std::pair<std::string, std::string>> orders;
};

long BookStoreServiceImpl::order_count = 0;
std::map<std::string, std::pair<std::string, std::string>> BookStoreServiceImpl::orders;

void RunServer() {
  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;
  
  // Listen on the given address without any authentication mechanism.
  // builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

  // Listen securely on the given address.
  std::string server_address("0.0.0.0:8000");
  builder.AddListeningPort(server_address, server_credentials());
  
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  BookStoreServiceImpl service;
  builder.RegisterService(&service);

  // Lets set up an inceptor
  std::vector<std::unique_ptr<experimental::ServerInterceptorFactoryInterface>> creators;
  creators.push_back(std::unique_ptr<experimental::ServerInterceptorFactoryInterface>(new LoggingInterceptorFactory()));
  builder.experimental().SetInterceptorCreators(std::move(creators));

  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

int main(int argc, char** argv) {
  RunServer();

  return 0;
}
