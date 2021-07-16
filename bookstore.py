from concurrent import futures
import logging

import grpc

import common_pb2
import bookstore_pb2
import bookstore_pb2_grpc

import warehouse_pb2
import warehouse_pb2_grpc

WAREHOUSE = 'warehouse'


class BookStore(bookstore_pb2_grpc.BookStoreServicer):
    order_id = 0
    orders = dict()
    def Reserve(self, request, context):
        print(f"Order from '{request.buyer}' for '{request.quantity}' no of '{request.isbn}' is received", flush=True)

        # make a reservation request to warehouse if one doesnt exist already
        with grpc.insecure_channel(WAREHOUSE + ':8001') as channel:
            stub  = warehouse_pb2_grpc.WarehouseStub(channel)
            # stream reservations from warehouse
            reservations = stub.Reservations(warehouse_pb2.WarehouseId(id=request.isbn))

            # if reservation found, cancel stream
            already_reserved = False
            try:
                for reservation in reservations:
                    if reservation.isbn == request.isbn and reservation.buyer == request.buyer:
                        already_reserved = True
                        reservations.cancel()
            except grpc.RpcError as e:
                pass
        
            # make a reservations if one not exists already
            store_id = 'INVALID'
            if not already_reserved:
                response = stub.Reserve(common_pb2.Order(isbn=request.isbn, buyer=request.buyer, quantity=request.quantity))
                if response.message == 'RESERVED':
                    self.order_id = self.order_id + 1
                    store_id = 'BK' + str(self.order_id).zfill(5)
                    self.orders[store_id] = {'warehouse_id': response.id, 'status': response.message}
        
        if already_reserved: 
            return bookstore_pb2.OrderReply(order_id=store_id, status="ALREADY_RESERVED")
        elif response.message == 'RESERVED':
            print(f"Order is reserved with id '{store_id}'")
            return bookstore_pb2.OrderReply(order_id=store_id, status="RESERVED")
        else:
            return bookstore_pb2.OrderReply(order_id='INVALID', status="FAILED")

    def Purchase(self, request, context):
        # check if order exists
        store_id = request.id
        if store_id not in self.orders:
            return bookstore_pb2.OrderReply(order_id=store_id, status="NOT_FOUND")

        if(self.orders[store_id]['status'] == 'DISPATCHED'):
            return bookstore_pb2.OrderReply(order_id=store_id, status="ALREADY_DISPATCHED")

        # make a dispatch request to warehouse
        with grpc.insecure_channel(WAREHOUSE + ':8001') as channel:
            stub  = warehouse_pb2_grpc.WarehouseStub(channel)
            response = stub.Dispatch(warehouse_pb2.WarehouseId(id=self.orders[store_id]['warehouse_id']))
            if response.message == 'CONFIRMED':
                self.orders[store_id]['status'] = 'DISPATCHED'
                print(f"Order with id '{store_id}' is completed now", flush=True)
                return bookstore_pb2.OrderReply(order_id=store_id, status="DISPATCHED")
            
            return bookstore_pb2.OrderReply(order_id=store_id, status=response.message)


def serve():
    server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
    bookstore_pb2_grpc.add_BookStoreServicer_to_server(BookStore(), server)
    server.add_insecure_port('[::]:8000')
    server.start()
    server.wait_for_termination()


if __name__ == '__main__':
    logging.basicConfig()
    serve()