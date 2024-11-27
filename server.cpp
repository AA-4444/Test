#include <iostream>
#include <string>
#include <zmq.hpp>  // For ZeroMQ
#include <dlfcn.h>  // For dynamic loading of shared libraries

// Function to load and call a function from a plugin dynamically
void call_plugin_function(const std::string& func_name, const std::string& arg) {
    // Load the shared library
    void* handle = dlopen("./plugins/libplugin.so", RTLD_LAZY);
    if (!handle) {
        std::cerr << "Cannot open library: " << dlerror() << std::endl;
        return;
    }
    //

    // Look for the function symbol in the library
    typedef void (*plugin_func)(const char*);
    plugin_func func = (plugin_func)dlsym(handle, func_name.c_str());
    if (!func) {
        std::cerr << "Cannot find function: " << dlerror() << std::endl;
        dlclose(handle);
        return;
    }

    // Call the function from the library
    func(arg.c_str());
    dlclose(handle);
}

int main() {
    // Initialize ZeroMQ context and socket
    zmq::context_t context(1);
    zmq::socket_t socket(context, ZMQ_REP);  // REP (reply) socket for responding
    socket.bind("tcp://*:5555");

    std::cout << "Server started, waiting for requests..." << std::endl;

    while (true) {
        zmq::message_t request;
        socket.recv(request, zmq::recv_flags::none);
        std::string request_str(static_cast<char*>(request.data()), request.size());

        std::cout << "Received request: " << request_str << std::endl;

        // Assuming the client sends the function name and arguments
        // e.g., "foo|arg1"
        size_t delimiter_pos = request_str.find('|');
        if (delimiter_pos != std::string::npos) {
            std::string func_name = request_str.substr(0, delimiter_pos);
            std::string arg = request_str.substr(delimiter_pos + 1);

            // Call the corresponding function from the plugin
            call_plugin_function(func_name, arg);
        }

        // Send a reply to the client
        std::string response = "Function executed!";
        socket.send(zmq::buffer(response), zmq::send_flags::none);
    }

    return 0;
}

