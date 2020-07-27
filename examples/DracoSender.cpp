
#include "draco/io/mesh_io.h"
#include "draco/io/point_cloud_io.h"
#include "mesh_object_reader.h"
#include "mesh_object.h"
#include "rudp_server.h"

#include <experimental/filesystem>
#include <fstream>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/types_c.h>

#include "trvl.h"

[[noreturn]] void send_mesh(){
    std::vector<float> vertices
            {
                    0, 0, 0,
                    1, 0, 0,
                    1, 1, 0,
                    0, 1, 0,
                    0, 1, 1,
                    1, 1, 1,
                    1, 0, 1,
                    0, 0, 1
            };

    std::vector<float> normals
            {
                    1, 0, 0,
                    1, 0, 0,
                    1, 0, 0,
                    1, 0, 0,
                    1, 0, 0,
                    1, 0, 0,
                    1, 0, 0,
                    1, 0, 0,
            };

    std::vector<int> faces
            {
                    0, 2, 1, //face front
                    0, 3, 2,
                    2, 3, 4, //face top
                    2, 4, 5,
                    1, 2, 5, //face right
                    1, 5, 6,
                    0, 7, 4, //face left
                    0, 4, 3,
                    5, 4, 7, //face back
                    5, 7, 6,
                    0, 6, 7, //face bottom
                    0, 1, 6
            };

    std::vector<float> uv
            {
                    0, 1,
                    0, 1,
                    0, 1,
                    0, 1,
                    0, 1,
                    0, 1,
                    0, 1,
                    0, 1
            };


    vertices.clear();
    int gridMax = 200;
    for(int x=0;x<gridMax;x++)
    {
        for(int y=0;y<gridMax;y++)
        {
            for(int z=0;z<gridMax;z++)
            {
                vertices.push_back(x);
                vertices.push_back(y);
                vertices.push_back(z);
            }
        }
    }

    // ============= Mesh Sender
    std::cout<<"server start" << std::endl;

    rudp_server sender = rudp_server();

    std::cout<<"sender init" << std::endl;

    sender.Init("131.159.10.99", 9898);

    float updateRateMS = 1.0 / 15;

    while (true)
    {
        mesh_object mesh = mesh_object();
        mesh.vertices = vertices;
        //mesh.normals = normals;
        //mesh.faces = faces;
        //mesh.uv = uv;

        sender.Send(&mesh);

        //usleep((int)(updateRateMS * 1000 * 1000)); // Limit Bitrate
    }

    sender.CleanUp();
    // ============= END Mesh Sender

}

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

// Converts 16-bit buffers into OpenCV Mats.
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

void trvl_stuff(){

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
    cv::imwrite( "../../depth.png", depth_mat );

    //if (cv::waitKey(1) >= 0)
        //return 1;

    //depth_mat.convertTo(depth_mat, CV_16U);
    //std::cout<<"depth map type? : " << depth_mat.type() << std::endl;
    //CV_Assert(depth_mat.type() == CV_16U);
    short *ps = reinterpret_cast<short *>(depth_mat.ptr<ushort>());

    input_file.input_stream().read(reinterpret_cast<char*>(ps), frame_size * sizeof(short));
}

int main(int argc, char **argv)
{

    // Read depth image
    std::string depth_file = "../../depth.png";
    auto depth_mat = cv::imread(depth_file);
    cv::imshow("Depth", depth_mat);

    int width = depth_mat.cols;
    int height = depth_mat.rows;
    int frame_size = width * height;

    //std::vector<short> depth_buffer(frame_size);
    std::cout<<"depth map type? : " << depth_mat.type() << std::endl;
    short *depth_buffer = reinterpret_cast<short *>(depth_mat.ptr<ushort>());

    // Create encoder decoder
    short CHANGE_THRESHOLD = 10;
    int INVALIDATION_THRESHOLD = 2;
    trvl::Encoder encoder(frame_size, CHANGE_THRESHOLD, INVALIDATION_THRESHOLD);
    trvl::Decoder decoder(frame_size);

    // Init server
    rudp_server sender = rudp_server();
    std::cout<<"sender init" << std::endl;
    sender.Init("131.159.10.99", 9898);

    // Wait until first client is connected
    std::cout<<"clients? : " << sender.HasClients() << std::endl;
    while(!sender.HasClients()){
        std::this_thread::sleep_for(std::chrono::milliseconds(700));
    }
    std::cout<<"Client connected! " << std::endl;
    int frame_count = 0;
    //while (!input_file.input_stream().eof()) {

    // Send the depth image to the client
    while(true){
        bool keyframe = frame_count++ % 30 == 0;
        auto trvl_frame = encoder.encode(depth_buffer, keyframe);
        sender.Send(trvl_frame.data(), frame_size * sizeof(short));

        /*auto depth_image = decoder.decode(trvl_frame.data(), keyframe);
        auto depth_mat = create_depth_mat(640, 576, depth_image.data());

        cv::imshow("Depth", depth_mat);
        if (cv::waitKey(1) >= 0)
            return 1; */
    }

    sender.CleanUp();
    return 1;
}
