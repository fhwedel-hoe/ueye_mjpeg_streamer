#include <iostream>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <chrono>
#include <stdexcept>
#include <thread>
#include <memory>
#include <dlfcn.h>

#include "publisher.hpp"
#include "compress.hpp"
#include "serve.hpp"
#include "camera.hpp"

void capture(IPC_globals & ipc) {
    for (;;) { // stream forever
        try { // do not break loop due to exceptions
            ipc.readers.read(); // wait for reader
            std::cerr << "Initializing camera for new recording session..." << std::endl;
            std::unique_ptr<Camera> camera = init_camera();
            std::cerr << "Camera initialized, starting stream..." << std::endl;
            /* capture a single image and submit it to the streaming library */
            while (ipc.readers.read_unsafe() > 0) {
                /* grab raw image data frame */
                RawImage raw_image = camera->grab_frame();
                /* compress image data */
                binary_data image_compressed = 
                    compress(raw_image.data, raw_image.width, raw_image.height, raw_image.pixelFormat);
                /* publish for readers */
                ipc.data.publish(image_compressed);
            }
            std::cerr << "Stopping camera due to lack of viewers..." << std::endl; 
            // ^^ happens implicitly during destructor
        } catch (std::exception & se) {
            std::cerr << "Unexpected exception: " << se.what() << "\n";
        }
    }
}

IPC_globals ipc_globals;

const unsigned short server_port = 8080;

int main(int, char**) {
    std::thread(capture, std::ref(ipc_globals)).detach();
    try {
        boost::asio::io_service io_service;
        server(io_service, server_port, ipc_globals);
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    return 0;
}
