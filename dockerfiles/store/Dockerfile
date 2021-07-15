FROM python:3.9-slim-buster

RUN mkdir -p /app
WORKDIR /app

COPY requirements.txt /app
RUN pip install -r requirements.txt

COPY bookstore.py /app
COPY protos /app/protos

RUN python3 -m grpc_tools.protoc -Iprotos --python_out=. --grpc_python_out=. protos/common.proto
RUN python3 -m grpc_tools.protoc -Iprotos --python_out=. --grpc_python_out=. protos/bookstore.proto
RUN python3 -m grpc_tools.protoc -Iprotos --python_out=. --grpc_python_out=. protos/warehouse.proto

CMD ["python3", "/app/bookstore.py"]