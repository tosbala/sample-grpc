from concurrent import futures
import logging
from google.protobuf import message

import grpc

import common_pb2
import warehouse_pb2
import warehouse_pb2_grpc


class Warehouse(warehouse_pb2_grpc.WarehouseServicer):
    order_id = 0
    reservations = {}

    def Reserve(self, request, context):
        self.order_id = self.order_id + 1
        warehouse_id = 'WH' + str(self.order_id).zfill(5)

        self.reservations[warehouse_id] = request

        print(f"Book {request.isbn} for user is reserved with id {warehouse_id}")
        return warehouse_pb2.WarehouseReply(id=warehouse_id, message="RESERVED")

    def Reservations(self, request, context):
        def response_messages():
            for reservation in self.reservations:
                response = common_pb2.Order(isbn=reservation.isbn, buyer=reservation.buyer, quantity=reservation.quantity)
                yield response

        return response_messages()

    def Dispatch(self, request, context):
        if request.id not in self.reservations:
            return warehouse_pb2.WarehouseReply(id=request.id, message="RESERVATION NOT FOUND")

        reservation = self.reservations.pop(request.id)
        print(f"Book {reservation.isbn} reserved with id {request.id} is dispatched now")
        return warehouse_pb2.WarehouseReply(id=request.id, message="CONFIRMED")


def serve():
    server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
    warehouse_pb2_grpc.add_WarehouseServicer_to_server(Warehouse(), server)
    server.add_insecure_port('[::]:8001')
    server.start()
    server.wait_for_termination()


if __name__ == '__main__':
    logging.basicConfig()
    serve()