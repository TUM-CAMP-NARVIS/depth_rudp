
#include "rudp_server.h"

#include <experimental/filesystem>
#include <fstream>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/types_c.h>

#include "trvl.h"

class InputFile
{
public:
    InputFile(std::string filename, std::ifstream&& input_stream, int width, int height)
            : filename_(filename), input_stream_(std::move(input_stream)), width_(width), height_(height) {}

    std::string filename() { return filename_; }
    std::ifstream& input_stream() { return input_stream_; }
    int width() { return width_; }
    int height() { return height_; }

private:
    std::string filename_;
    std::ifstream input_stream_;
    int width_;
    int height_;
};

std::vector<std::string> get_filenames_from_folder_path(std::string folder_path)
{
    std::vector<std::string> filenames;
    for (const auto& entry : std::experimental::filesystem::directory_iterator(folder_path)) {
        std::string filename = entry.path().filename().string();
        if (filename == ".gitignore")
            continue;
        if (std::experimental::filesystem::is_directory(entry))
            continue;
        filenames.push_back(filename);
    }

    return filenames;
}

InputFile create_input_file(std::string folder_path, std::string filename)
{
    std::ifstream input(folder_path + filename, std::ios::binary);

    if (input.fail())
        throw std::runtime_error("The filename was invalid.");

    int width;
    int height;
    int byte_size;
    input.read(reinterpret_cast<char*>(&width), sizeof(width));
    input.read(reinterpret_cast<char*>(&height), sizeof(height));
    input.read(reinterpret_cast<char*>(&byte_size), sizeof(byte_size));
    if (byte_size != sizeof(short))
        throw std::runtime_error("The depth pixels are not 16-bit.");

    return InputFile(filename, std::move(input), width, height);
}

// Converts 16-bit buffers into 8-bit OpenCV Mats.
cv::Mat create_depth_mat(int width, int height, const short* depth_buffer)
{
    int frame_size = width * height;
    std::vector<char> reduced_depth_frame(frame_size);
    std::vector<char> chroma_frame(frame_size);

    for (int i = 0; i < frame_size; ++i) {
        reduced_depth_frame[i] = depth_buffer[i] / 32;
        chroma_frame[i] = 128;
    }

    cv::Mat y_channel(height, width, CV_8UC1, reduced_depth_frame.data());
    cv::Mat chroma_channel(height, width, CV_8UC1, chroma_frame.data());

    std::vector<cv::Mat> y_cr_cb_channels;
    y_cr_cb_channels.push_back(y_channel);
    y_cr_cb_channels.push_back(chroma_channel);
    y_cr_cb_channels.push_back(chroma_channel);

    cv::Mat y_cr_cb_frame;
    cv::merge(y_cr_cb_channels, y_cr_cb_frame);

    cv::Mat bgr_frame = y_cr_cb_frame.clone();
    cvtColor(y_cr_cb_frame, bgr_frame, CV_YCrCb2BGR);
    return bgr_frame;
}

// Converts 8-bit buffers into 8-bit OpenCV Mats.
cv::Mat create_depth_mat2(int width, int height, const short* depth_buffer)
{
    int frame_size = width * height;
    std::vector<char> reduced_depth_frame(frame_size);
    std::vector<char> chroma_frame(frame_size);

    for (int i = 0; i < frame_size; ++i) {
        reduced_depth_frame[i] = depth_buffer[i];
        chroma_frame[i] = 128;
    }

    cv::Mat y_channel(height, width, CV_8UC1, reduced_depth_frame.data());
    cv::Mat chroma_channel(height, width, CV_8UC1, chroma_frame.data());

    std::vector<cv::Mat> y_cr_cb_channels;
    y_cr_cb_channels.push_back(y_channel);
    y_cr_cb_channels.push_back(chroma_channel);
    y_cr_cb_channels.push_back(chroma_channel);

    cv::Mat y_cr_cb_frame;
    cv::merge(y_cr_cb_channels, y_cr_cb_frame);

    cv::Mat bgr_frame = y_cr_cb_frame.clone();
    cvtColor(y_cr_cb_frame, bgr_frame, CV_YCrCb2BGR);
    return bgr_frame;
}

// Reads a depth video stream from the 'data' folder. Converts buffer to opencv
// matrix through create_depth_map function. Then applies compression and
// decompression. Visualizes the image before and after compression. Results
// should be identical.
void trvl_stream_test(){

    const std::string DATA_FOLDER_PATH = "../../data/";
    std::vector<std::string> filenames(get_filenames_from_folder_path(DATA_FOLDER_PATH));
    int filename_index = 1;
    std::string filename = filenames[filename_index];
    std::cout<<" file name: "<< filename <<std::endl;
    InputFile input_file(create_input_file(DATA_FOLDER_PATH, filename));

    int frame_size = input_file.width() * input_file.height();
    std::cout<<" width: "<< input_file.width() <<" width: "<< input_file.height() <<std::endl;

    int frame_count = 0;
    std::cout<<"frame: " << frame_count << std::endl;
    std::vector<short> depth_buffer(frame_size);
    input_file.input_stream().read(reinterpret_cast<char*>(depth_buffer.data()), frame_size * sizeof(short));
    //auto depth_mat = create_depth_mat(640, 576, depth_buffer.data());
    //cv::imshow("Depth", depth_mat);

    auto depth_mat = create_depth_mat(640, 576, depth_buffer.data());
    cv::imshow("Depth", depth_mat);
    //cv::imwrite( "../../depth.png", depth_mat);
    std::cout<<"depth map type? : " << depth_mat.type() << std::endl;
    std::cout << " depth channel" << depth_mat.depth() << ", " << depth_mat.channels() << std::endl;
    cv::waitKey(0);

    // Create encoder decoder
    short CHANGE_THRESHOLD = 10;
    int INVALIDATION_THRESHOLD = 2;
    trvl::Encoder encoder(frame_size, CHANGE_THRESHOLD, INVALIDATION_THRESHOLD);
    trvl::Decoder decoder(frame_size);


    bool keyframe = false;
    auto trvl_frame = encoder.encode(depth_buffer.data(), keyframe);
    auto depth_image = decoder.decode(trvl_frame.data(), keyframe);
    //auto depth_mat2 = create_depth_mat(640, 576, depth_image.data());
    auto depth_mat2 = create_depth_mat(640, 576, depth_image.data());
    std::cout<<"depth map type 2? : " << depth_mat2.type() << std::endl;
    std::cout << " depth channel 2" << depth_mat2.depth() << ", " << depth_mat2.channels() << std::endl;
    cv::imshow("Depth", depth_mat2);
    cv::waitKey(0);

}

// Reads a single depth image, which was already reduced from 16-bit to 8-bit.
// So after compression/decompression, one can convert the buffer to opencv mat
// through create_depth_mat2 function. Also, keyframe should be set to true.
void trvl_singledepth_test(){
    // Read depth image
    std::string depth_file = "../../depth.png";
    auto depth_mat = cv::imread(depth_file);

    cv::imshow("Depth", depth_mat);
    cv::waitKey(0);

    int width = depth_mat.cols;
    int height = depth_mat.rows;
    int frame_size = width * height;

    //std::vector<short> depth_buffer(frame_size);
    std::cout<<"depth map type? : " << depth_mat.type() << std::endl;
    std::cout<<"depth map size : " << depth_mat.size() << std::endl;
    std::cout << " depth channel:" << depth_mat.depth() << ", " << depth_mat.channels() << std::endl;
    //depth_mat.convertTo(depth_mat, CV_8U);
    std::vector<short>depth_buffer(depth_mat.begin<short>(), depth_mat.end<short>());

    // Create encoder decoder
    short CHANGE_THRESHOLD = 10;
    int INVALIDATION_THRESHOLD = 2;
    trvl::Encoder encoder(frame_size, CHANGE_THRESHOLD, INVALIDATION_THRESHOLD);
    trvl::Decoder decoder(frame_size);

    bool keyframe = true;
    auto trvl_frame = encoder.encode(depth_buffer.data(), keyframe);
    std::vector<short> depth_image = decoder.decode(trvl_frame.data(), keyframe);
    auto depth_mat2 = create_depth_mat2(640, 576, depth_image.data());
    std::cout<<"depth map type 2? : " << depth_mat2.type() << std::endl;
    std::cout<<"depth map size 2 : " << depth_mat2.size() << std::endl;
    std::cout << " depth channel 2: " << depth_mat2.depth() << ", " << depth_mat2.channels() << std::endl;
    cv::imshow("Depth2", depth_mat2);
    cv::waitKey(0);
}

int main(int argc, char **argv)
{

    //trvl_stream_test();
    //trvl_singledepth_test();

    const std::string DATA_FOLDER_PATH = "../../data/";
    std::vector<std::string> filenames(get_filenames_from_folder_path(DATA_FOLDER_PATH));
    int filename_index = 1;
    std::string filename = filenames[filename_index];
    InputFile input_file(create_input_file(DATA_FOLDER_PATH, filename));
    int frame_size = input_file.width() * input_file.height();
    std::vector<short> depth_buffer(frame_size);
    input_file.input_stream().read(reinterpret_cast<char*>(depth_buffer.data()), frame_size * sizeof(short));
    auto depth_mat = create_depth_mat(640, 576, depth_buffer.data());
    //cv::imshow("Depth original", depth_mat);
    //cv::waitKey(0);

    // Create encoder
    short CHANGE_THRESHOLD = 10;
    int INVALIDATION_THRESHOLD = 2;
    trvl::Encoder encoder(frame_size, CHANGE_THRESHOLD, INVALIDATION_THRESHOLD);

    // Init server
    rudp_server sender = rudp_server();
    std::cout<<"sender init" << std::endl;
    sender.Init("131.159.10.99", 9898);

    // Wait until first client is connected
    /* std::cout<<"clients? : " << sender.HasClients() << std::endl;
    while(!sender.HasClients()){
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    std::cout<<"Client connected! " << std::endl;

    //while (!input_file.input_stream().eof()) {

    std::this_thread::sleep_for(std::chrono::milliseconds(200)); */
    // Send the depth image to the client
    int frame_count = 0;
    //while(true){
    while (!input_file.input_stream().eof()) {
        // std::vector<short> depth_buffer(frame_size);
        input_file.input_stream().read(reinterpret_cast<char*>(depth_buffer.data()), frame_size * sizeof(short));
        bool keyframe = true; //frame_count++ % 30 == 0;
        std::cout<<"Keyframe sent:  "<< keyframe << " data size: " << frame_size * sizeof(short) << std::endl;
        auto trvl_frame = encoder.encode(depth_buffer.data(), keyframe);
        std::cout<<"Encoded.  "<< std::endl;
        sender.Send(trvl_frame.data(), frame_size * sizeof(short));

        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        if (cv::waitKey(1) >= 0)
            return 1;
    }

    sender.CleanUp();
    return 1;
}
