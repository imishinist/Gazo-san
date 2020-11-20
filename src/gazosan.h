#pragma once

#include <sys/stat.h>  //for mkdir for Linux

#include <chrono>
#include <ctime>  // for tm
#include <iomanip>
#include <iostream>  // for std::string
#include <map>
#include <opencv2/features2d.hpp>
#include <opencv2/highgui/highgui.hpp>  // for highgui module
#include <opencv2/imgproc/imgproc.hpp>
#include <vector>  // for std::vector

#include "cxxopts.hpp"  // for option phrase

// time.cpp
namespace gazosan {
enum TimeFormat {
  YYYYMMDD = 1,
  HHMMSS = 2,
};

std::chrono::system_clock::time_point now();
std::string format_date(const std::chrono::system_clock::time_point& now,
                        const TimeFormat& format_enum);
}  // namespace gazosan

// pixel connectivity
struct PixelConnectivity {
  int nIdx{};
  std::vector<int> nNeighborIdxList;
};
// segmented region information
struct SegmentedRegionInfo {
  cv::Point ptOrigin;
  int nW{};
  int nH{};
  cv::Scalar clrFrame;
  std::vector<cv::Point> ptPixList;
};

////////// Global function //////////
int ImgSegMain(int argc, const char** argv);
int ImgSeg00(const std::string& strOldImgFile,
             const std::string& strNewImgFile);
void ImgSeg01(const std::string& strImgFile,
              const std::string& strOutputFolder);
void ImgSeg02(const std::string& strOldFile,
              const std::vector<std::string>& strOldPartFileList,
              const std::string& strNewFile,
              const std::vector<std::string>& strNewPartFileList,
              const std::string& strOutputFolder);
void ImgSeg03(const std::string& strOldFile,
              std::map<int, std::vector<std::string> > strPartFileListMap,
              const std::string& strOutputFolder);

void ExecuteFeatureDetectorAndMatching(
    const std::vector<std::string>& strOldPartFileList,
    const std::vector<std::string>& strNewPartFileList,
    std::map<int, std::vector<std::string> >& strMap);
void ComputeKeypointAndDescriptor(
    const std::vector<std::string>& strPartFileList,
    std::map<std::string, cv::Mat>& strMap);
void ExecuteTemplateMatch(const std::string& strImgFile,
                          const std::vector<std::string>& strPartFileList,
                          cv::Mat& clrImg,
                          std::vector<SegmentedRegionInfo>& segRegionInfoList);
void ExecuteTemplateMatchEx(
    const std::string& strImgFile,
    const std::vector<std::string>& strPartFileList, cv::Mat& clrImg,
    std::vector<SegmentedRegionInfo>& segRegionInfoList);

void CreateDirectory(const std::string& strFolderPath);
std::vector<std::string> Split(const std::string& s, const std::string& delim);
std::vector<std::string> Split(const std::string& s, char delim);

void ConvertColorToGray(cv::Mat& img);
unsigned char* ConvertCVMATtoUCHAR(const cv::Mat& img, const int& nH = -1,
                                   const int& nW = -1);

void CreatePNGfromCVMAT(const int& nNum, const cv::Mat& img,
                        const std::string& strOutputFolder);
void CreatePNGfromUCHAR(const int& nNum, const int& nW, const int& nH,
                        unsigned char* pImg,
                        const std::string& strOutputFolder);
void CreateBMP(const std::string& strFile, const int& nW, const int& nH,
               const unsigned char* pImg);
void ConvertBMPtoPNG(const std::string& strBMPFile,
                     const std::string& strPNGFile);
std::string GetPNGFile(const int& nNum, const std::string& strOutputFolder);

bool GetGroupedDataTest(const int& nSrcW, const int& nSrcH,
                        const unsigned char* pSrcImg,
                        std::vector<std::vector<PixelConnectivity*>*>& solid);

inline void SetProcessStartMsg(const std::string& strFuncName,
                               const int& nStepNo,
                               const std::string& strStepName) {
  using namespace gazosan;
  std::string strHHMMSS = format_date(now(), TimeFormat::HHMMSS);
  std::clog << strFuncName << " : Step" << nStepNo << ". " << strStepName
            << " --START ( " << strHHMMSS << " )--" << std::endl;
}
inline void SetProcessEndMsg(const std::string& strFuncName, const int& nStepNo,
                             const std::string& strStepName) {
  using namespace gazosan;
  std::string strHHMMSS = format_date(now(), TimeFormat::HHMMSS);
  std::clog << strFuncName << " : Step" << nStepNo << ". " << strStepName
            << " --END ( " << strHHMMSS << " )--" << std::endl;
}
inline void SetProcessErrorMsg(const int& nStepNo) {
  std::clog << " -> Error : Step" << nStepNo << std::endl;
}
