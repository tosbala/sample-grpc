# sample-grpc
sample book store app using grpc python and grpc cpp
demonstrates python and cpp services interacting over grpc.

## run python services
Run the following commands that will bring `store`, `warehouse` and `client` up

```
git clone https://github.com/tosbala/sample-grpc.git
cd sample-grpc
docker-compose build
docker-compose up
```

## Build and Run cpp services
Ensure you have [grpc_cpp](https://grpc.io/docs/languages/cpp/quickstart/) setup in your system.  
We can take out python service and replace it with cpp service and vice versa.

### Build
```
cd sample-grpc/cpp
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

### Health Check
Ensure `grpc::EnableDefaultHealthCheckService` is set to `true` to enable tracking the status through [grpc health check protocol](https://github.com/grpc/grpc/blob/master/doc/health-checking.md).  
Then [grpc_health_probe](https://github.com/grpc-ecosystem/grpc-health-probe) shall be used to probe the health of the service

### Secure Using SSL credentials
#### Client
```
  grpc::SslCredentialsOptions ssl_opt = {root_cert, client_key, client_cert};
  auto client_creds = grpc::SslCredentials(ssl_opt);
  BookstoreClient store(
      grpc::CreateChannel(target_str, client_creds));
```
Refer ```client_credentials()``` in ```cpp/bookstore_client.cc```
#### Server
```
  grpc::SslServerCredentialsOptions::PemKeyCertPair key_cert_pair = {server_key, server_cert};
  grpc::SslServerCredentialsOptions ssl_opts(GRPC_SSL_REQUEST_AND_REQUIRE_CLIENT_CERTIFICATE_AND_VERIFY);
  ssl_opts.pem_root_certs = root_cert;
  ssl_opts.pem_key_cert_pairs.push_back(key_cert_pair);

  auto server_creds = grpc::SslServerCredentials(ssl_opts);
  builder.AddListeningPort(server_address, server_creds);
```
Refer ```server_credentials()``` in ```cpp/bookstore_server.cc```  
All the references to certs and keys require file content as string not the file path.
