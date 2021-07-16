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

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "bookstore.grpc.pb.h"
#include "warehouse.grpc.pb.h"
#include "common.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using bookstore::BookStore;
using bookstore::OrderReply;
using bookstore::OrderId;

// for warehouse stub
using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::Status;
using warehouse::Warehouse;
using warehouse::WarehouseReply;
using warehouse::WarehouseId;

class WarehouseClient
{
  public:
    WarehouseClient(std::shared_ptr<Channel> channel)
      : stub_(Warehouse::NewStub(channel)) {}

    std::pair<std::string, std::string> Reserve(const Order& order_) {
      // Container for the data we expect from the server.
      WarehouseReply reply;

      // Context for the client. It could be used to convey extra information to
      // the server and/or tweak certain RPC behaviors.
      ClientContext context;

      // The actual RPC.
      Status status = stub_->Reserve(&context, order_, &reply);

      // Act upon its status.
      if (status.ok()) {
        std::cout << "Warehouse created " << reply.id() << " in state " << reply.message() << std::endl;
        return std::make_pair(reply.id(), reply.message());
      } else {
        std::cout << status.error_code() << ": " << status.error_message()
                  << std::endl;
        return std::make_pair("INVALID", "FAILED");
      }
    }

    bool ReservationExists(const Order& order_) {
      bool reservation_exists = false;

      WarehouseId request;
      request.set_id("INVALID");

      ClientContext context;
      std::unique_ptr<ClientReader<Order> > reader(stub_->Reservations(&context, request));

      Order reservation;
      while (reader->Read(&reservation)) {
            std::cout << "Found an existing order for " << order_.isbn() << " from user " << order_.buyer() << std::endl;
            if (reservation.isbn() == order_.isbn() and reservation.buyer() == order_.buyer()){
              context.TryCancel();
              reservation_exists = true;
              break;
            }
      }

      reader->Finish();

      return reservation_exists;
    }

    std::string Dispatch(const std::string& warehouse_id_) {
      WarehouseId request;
      request.set_id(warehouse_id_);

      // Container for the data we expect from the server.
      WarehouseReply reply;

      // Context for the client. It could be used to convey extra information to
      // the server and/or tweak certain RPC behaviors.
      ClientContext context;

      // The actual RPC.
      Status status = stub_->Dispatch(&context, request, &reply);

      // Act upon its status.
      if (status.ok()) {
        return reply.message();
      } else {
        std::cout << status.error_code() << ": " << status.error_message()
                  << std::endl;
        return "FAILED";
      }
    }

  private:
    std::unique_ptr<Warehouse::Stub> stub_;
};

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
  std::string server_address("0.0.0.0:8000");
  BookStoreServiceImpl service;

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
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
