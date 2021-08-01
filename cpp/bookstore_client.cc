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
#include <fstream>

#include <grpcpp/grpcpp.h>

#include "bookstore.grpc.pb.h"
#include "common.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using bookstore::BookStore;
using bookstore::OrderId;
using bookstore::OrderReply;

bool read_file(const std::string& file_name, std::string& data) {
  std::ifstream ifs(file_name.c_str(), std::ios::in);
  if(ifs.is_open()) {
    data.assign((std::istreambuf_iterator<char>(ifs)),
                 std::istreambuf_iterator<char>());
  }

  return ifs.good();
}

// return SSL creds if certs exists else insecure creds.
std::shared_ptr<grpc::ChannelCredentials> client_credentials() {
  // read certs and keys
  std::string client_cert;
  bool client_cert_exists = read_file("../../../certs/client.crt", client_cert);
  std::string client_key;
  bool client_key_exists = read_file("../../../certs/client.key", client_key);
  std::string root_cert;
  bool root_cert_exists = read_file("../../../certs/ca.crt", root_cert);

  if (!client_cert_exists || !client_key_exists || !root_cert_exists) {
    std::cout << "Certs not found, setting up insecure channel" << std::endl;
    return grpc::InsecureChannelCredentials();
  }

  std::cout << "setting up secure channel" << std::endl;
  grpc::SslCredentialsOptions ssl_opt = {root_cert, client_key, client_cert};
  return grpc::SslCredentials(ssl_opt);
}

class BookstoreClient {
 public:
  BookstoreClient(std::shared_ptr<Channel> channel)
      : stub_(BookStore::NewStub(channel)) {}

  // Assembles the client's payload, sends it and presents the response back
  // from the server.
  std::string Reserve(const std::string& isbn_) {
    // Data we are sending to the server.
    Order request;
    request.set_isbn(isbn_);
    request.set_buyer("bala");
    request.set_quantity(1);

    // Container for the data we expect from the server.
    OrderReply reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC.
    Status status = stub_->Reserve(&context, request, &reply);

    // Act upon its status.
    if (status.ok()) {
      std::cout << "Order " << reply.order_id() << " is in state " << reply.status() << std::endl;
      return reply.order_id();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "RPC failed";
    }
  }

  std::string Purchase(const std::string& order_id_) {
    OrderId request;
    request.set_id(order_id_);

    // Container for the data we expect from the server.
    OrderReply reply;

    ClientContext context;

    // The actual RPC.
    Status status = stub_->Purchase(&context, request, &reply);

    // Act upon its status.
    if (status.ok()) {
      std::cout << "Order " << reply.order_id() << " is in state " << reply.status() << std::endl;
      return reply.status();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "RPC failed";
    }
  }

 private:
  std::unique_ptr<BookStore::Stub> stub_;
};

int main(int argc, char** argv) {
  // Instantiate the client. It requires a channel, out of which the actual RPCs
  // are created. This channel models a connection to an endpoint specified by
  // the argument "--target=" which is the only expected argument.
  // We indicate that the channel isn't authenticated (use of
  // InsecureChannelCredentials()).
  std::string target_str;
  std::string arg_str("--target");
  if (argc > 1) {
    std::string arg_val = argv[1];
    size_t start_pos = arg_val.find(arg_str);
    if (start_pos != std::string::npos) {
      start_pos += arg_str.size();
      if (arg_val[start_pos] == '=') {
        target_str = arg_val.substr(start_pos + 1);
      } else {
        std::cout << "The only correct argument syntax is --target="
                  << std::endl;
        return 0;
      }
    } else {
      std::cout << "The only acceptable argument is --target=" << std::endl;
      return 0;
    }
  } else {
    target_str = "localhost:8000";
  }

  // create an insecure channel
  // BookstoreClient store(
  //     grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  // create a secure channel
  BookstoreClient store(
      grpc::CreateChannel(target_str, client_credentials()));

  // make rpc calls
  std::string order_id = store.Reserve("02");
  std::cout << "order id received: " << order_id << std::endl;
  store.Reserve("02");
  
  std::string order_status = store.Purchase(order_id);
  std::cout << "order in state : " << order_status << std::endl;

  return 0;
}
