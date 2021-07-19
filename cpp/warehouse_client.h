#pragma once

#include <string>

#include <grpcpp/grpcpp.h>

#include "warehouse.grpc.pb.h"
#include "common.grpc.pb.h"

// for warehouse stub
using grpc::Channel;
using warehouse::Warehouse;
using warehouse::WarehouseReply;
using warehouse::WarehouseId;

class WarehouseClient
{
  public:
    WarehouseClient(std::shared_ptr<Channel> channel);

    std::pair<std::string, std::string> Reserve(const Order& order_);
    bool ReservationExists(const Order& order_);
    std::string Dispatch(const std::string& warehouse_id_);

  private:
    std::unique_ptr<Warehouse::Stub> stub_;
};