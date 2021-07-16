from __future__ import print_function
import logging
import random

import grpc

import common_pb2
import bookstore_pb2
import bookstore_pb2_grpc

BOOKSTORE = 'store'


def run():
    # NOTE(gRPC Python Team): .close() is possible on a channel and should be
    # used in circumstances in which the with statement does not fit the needs
    # of the code.
    no_of_reservations = 25
    user_list = ['aviz', 'santi', 'ishbi', 'devz']
    with grpc.insecure_channel(BOOKSTORE + ':8000') as channel:
        stub = bookstore_pb2_grpc.BookStoreStub(channel)
        reservations = []
        # make all the reservations at once
        for i in range(no_of_reservations):
            response = stub.Reserve(common_pb2.Order(isbn=str(i).zfill(4), buyer=random.choice(user_list), quantity=1))
            order_id = response.order_id
            print(f"Order id {order_id} in state {response.status} received")

            if response.status in ['RESERVED']:
                reservations.append(order_id)
        # then go for the purchase
        for reservation in reservations:
                response = stub.Purchase(bookstore_pb2.OrderId(id=reservation))
                print(f"Order id {reservation} in state {response.status} received ")

        


if __name__ == '__main__':
    logging.basicConfig()
    run()