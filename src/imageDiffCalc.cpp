///////////////////////////////////////////////////////////////////////////
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:

//   * Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.

//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.

//   * Neither the names of the copyright holders nor the names of the
//   contributors
//     may be used to endorse or promote products derived from this software
//     without specific prior written permission.

// This software is provided by the copyright holders and contributors "as is"
// and any express or implied warranties, including, but not limited to, the
// implied warranties of merchantability and fitness for a particular purpose
// are disclaimed. In no event shall copyright holders or contributors be liable
// for any direct, indirect, incidental, special, exemplary, or consequential
// damages (including, but not limited to, procurement of substitute goods or
// services; loss of use, data, or profits; or business interruption) however
// caused and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
///////////////////////////////////////////////////////////////////////////

#include "gazosan.h"

////////// Global variables //////////
// old/new part file list (Relative Path)
std::vector<std::string> g_strOldPartFileList;
std::vector<std::string> g_strNewPartFileList;

// output file name
std::string g_strFileName;
// file list
std::vector<std::string> g_strFileList;
// between files difference info ([0]/[1] : same/remove image between old and
// new, [2]/[3] : same/add image between new and old)
std::map<int, std::vector<std::string> > g_strFileDiffInfoListMap;

// part frame color list for rectangle
std::vector<cv::Vec3b> g_clrPartFrameList;
unsigned int g_nClrPartFrameIndex;

int g_nPartFileNo = 0;
bool g_bCreateChangeImg = false;

////////////////////////////////////////////////////////////////////////////////////////////////////
int ImgSegMain(int argc, const char** argv) {
  std::clog.setstate(std::ios_base::failbit);
  std::string strOldFile, strNewFile;
  // Set the options
  cxxopts::Options options("options");
  try {
    options.add_options()("new_image", "New image file path",
                          cxxopts::value<std::string>(strNewFile))(
        "old_image", "Old image file path",
        cxxopts::value<std::string>(strOldFile))(
        "o,output_name", "Output prefix name",
        cxxopts::value<std::string>(g_strFileName)
            ->default_value("image_difference"))(
        "v,verbose", "Enable verbose output message")(
        "create-change-image",
        "Generate 2 more output files. 1.Output_delete.png: An image shows "
        "decreasing part as the green rectangle on the old image. "
        "2.Output_add.png: An image shows increasing part as the green "
        "rectangle on the new image.")("h,help", "Print help");
    options.parse_positional({"new_image", "old_image", "output_name"});

    auto result = options.parse(argc, argv);
    if (result.count("help")) {
      std::cout << options.help() << std::endl;
      return 0;
    }
    if (result.count("new_image") == false ||
        result.count("old_image") == false) {
      std::cerr << "Not enough input" << std::endl;
      std::cerr << " -> 1st : Relative path for new color image file"
                << std::endl;
      std::cerr << " -> 2nd : Relative path for old color image file"
                << std::endl;
      std::cerr
          << " -> 3rd : Result file name prefix (default: image_difference) "
          << std::endl;
      return -1;
    }
    if (result.count("verbose")) {
      std::clog.clear();
    }
    if (result.count("create-change-image")) {
      g_bCreateChangeImg = true;
    }
  } catch (cxxopts::OptionException& e) {
    std::cerr << e.what() << std::endl;
    return -1;
  }

  // ImgSeg00
  {
    int ImgSeg00_return = ImgSeg00(strOldFile, strNewFile);
    if (ImgSeg00_return == -2) {
      std::cerr << "Can't load images." << std::endl;
      return -1;
    } else if (ImgSeg00_return == -1) {
      std::cerr << "There isn't any difference in those images." << std::endl;
      return -1;
    }
  }

  auto context = std::make_shared<gazosan::Context>();
  // ImgSeg01
  {
    // create new folder under temporary folder, and parts division
    std::string strOutputFolder = "/tmp/image_diff_temp/new/";
    CreateDirectory(strOutputFolder);
    g_nPartFileNo = 1;
    ImgSeg01(context, strNewFile, strOutputFolder);
    g_strNewPartFileList = g_strFileList;
    g_strFileList.clear();

    // create old folder under temporary folder, and parts division
    strOutputFolder = "/tmp/image_diff_temp/old/";
    CreateDirectory(strOutputFolder);
    g_nPartFileNo = 1;
    ImgSeg01(context, strOldFile, strOutputFolder);
    g_strOldPartFileList = g_strFileList;
    g_strFileList.clear();
  }

  // ImgSeg02
  {
    std::string strOutputFolder = "./";
    ImgSeg02(context, strOldFile, g_strOldPartFileList, strNewFile,
             g_strNewPartFileList, strOutputFolder);
  }

  // ImgSeg03
  {
    std::string strOutputFolder = "./";
    ImgSeg03(context, strOldFile, g_strFileDiffInfoListMap, strOutputFolder);
    if (system("exec rm -r /tmp/image_diff_temp") != 0) {
      std::cerr << "Fail in delete temp directoty." << std::endl;
      return -1;
    }
  }

  return 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
int ImgSeg00(const std::string& strOldImgFile,
             const std::string& strNewImgFile) {
  cv::Mat clrOldImg = cv::imread(strOldImgFile, cv::IMREAD_COLOR);
  cv::Mat clrNewImg = cv::imread(strNewImgFile, cv::IMREAD_COLOR);
  if (clrOldImg.data == nullptr || clrNewImg.data == nullptr) {
    return -2;
  }

  cv::Mat hsvOldImg, hsvNewImg;
  cv::cvtColor(clrOldImg, hsvOldImg, cv::COLOR_BGR2HSV);
  cv::cvtColor(clrNewImg, hsvNewImg, cv::COLOR_BGR2HSV);

  int nHistSize[] = {256, 256};
  float fHRanges[] = {0, 180};
  float fSRanges[] = {0, 256};
  const float* pRanges[] = {fHRanges, fSRanges};

  int nChannels[] = {0, 1};

  cv::Mat histOld, histNew;
  cv::calcHist(&hsvOldImg, 1, nChannels, cv::Mat(), histOld, 2, nHistSize,
               pRanges, true, false);
  cv::normalize(histOld, histOld, 0, 1, cv::NORM_MINMAX, -1, cv::Mat());
  cv::calcHist(&hsvNewImg, 1, nChannels, cv::Mat(), histNew, 2, nHistSize,
               pRanges, true, false);
  cv::normalize(histNew, histNew, 0, 1, cv::NORM_MINMAX, -1, cv::Mat());

  double dOldToNew = cv::compareHist(histOld, histNew, 1);
  std::clog << " Compare Old to New : " << dOldToNew << std::endl;

  if (dOldToNew <= 0.00001) {
    return -1;
  }
  return 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void ImgSeg01(std::shared_ptr<gazosan::Context> context,
              const std::string& strImgFile,
              const std::string& strOutputFolder) {
  context->reset("ImgSeg01");

  // Step1 : load image
  context->set_step("Load image");
  context->start_message();
  cv::Mat clrImg = cv::imread(strImgFile, cv::IMREAD_COLOR);
  if (clrImg.data == nullptr) {
    context->error_message();
    return;
  }
  context->end_message();
  // Step1 : load image

  // Step2 : color -> gray -> binary
  context->set_step("Transform image color -> gray -> binary");
  context->start_message();
  cv::Mat gryImg;
  cv::cvtColor(clrImg, gryImg, cv::COLOR_BGR2GRAY);

  cv::Mat binImg;
  cv::threshold(gryImg, binImg, 200, 255, cv::THRESH_BINARY);
  context->end_message();
  // Step2 : color -> gray -> binary

  // Step 3 : morphology process
  context->set_step("Morphology process");
  context->start_message();
  int nIter = 7;
  cv::Mat grdImg;
  // cv::Mat kernel(3, 3, CV_8U, cv::Scalar(1)); // =cv::MORPH_RECT
  cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
  cv::morphologyEx(binImg, grdImg, cv::MORPH_GRADIENT, kernel,
                   cv::Point(-1, -1), nIter);
  context->end_message();
  // Step 3 : morphology process

  // Step 4 : find contours and auto labeling
  context->set_step("Find contours and Auto labeling");
  context->start_message();
  int compCount = 0;
  std::vector<std::vector<cv::Point> > contours;
  std::vector<cv::Vec4i> hierarchy;
  grdImg.convertTo(grdImg, CV_32SC1, 1.0);
  cv::findContours(grdImg, contours, hierarchy, cv::RETR_CCOMP,
                   cv::CHAIN_APPROX_SIMPLE);
  if (contours.empty()) {
    context->error_message();
    return;
  }
  cv::Mat markers = cv::Mat::zeros(grdImg.rows, grdImg.cols, CV_32SC1);
  int idx = 0;
  for (; idx >= 0; idx = hierarchy[idx][0], compCount++) {
    cv::drawContours(markers, contours, idx, cv::Scalar::all(compCount + 1), -1,
                     8, hierarchy, INT_MAX);
    markers = markers + 1;
  }
  context->end_message();
  // Step 4 : find contours and auto labeling

  // Step 5 : watershed
  context->set_step("Watershed process");
  context->start_message();
  cv::watershed(clrImg, markers);
  context->end_message();
  // Step 5 : watershed

  // Step 6 : change color and create watershed png image
  context->set_step("Change color and Create Watershed png image");
  context->start_message();
  cv::Mat wsdImg(markers.size(), CV_8UC3);
  for (int y = 0; y < markers.rows; ++y) {
    for (int x = 0; x < markers.cols; ++x) {
      int index = markers.at<int>(y, x);
      if (index == -1) {
        wsdImg.at<cv::Vec3b>(y, x) = cv::Vec3b(128, 128, 128);
      } else if (index == 0 || index > compCount) {
        wsdImg.at<cv::Vec3b>(y, x) = cv::Vec3b(0, 0, 0);
      } else {
        wsdImg.at<cv::Vec3b>(y, x) = cv::Vec3b(128, 128, 128);
      }
    }
  }
  CreatePNGfromCVMAT(0, wsdImg, strOutputFolder);
  context->end_message();
  // Step 6 : change color and allocate memory for watershed png

  // Step 7 : grouping and create png for each parts
  context->set_step("Grouping and Create png for each parts");
  context->start_message();
  int nH = wsdImg.rows;
  int nW = wsdImg.cols;
  unsigned char* pSrcImg = ConvertCVMATtoUCHAR(wsdImg);
  std::vector<std::vector<PixelConnectivity*>*> solid;
  GetGroupedDataTest(nW, nH, pSrcImg, solid);
  delete[] pSrcImg;
  pSrcImg = nullptr;

  for (unsigned int i = 0; i < solid.size(); ++i) {
    std::vector<PixelConnectivity*>* pShell = solid.at(i);

    int nMinX = 2 * nW;
    int nMinY = 2 * nH;
    int nMaxX = -1;
    int nMaxY = -1;
    for (auto pPix : *pShell) {
      int y = pPix->nIdx / nW;
      int x = pPix->nIdx % nW;
      if (x < nMinX) nMinX = x;
      if (y < nMinY) nMinY = y;
      if (x > nMaxX) nMaxX = x;
      if (y > nMaxY) nMaxY = y;
    }  // for(j)

    int w = nMaxX - nMinX + 1;
    int h = nMaxY - nMinY + 1;
    auto* pDstImg = new unsigned char[w * h * 3];
    for (int y = nMinY; y < nMinY + h; ++y) {
      int dstImgIdx = (y - nMinY) * w * 3;
      for (int x = nMinX; x < nMinX + w; ++x) {
        pDstImg[dstImgIdx + 0] = clrImg.at<cv::Vec3b>(y, x)[0];
        pDstImg[dstImgIdx + 1] = clrImg.at<cv::Vec3b>(y, x)[1];
        pDstImg[dstImgIdx + 2] = clrImg.at<cv::Vec3b>(y, x)[2];
        dstImgIdx += 3;
      }  // for(x)
    }    // for(y)
    CreatePNGfromUCHAR(i + 1, w, h, pDstImg, strOutputFolder);
    delete[] pDstImg;
    pDstImg = nullptr;
  }  // for(i)
  context->end_message();
  // Step 7 : grouping and create png for each parts

  std::clog << "\n" << std::endl;
}
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void ImgSeg02(std::shared_ptr<gazosan::Context> context,
              const std::string& strOldFile,
              const std::vector<std::string>& strOldPartFileList,
              const std::string& strNewFile,
              const std::vector<std::string>& strNewPartFileList,
              const std::string& strOutputFolder) {
  context->reset("ImgSeg02");

  /* old <-> new */
  // Step1 : feature detector and matching between base and target image
  context->set_step("Feature detector and matching between old and new image");
  context->start_message();
  std::clog << "  old (" << strOldPartFileList.size() << ")"
            << " <-> new (" << strNewPartFileList.size() << ")" << std::endl;
  ExecuteFeatureDetectorAndMatching(strOldPartFileList, strNewPartFileList,
                                    g_strFileDiffInfoListMap);
  context->end_message();
  // Step1 : feature detector and matching between base and target image

  if (g_bCreateChangeImg) {
    // Step2 : create base image with difference part frame
    context->set_step("Create base image with difference part frame");
    context->start_message();
    std::clog << "  old difference parts ("
              << g_strFileDiffInfoListMap[1].size() << ")" << std::endl;
    cv::Mat clrOldImg;
    std::vector<SegmentedRegionInfo> oldSegRegionInfoList;
    ExecuteTemplateMatch(strOldFile, g_strFileDiffInfoListMap[1], clrOldImg,
                         oldSegRegionInfoList);
    for (const auto& info : oldSegRegionInfoList) {
      cv::rectangle(
          clrOldImg, info.ptOrigin,
          cv::Point(info.ptOrigin.x + info.nW, info.ptOrigin.y + info.nH),
          info.clrFrame, 2);
    }
    CreatePNGfromCVMAT(8000, clrOldImg, strOutputFolder);

    std::clog << "  new difference parts ("
              << g_strFileDiffInfoListMap[3].size() << ")" << std::endl;
    cv::Mat clrNewImg;
    std::vector<SegmentedRegionInfo> newSegRegionInfoList;
    ExecuteTemplateMatch(strNewFile, g_strFileDiffInfoListMap[3], clrNewImg,
                         newSegRegionInfoList);
    for (const auto& info : newSegRegionInfoList) {
      cv::rectangle(
          clrNewImg, info.ptOrigin,
          cv::Point(info.ptOrigin.x + info.nW, info.ptOrigin.y + info.nH),
          info.clrFrame, 2);
    }
    CreatePNGfromCVMAT(9000, clrNewImg, strOutputFolder);
    context->end_message();
    // Step2 : Create base image with difference part frame
  }

  std::clog << "\n" << std::endl;
}
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void ImgSeg03(std::shared_ptr<gazosan::Context> context,
              const std::string& strOldFile,
              std::map<int, std::vector<std::string> > strPartFileListMap,
              const std::string& strOutputFolder) {
  context->reset("ImgSeg03");

  // Step 1 : check template match for old file and new->old same part files
  context->set_step(
      "Check template match for old file and new->old same part files");
  context->start_message();
  cv::Mat oldClrImg;
  std::vector<SegmentedRegionInfo> newSegRegionInfoList;
  ExecuteTemplateMatchEx(strOldFile, strPartFileListMap[2], oldClrImg,
                         newSegRegionInfoList);
  context->end_message();
  // Step 1 : check template match for old file and new->old same part files

  // Step 2 : draw information in old file
  context->set_step("Draw information in old file");
  context->start_message();
  cv::Mat oldImg = oldClrImg;
  ConvertColorToGray(oldImg);
  for (auto info : newSegRegionInfoList) {
    cv::rectangle(
        oldImg, info.ptOrigin,
        cv::Point(info.ptOrigin.x + info.nW, info.ptOrigin.y + info.nH),
        info.clrFrame, 0);
    for (unsigned int k = 0; k < info.ptPixList.size(); ++k) {
      cv::Point pt = info.ptPixList.at(k);
      int x = info.ptOrigin.x + pt.x;
      int y = info.ptOrigin.y + pt.y;
      oldImg.at<cv::Vec3b>(y, x) =
          cv::Vec3b(info.clrFrame[0], info.clrFrame[1], info.clrFrame[2]);
    }
  }
  CreatePNGfromCVMAT(10000, oldImg, strOutputFolder);
  context->end_message();
  // Step 2 : draw information in old file

  std::clog << "\n" << std::endl;
}
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void ExecuteFeatureDetectorAndMatching(
    const std::vector<std::string>& strOldPartFileList,
    const std::vector<std::string>& strNewPartFileList,
    std::map<int, std::vector<std::string> >& strMap) {
  std::clog << "   Compute 'key points' and 'descriptor' of old part"
            << std::endl;
  std::map<std::string, cv::Mat> strOldPartDescriptorInfoMap;
  ComputeKeypointAndDescriptor(strOldPartFileList, strOldPartDescriptorInfoMap);

  std::clog << "   Compute 'key points' and 'descriptor' of new part"
            << std::endl;
  std::map<std::string, cv::Mat> strNewPartDescriptorInfoMap;
  ComputeKeypointAndDescriptor(strNewPartFileList, strNewPartDescriptorInfoMap);

  std::clog << "   Compute 'feature match' of old to new part" << std::endl;
  cv::Ptr<cv::DescriptorMatcher> matcher =
      cv::DescriptorMatcher::create("FlannBased");
  std::map<std::string, std::string> strMatchedPartFilesMap;
  // old -> new
  {
    unsigned int i = 0;
    for (auto& itrOld : strOldPartDescriptorInfoMap) {
      std::clog << "    Old No. " << ++i << " : " << std::flush;
      // std::string strOldPartFile = itrOld->first;
      // cv::Mat desOldPart = itrOld->second;

      bool bIsMatched = false;
      if (itrOld.second.data == nullptr) {
        std::clog << "key point size = 0." << std::endl;
      } else {
        std::clog << "" << std::endl;

        unsigned int j = 0;
        for (auto itrNew = strNewPartDescriptorInfoMap.begin();
             itrNew != strNewPartDescriptorInfoMap.end(); ++itrNew) {
          std::clog << "     New No." << ++j << " : " << std::flush;
          // std::string strNewPartFile = itrNew->first;
          // cv::Mat desNewPart = itrNew->second;

          std::vector<cv::DMatch> matches;
          if (itrOld.second.data && itrNew->second.data) {
            matcher->match(itrOld.second, itrNew->second, matches);
            std::sort(matches.begin(),
                      matches.end());  // sorted by cv::DMatch::distance
          }

          if (!matches.empty() &&
              matches[matches.size() / 2].distance <= 1.0f) {
            std::clog << "Match" << std::endl;
            bIsMatched = true;  // full or almost match
            strMatchedPartFilesMap[itrNew->first] = itrOld.first;
            strNewPartDescriptorInfoMap.erase(itrNew);
            break;
          } else {
            std::clog << "No Match" << std::endl;
          }
        }  // for(j)
      }

      if (bIsMatched && g_bCreateChangeImg) {
        strMap[0].push_back(itrOld.first);
      } else {
        strMap[1].push_back(itrOld.first);
      }
    }  // for(i)
  }

  // new -> old
  {
    std::clog << "   Compute 'feature match' of new to old part" << std::endl;
    unsigned int j = 0;
    std::vector<std::string> strList = strNewPartFileList;
    for (auto& itr : strList) {
      std::clog << "    New No. " << ++j << " : " << std::flush;

      if (strMatchedPartFilesMap.find(itr) != strMatchedPartFilesMap.end()) {
        std::clog << "Match" << std::endl;
        strMap[2].push_back(itr);
      } else if (g_bCreateChangeImg) {
        if (strNewPartDescriptorInfoMap.find(itr)->second.data) {
          std::clog << "No Match" << std::endl;
        } else {
          std::clog << "key point size = 0." << std::endl;
        }
        strMap[3].push_back(itr);
      }
    }
  }
}
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void ComputeKeypointAndDescriptor(
    const std::vector<std::string>& strPartFileList,
    std::map<std::string, cv::Mat>& strMap) {
  std::vector<std::string> strCopiedPartFileList = strPartFileList;

  cv::Ptr<cv::AKAZE> akaze = cv::AKAZE::create();

  unsigned int i = 0;
  for (auto& itr : strCopiedPartFileList) {
    std::clog << "    File No. " << ++i << " : " << std::flush;

    // std::string strBasePartFile = *itr;
    cv::Mat gryImg = cv::imread(itr, cv::IMREAD_GRAYSCALE);
    if (gryImg.data == nullptr) {
      continue;
    }

    std::vector<cv::KeyPoint> kpList;
    akaze->detect(gryImg, kpList);
    if (kpList.empty()) {
      std::clog << "key point size = 0." << std::endl;
      strMap[itr] = cv::Mat();
      continue;
    }
    cv::Mat descriptors;
    akaze->compute(gryImg, kpList, descriptors);
    descriptors.convertTo(descriptors, CV_32F);
    strMap[itr] = descriptors;
    std::clog << "OK" << std::endl;
  }
}
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void ExecuteTemplateMatch(const std::string& strImgFile,
                          const std::vector<std::string>& strPartFileList,
                          cv::Mat& clrImg,
                          std::vector<SegmentedRegionInfo>& segRegionInfoList) {
  // current image
  cv::Mat curClrImg, curGryImg;
  curClrImg = cv::imread(strImgFile, cv::IMREAD_COLOR);
  cv::cvtColor(curClrImg, curGryImg, cv::COLOR_BGR2GRAY);
  for (const auto& strPartFile : strPartFileList) {
    // part image
    cv::Mat partClrImg = cv::imread(strPartFile, cv::IMREAD_COLOR);
    if (partClrImg.data == nullptr) {
      continue;
    }
    cv::Mat partGryImg;
    cv::cvtColor(partClrImg, partGryImg, cv::COLOR_BGR2GRAY);

    // global minimum
    double dMinVal;
    cv::Point ptMin;
    cv::Mat retImg;
    cv::matchTemplate(curGryImg, partGryImg, retImg, cv::TM_SQDIFF);
    cv::minMaxLoc(retImg, &dMinVal, nullptr, &ptMin, nullptr);

    int nPartH = partClrImg.rows;
    int nPartW = partClrImg.cols;

    SegmentedRegionInfo info;
    info.ptOrigin = ptMin;
    info.nW = nPartW;
    info.nH = nPartH;
    info.clrFrame = CV_RGB(0, 255, 0);
    segRegionInfoList.push_back(info);
  }

  clrImg = curClrImg;
}
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void ExecuteTemplateMatchEx(
    const std::string& strImgFile,
    const std::vector<std::string>& strPartFileList, cv::Mat& clrImg,
    std::vector<SegmentedRegionInfo>& segRegionInfoList) {
  // current image
  cv::Mat curClrImg, curGryImg;
  curClrImg = cv::imread(strImgFile, cv::IMREAD_COLOR);
  cv::cvtColor(curClrImg, curGryImg, cv::COLOR_BGR2GRAY);
  for (const auto& strPartFile : strPartFileList) {
    // part image
    cv::Mat partClrImg = cv::imread(strPartFile, cv::IMREAD_COLOR);
    if (partClrImg.data == nullptr) {
      continue;
    }
    cv::Mat partGryImg;
    cv::cvtColor(partClrImg, partGryImg, cv::COLOR_BGR2GRAY);

    // global minimum
    double dMinVal;
    cv::Point ptMin;
    cv::Mat retImg;
    cv::matchTemplate(curGryImg, partGryImg, retImg, cv::TM_SQDIFF);
    cv::minMaxLoc(retImg, &dMinVal, nullptr, &ptMin, nullptr);

    int nPartH = partClrImg.rows;
    int nPartW = partClrImg.cols;
    int nXs = ptMin.x;
    int nYs = ptMin.y;
    int nXe = nXs + nPartW;
    int nYe = nYs + nPartH;
    std::vector<cv::Point> ptPixList;
    for (int y = nYs; y < nYe; ++y) {
      for (int x = nXs; x < nXe; ++x) {
        unsigned char clrCur[3] = {curClrImg.at<cv::Vec3b>(y, x)[0],
                                   curClrImg.at<cv::Vec3b>(y, x)[1],
                                   curClrImg.at<cv::Vec3b>(y, x)[2]};
        unsigned char clrPart[3] = {
            partClrImg.at<cv::Vec3b>(y - nYs, x - nXs)[0],
            partClrImg.at<cv::Vec3b>(y - nYs, x - nXs)[1],
            partClrImg.at<cv::Vec3b>(y - nYs, x - nXs)[2]};
        if (clrPart[0] != clrCur[0] || clrPart[1] != clrCur[1] ||
            clrPart[2] != clrCur[2]) {
          ptPixList.emplace_back(x - nXs, y - nYs);
        }
      }
    }

    SegmentedRegionInfo info;
    info.ptOrigin = ptMin;
    info.nW = nPartW;
    info.nH = nPartH;
    info.clrFrame = CV_RGB(255, 0, 0);
    info.ptPixList = ptPixList;
    segRegionInfoList.push_back(info);
  }  // for(i)

  clrImg = curClrImg;
}
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void CreateDirectory(const std::string& strFolderPath) {
  std::vector<std::string> strFolderList = Split(strFolderPath, "/");

  std::string str;
  for (const auto& strFolder : strFolderList) {
    str += strFolder + "/";
    if (strFolder == "." || strFolder == "..") {
      continue;
    }
    mkdir(str.c_str(), S_IRWXU);
  }
}
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
std::vector<std::string> Split(const std::string& s, const std::string& delim) {
  std::vector<std::string> elems;
  std::clog << s << std::endl;

  std::string::size_type pos = 0;
  while (pos != std::string::npos) {
    std::string::size_type p = s.find(delim, pos);
    if (p == std::string::npos) {
      elems.push_back(s.substr(pos));
      break;
    } else {
      elems.push_back(s.substr(pos, p - pos));
    }
    pos = p + delim.size();
  }

  return elems;
}
std::vector<std::string> Split(const std::string& s, char delim) {
  std::vector<std::string> elems;
  std::string item;
  std::clog << s << std::endl;
  for (char ch : s) {
    if (ch == delim) {
      if (!item.empty()) {
        elems.push_back(item);
      }
      item.clear();
    } else {
      item += ch;
    }
  }
  if (!item.empty()) {
    elems.push_back(item);
  }

  return elems;
}
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void ConvertColorToGray(cv::Mat& img) {
  int nH = img.rows;
  int nW = img.cols;
  for (int y = 0; y < nH; ++y) {
    int idx = y * nW * 3;
    for (int x = 0; x < nW; ++x) {
      img.at<cv::Vec3b>(y, x)[0] =
          (img.at<cv::Vec3b>(y, x)[0] + img.at<cv::Vec3b>(y, x)[1] +
           img.at<cv::Vec3b>(y, x)[2]) /
          3;
      img.at<cv::Vec3b>(y, x)[1] = img.at<cv::Vec3b>(y, x)[0];
      img.at<cv::Vec3b>(y, x)[2] = img.at<cv::Vec3b>(y, x)[0];
      idx += 3;
    }
  }
}
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
unsigned char* ConvertCVMATtoUCHAR(const cv::Mat& img, const int& nH /*=-1*/,
                                   const int& nW /*=-1*/) {
  int h = nH;
  int w = nW;
  if (nH == -1 && nW == -1) {
    h = img.rows;
    w = img.cols;
  }
  auto* pImg = new unsigned char[w * h * 3];
  for (int y = 0; y < h; ++y) {
    int idx = y * w * 3;
    for (int x = 0; x < w; ++x) {
      pImg[idx + 0] = img.at<cv::Vec3b>(y, x)[0];  //=0 or 128;
      pImg[idx + 1] = img.at<cv::Vec3b>(y, x)[1];  //=0 or 128;
      pImg[idx + 2] = img.at<cv::Vec3b>(y, x)[2];  //=0 or 128;
      idx += 3;
    }
  }
  return pImg;
}
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
std::string GetPNGFile(const int& nNum, const std::string& strOutputFolder) {
  using namespace gazosan;
  auto now_time = now();
  std::string strYYYYMMDD = format_date(now_time, TimeFormat::YYYYMMDD),
              strHHMMSS = format_date(now_time, TimeFormat::HHMMSS);

  int nFileNo = nNum;

  bool bIsPartFile = (1 <= nNum && nNum < 5000);
  if (bIsPartFile) {
    nFileNo = g_nPartFileNo;
  }

  std::ostringstream strFileNo;
  strFileNo << nFileNo;

  std::string strTmp;
  if (0 <= nFileNo && nFileNo <= 9) strTmp = "000";
  if (10 <= nFileNo && nFileNo <= 99) strTmp = "00";
  if (100 <= nFileNo && nFileNo <= 999) strTmp = "0";
  std::string strPNGFile = "ImgSeg_" + strYYYYMMDD + "_" + strHHMMSS + "-" +
                           strTmp + strFileNo.str() + ".png";

  switch (nNum) {
    case 8000:
      strPNGFile = g_strFileName + "_delete.png";
      break;
    case 9000:
      strPNGFile = g_strFileName + "_add.png";
      break;
    case 10000:
      strPNGFile = g_strFileName + "_diff.png";
      break;
    default:
      break;
  }

  std::string strPNGFileRelativePath = strOutputFolder + strPNGFile;

  if (bIsPartFile) {
    g_strFileList.push_back(strPNGFileRelativePath);
    ++g_nPartFileNo;
  }

  return strPNGFileRelativePath;
}
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void CreatePNGfromCVMAT(const int& nNum, const cv::Mat& img,
                        const std::string& strOutputFolder) {
  // "8-bit, unsigned char, 3-channels" data
  if (img.type() == CV_8UC3) {
    std::string strPNGFileRelativePath = GetPNGFile(nNum, strOutputFolder);
    cv::imwrite(strPNGFileRelativePath, img);
  } else {
    if (nNum >= 0) {
      cv::Mat clrImg(img.size(), CV_8UC3);
      CreatePNGfromCVMAT(nNum, clrImg, strOutputFolder);
    } else {
      std::string strPNGFileRelativePath = GetPNGFile(nNum, strOutputFolder);
      cv::imwrite(strPNGFileRelativePath, img);
    }
  }
}
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void CreatePNGfromUCHAR(const int& nNum, const int& nW, const int& nH,
                        unsigned char* pImg,
                        const std::string& strOutputFolder) {
  std::string strBMPFile = "ImgSeg_Tmp.bmp";
  std::string strBMPFileRelativePath = strOutputFolder + strBMPFile;
  CreateBMP(strBMPFileRelativePath, nW, nH, pImg);

  fpos_t fsize;
  FILE* pF = fopen(strBMPFileRelativePath.c_str(), "rb");
  if (pF) {
    fseek(pF, 0, SEEK_END);
    fgetpos(pF, &fsize);
    fclose(pF);
#ifdef __linux
    if (fsize.__pos < 1024) /* for LINUX */
#else
    if (fsize < 1024) /* for LINUX */
#endif
    {
      remove(strBMPFileRelativePath.c_str());
      return;
    }
  }

  std::string strPNGFileRelativePath = GetPNGFile(nNum, strOutputFolder);
  ConvertBMPtoPNG(strBMPFileRelativePath, strPNGFileRelativePath);

  remove(strBMPFileRelativePath.c_str());
}
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void ConvertBMPtoPNG(const std::string& strBMPFile,
                     const std::string& strPNGFile) {
  cv::Mat bmpImg = cv::imread(strBMPFile, -1);
  cv::Mat pngImg;
  bmpImg.convertTo(pngImg, CV_8UC3);

  cv::imwrite(strPNGFile, pngImg);
}
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void CreateBMP(const std::string& strFile, const int& nW, const int& nH,
               const unsigned char* pImg) {
  FILE* pF = nullptr;
  // fopen_s(&pF, strFile.c_str(), "wb"); /* for visual studio */
  pF = fopen(strFile.c_str(), "wb"); /* for LINUX */
  if (pF == nullptr) return;

  const int kFileHeaderSize = 14;
  const int kInfoHeaderSize = 40;
  const int kHeaderSize = kFileHeaderSize + kInfoHeaderSize;
  unsigned char HeaderBuffer[kHeaderSize];

  int nRealW = nW * 3 + nW % 4;
  unsigned int nFileSize = nH * nRealW + kHeaderSize;
  unsigned int nOffsetToData = kHeaderSize;
  unsigned long nInfoHeaderSize = kInfoHeaderSize;
  unsigned int nPlanes = 1;
  unsigned int nColor = 24;
  unsigned long nCompress = 0;
  unsigned long nDataSize = nH * nRealW;
  long nXppm = 1;
  long nYppm = 1;

  HeaderBuffer[0] = 'B';
  HeaderBuffer[1] = 'M';
  memcpy(HeaderBuffer + 2, &nFileSize, sizeof(nFileSize));
  HeaderBuffer[6] = 0;
  HeaderBuffer[7] = 0;
  HeaderBuffer[8] = 0;
  HeaderBuffer[9] = 0;
  memcpy(HeaderBuffer + 10, &nOffsetToData, sizeof(nOffsetToData));
  HeaderBuffer[11] = 0;
  HeaderBuffer[12] = 0;
  HeaderBuffer[13] = 0;
  memcpy(HeaderBuffer + 14, &nInfoHeaderSize, sizeof(nInfoHeaderSize));
  HeaderBuffer[15] = 0;
  HeaderBuffer[16] = 0;
  HeaderBuffer[17] = 0;
  memcpy(HeaderBuffer + 18, &nW, sizeof(nW));
  memcpy(HeaderBuffer + 22, &nH, sizeof(nH));
  memcpy(HeaderBuffer + 26, &nPlanes, sizeof(nPlanes));
  memcpy(HeaderBuffer + 28, &nColor, sizeof(nColor));
  memcpy(HeaderBuffer + 30, &nCompress, sizeof(nCompress));
  memcpy(HeaderBuffer + 34, &nDataSize, sizeof(nDataSize));
  memcpy(HeaderBuffer + 38, &nXppm, sizeof(nXppm));
  memcpy(HeaderBuffer + 42, &nYppm, sizeof(nYppm));
  HeaderBuffer[46] = 0;
  HeaderBuffer[47] = 0;
  HeaderBuffer[48] = 0;
  HeaderBuffer[49] = 0;
  HeaderBuffer[50] = 0;
  HeaderBuffer[51] = 0;
  HeaderBuffer[52] = 0;
  HeaderBuffer[53] = 0;

  fwrite(HeaderBuffer, sizeof(unsigned char), kHeaderSize, pF);

  auto* pBMP = new unsigned char[nRealW];
  for (int y = 0; y < nH; ++y) {
    int idx = (nH - 1 - y) * nW * 3;
    for (int x = 0; x < nW; ++x, idx += 3) {
      pBMP[x * 3 + 0] = pImg[idx + 0];
      pBMP[x * 3 + 1] = pImg[idx + 1];
      pBMP[x * 3 + 2] = pImg[idx + 2];
    }
    // for (int x=nW*3; x<nRealW; ++x)
    //{
    //	pBMP[x] = 0;
    //}
    fwrite(pBMP, sizeof(unsigned char), nRealW, pF);
  }

  delete[] pBMP;
  pBMP = nullptr;

  fclose(pF);
}
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
bool GetGroupedDataTest(const int& nSrcW, const int& nSrcH,
                        const unsigned char* pSrcImg,
                        std::vector<std::vector<PixelConnectivity*>*>& solid) {
  {
    using namespace gazosan;
    std::string strHHMMSS = format_date(now(), TimeFormat::HHMMSS);

    std::clog << " -> Grouping Start : " << strHHMMSS << std::endl;
  }
  const int Xs = 0;
  const int Ys = 0;
  const int Xe = Xs + nSrcW;
  const int Ye = Ys + nSrcH;
  auto* pImg = new unsigned char[nSrcW * nSrcH];
  // create data
  for (int y = Ys; y < Ye; ++y) {
    int idxF = y * nSrcW * 3;
    for (int x = Xs; x < Xe; ++x) {
      int idxE = (y - Ys) * nSrcW + (x - Xs);
      pImg[idxE] = pSrcImg[idxF + 0];  //(pSrcImg[idxF+0] + pSrcImg[idxF+1] +
                                       // pSrcImg[idxF+2])/3;
      idxF += 3;
    }
  }
  // get 8-neighborhood data
  for (int y = Ys; y < Ye; ++y) {
    for (int x = Xs; x < Xe; ++x) {
      int idx = (y - Ys) * nSrcW + (x - Xs);
      unsigned char clr = pImg[idx];
      if (clr == 128) {
        auto* pShell = new std::vector<PixelConnectivity*>;
        auto* pPix = new PixelConnectivity;
        pPix->nIdx = -1;
        pShell->push_back(pPix);
        solid.push_back(pShell);
        continue;
      }

      auto* pShell = new std::vector<PixelConnectivity*>;
      auto* pPix = new PixelConnectivity;
      pPix->nIdx = idx;
      pShell->push_back(pPix);
      solid.push_back(pShell);

      for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
          // skip myself
          if (dx == 0 && dy == 0) continue;

          if ((Xs <= x + dx && x + dx < Xe) && (Ys <= y + dy && y + dy < Ye)) {
            int m = (y + dy - Ys) * nSrcW + (x + dx - Xs);
            if (clr == pImg[m]) {
              pPix->nNeighborIdxList.push_back(m);
            }
          }
        }
      }
    }
  }
  // grouping
  while (true) {
    bool bIsConnectedShell = false;
    for (unsigned int idxShell = 0; idxShell < solid.size(); ++idxShell) {
      std::vector<PixelConnectivity*>* pShell = solid.at(idxShell);
      if (pShell == nullptr || pShell->empty()) continue;
      if (pShell->size() == 1 && pShell->at(0)->nIdx == -1) {
        delete pShell;
        pShell = nullptr;
        auto itr = solid.begin() + idxShell;
        *itr = NULL;
        continue;
      }

      unsigned int idx1stShell = 0;
      for (unsigned int idxPix = 0; idxPix < pShell->size(); ++idxPix) {
        PixelConnectivity* pPix = pShell->at(idxPix);
        if (pPix->nIdx == -1) continue;
        for (int idxNeighborShell : pPix->nNeighborIdxList) {
          if (idx1stShell != 0 && idx1stShell == idxNeighborShell) continue;

          std::vector<PixelConnectivity*>* pNeighborShell =
              solid.at(idxNeighborShell);
          if (pNeighborShell == nullptr || pNeighborShell->empty()) continue;

          for (auto pNeighborPix : *pNeighborShell) {
            pShell->push_back(pNeighborPix);
          }
          delete pNeighborShell;
          pNeighborShell = nullptr;
          auto itr = solid.begin() + idxNeighborShell;
          *itr = NULL;
          bIsConnectedShell = true;
        }
        if (idx1stShell == 0) {
          idx1stShell = pPix->nIdx;
        }
      }
    }
    if (!bIsConnectedShell) break;
  }
  // delete null pointer shells in solid
  for (unsigned int i = solid.size() - 1; i < solid.size(); --i) {
    auto itrShell = solid.begin() + i;
    if (*itrShell == NULL) {
      solid.erase(itrShell);
    }
  }
  {
    using namespace gazosan;
    std::string strHHMMSS = format_date(now(), TimeFormat::HHMMSS);
    std::clog << " -> Grouping End : " << strHHMMSS << std::endl;
  }

  std::clog << "*** Part count after grouping : " << solid.size() << std::endl;

  return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
