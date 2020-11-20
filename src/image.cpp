#include "gazosan.h"

namespace gazosan {

int hist_size[] = {256, 256};
float h_ranges[] = {0, 180};
float s_ranges[] = {0, 256};
const float* ranges[] = {h_ranges, s_ranges};

int channels[] = {0, 1};

cv::Mat histogram(cv::Mat bgr_image) {
  assert(bgr_image.data != nullptr);

  cv::Mat hsv_image;
  cv::cvtColor(bgr_image, hsv_image, cv::COLOR_BGR2HSV);

  cv::Mat hist;
  cv::calcHist(&hsv_image, 1, channels, cv::Mat(), hist, 2, hist_size, ranges,
               true, false);
  cv::normalize(hist, hist, 0, 1, cv::NORM_MINMAX, -1, cv::Mat());

  return hist;
}
}  // namespace gazosan
