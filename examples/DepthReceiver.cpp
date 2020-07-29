
#include "rudp_client.h"

#include <experimental/filesystem>
#include <fstream>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/types_c.h>

#include "trvl.h"

static bool output = true;

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

int main(int argc, char **argv)
{
  rudp_client receiver = rudp_client();

  int frame_size = 640 * 576;

  trvl::Decoder decoder(frame_size);
  int frame_count = 0;

  receiver.OnReceive = [&](const char *data, size_t dataLength) {
    printf("Received: %i bytes\n", (unsigned int)dataLength);

    bool keyframe = true; //frame_count++ % 30 == 0;
    std::cout<<"Keyframe received:  "<< keyframe << std::endl;
    auto depth_image = decoder.decode(const_cast<char *>(data), keyframe);
    auto depth_mat = create_depth_mat(640, 576, depth_image.data());
    //auto depth_mat = create_depth_mat2(640, 576, depth_image.data());
    cv::imshow("Depth received", depth_mat);

    if (cv::waitKey(1) >= 0)
        return 1;
    // ================= END Substitute by buffer sink
  };

  while(!receiver.Connect("131.159.10.99", 9898));

  // Keep application alive
  std::getchar();

  receiver.CleanUp();
  return 0;
}
