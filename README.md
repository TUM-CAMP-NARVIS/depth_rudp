# depth_rudp

Depth compression library with an example Reliable UDP based ENET Server/Client application. 

Compression is based on [Temporal-RVL](https://github.com/hanseuljun/temporal-rvl). 

### Setup:

Run these commands to install conan dependencies and build the source.

conan install . --install-folder build

conan build . --build-folder build

### Run example:

Within the build/bin directory, run

./depth_enet_server 

starts the server. 


./depth_enet_client

shows decompressed depth video stream.


### TODOs:

- Currently, all depth images are recognized as keyframes. When keyframe is set to false, server crushes with SIGSEGV error. The problem arises either from trvl.h or rvl.h, this should be debugged and identified. 

- In the stream compression algorithm, once per each 30 frame is recognized as a keyframe. Both the encoder and decoder should know whether a specific frame is a keyframe. This should be communicated between server and client. 
