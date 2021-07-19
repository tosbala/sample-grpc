#include <chrono>

#include "warehouse_client.h"

using grpc::ClientContext;
using grpc::ClientReader;
using grpc::Status;
using grpc::Channel;


WarehouseClient::WarehouseClient(std::shared_ptr<Channel> channel)
      : stub_(Warehouse::NewStub(channel)) {}

std::pair<std::string, std::string> WarehouseClient::Reserve(const Order& order_) {
    // Container for the data we expect from the server.
    WarehouseReply reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(2));

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

bool WarehouseClient::ReservationExists(const Order& order_) {
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

std::string WarehouseClient::Dispatch(const std::string& warehouse_id_) {
    WarehouseId request;
    request.set_id(warehouse_id_);

    // Container for the data we expect from the server.
    WarehouseReply reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(2));

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