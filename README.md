# sample-grpc
sample book store app using grpc python and grpc cpp
demonstrates python and cpp services interacting over grpc.

## run python services
Run the following commands that will bring `store`, `warehouse` and `client` up

```
docker-compose build
docker-compose up
```

## Build and Run cpp services
Ensure you have [grpc_cpp](https://grpc.io/docs/languages/cpp/quickstart/) setup in your system.
We can take out python service and replace it with cpp service and vice versa.

### Build
```
cd cpp
mkdir -p cmake/build
pushd cmake/build
cmake -DCMAKE_PREFIX_PATH=$MY_INSTALL_DIR ../..
make -j
```

### Run store service
```
./bookstore_server 
```

### Run client
```
./bookstore_client
```
