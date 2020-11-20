#include <iostream>

#include "gazosan.h"

int main(int argc, const char** argv) {
  using namespace gazosan;
  auto start = format_date(gazosan::now(), TimeFormat::HHMMSS);

  std::cout << "Start detection" << std::endl;
  // ImgSeg
  ImgSegMain(argc, argv);

  auto end = format_date(gazosan::now(), TimeFormat::HHMMSS);
  std::cout << "Process time : " << start << " - " << end << std::endl;

  return 0;
}