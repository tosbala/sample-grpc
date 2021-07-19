#include <iostream>

#include <grpcpp/support/server_interceptor.h>

using namespace grpc;

class LoggingInterceptor : public experimental::Interceptor {
    public:
        explicit LoggingInterceptor(experimental::ServerRpcInfo* info_) {
            info = info_;
        }

        void Intercept(experimental::InterceptorBatchMethods* methods) override {
            // std::cout << "START" << info->method() << methods->QueryInterceptionHookPoint() << std::endl;
            std::string method = info->method();
            std::string hook_point = GetHookPoint(methods);
            std::cout << "START " << method << ": " << hook_point << std::endl;
            methods->Proceed();
            std::cout << "END " << method << ": " << hook_point << std::endl;
        }

        std::string GetHookPoint(experimental::InterceptorBatchMethods* methods) {
            if (methods->QueryInterceptionHookPoint(experimental::InterceptionHookPoints::PRE_SEND_INITIAL_METADATA)) {
                return "PRE_SEND_INITIAL_METADATA";
            }
            else if (methods->QueryInterceptionHookPoint(experimental::InterceptionHookPoints::PRE_SEND_MESSAGE)) {
                return "PRE_SEND_MESSAGE";
            }
            else if (methods->QueryInterceptionHookPoint(experimental::InterceptionHookPoints::POST_SEND_MESSAGE)) {
                return "POST_SEND_MESSAGE";
            }
            else if (methods->QueryInterceptionHookPoint(experimental::InterceptionHookPoints::PRE_SEND_STATUS)) {
                return "PRE_SEND_STATUS";
            }
            else if (methods->QueryInterceptionHookPoint(experimental::InterceptionHookPoints::PRE_SEND_CLOSE)) {
                return "PRE_SEND_CLOSE";
            }
            else if (methods->QueryInterceptionHookPoint(experimental::InterceptionHookPoints::PRE_RECV_INITIAL_METADATA)) {
                return "PRE_RECV_INITIAL_METADATA";
            }
            else if (methods->QueryInterceptionHookPoint(experimental::InterceptionHookPoints::PRE_RECV_MESSAGE)) {
                return "PRE_RECV_MESSAGE";
            }
            else if (methods->QueryInterceptionHookPoint(experimental::InterceptionHookPoints::POST_RECV_INITIAL_METADATA)) {
                return "POST_RECV_INITIAL_METADATA";
            }
            else if (methods->QueryInterceptionHookPoint(experimental::InterceptionHookPoints::POST_RECV_MESSAGE)) {
                return "POST_RECV_MESSAGE";
            }
            else if (methods->QueryInterceptionHookPoint(experimental::InterceptionHookPoints::POST_RECV_STATUS)) {
                return "POST_RECV_STATUS";
            }
            else if (methods->QueryInterceptionHookPoint(experimental::InterceptionHookPoints::POST_RECV_CLOSE)) {
                return "POST_RECV_CLOSE";
            }
            else if (methods->QueryInterceptionHookPoint(experimental::InterceptionHookPoints::PRE_SEND_CANCEL)) {
                return "PRE_SEND_CANCEL";
            }
            else if (methods->QueryInterceptionHookPoint(experimental::InterceptionHookPoints::NUM_INTERCEPTION_HOOKS)) {
                return "NUM_INTERCEPTION_HOOKS";
            }
            else {
                return "UNEXPECTED";
            }
        }
    private:
        grpc::experimental::ServerRpcInfo* info;
};

class LoggingInterceptorFactory
    : public experimental::ServerInterceptorFactoryInterface {
 public:
  experimental::Interceptor* CreateServerInterceptor(
      experimental::ServerRpcInfo* info) override {
    return new LoggingInterceptor(info);
  }
};