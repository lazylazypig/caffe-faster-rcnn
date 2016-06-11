// ------------------------------------------------------------------
// Xuanyi . Refer to Dong Jian
// 2016/03/29
// ------------------------------------------------------------------
#ifndef CAFFE_FRCNN_UTILS_HPP_
#define CAFFE_FRCNN_UTILS_HPP_

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <exception>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <cstdlib>
#include <cassert>
#include <vector>
#include <map>
#include <queue>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/contrib/contrib.hpp>
#include <boost/algorithm/string.hpp>
#include "boost/filesystem.hpp"
#include "caffe/common.hpp"

#include <glog/logging.h>

#include "caffe/FRCNN/util/frcnn_param.hpp"

namespace caffe {

namespace Frcnn {

class DataPrepare {
public:
  DataPrepare() {
    rois.clear();
    ok = false;
  }
  std::string GetImagePath(std::string root = "") {
    CHECK(this->ok) << "illegal status(ok=" << ok << ")";
    return root + image_path;
  }
  int GetImageIndex() {
    CHECK(this->ok) << "illegal status(ok=" << ok << ")";
    return image_index;
  }
  std::vector<std::vector<float> > GetRois(bool include_diff = false) {
    CHECK(this->ok) << "illegal status(ok=" << ok << ")";
    CHECK_EQ(this->rois.size(), this->diff.size());
    std::vector<std::vector<float> > _rois;
    for (size_t index = 0; index < this->rois.size(); index++) {
      if (include_diff == false && this->diff[index] == 1) continue;
      _rois.push_back( this->rois[index] );
    }
    return _rois;
  }
  bool load_WithDiff(std::ifstream &infile) {
    string hashtag;
    if(!(infile >> hashtag)) return ok=false;
    CHECK_EQ(hashtag, "#");
    CHECK(infile >> this->image_index >> this->image_path);
    int num_roi;
    CHECK(infile >> num_roi); 
    rois.clear(); diff.clear();
    for (int index = 0; index < num_roi; index++) {
      int label, x1, y1, x2, y2; 
      int diff_;
      CHECK(infile >> label >> x1 >> y1 >> x2 >> y2 >> diff_); 
      x1 --; y1 --; x2 --; y2 --;
      // CHECK LABEL
      CHECK_GE(label, 1) << "illegal label : " << label << ", should >= 1 " ;
      CHECK_LT(label, FrcnnParam::n_classes) << "illegal label : " << label << ", should < " << FrcnnParam::n_classes << "(n_classes)";
      CHECK_GE(x2, x1) << "illegal coordinate : " << x1 << ", " << x2; 
      CHECK_GE(y2, y1) << "illegal coordinate : " << y1 << ", " << y2;
      vector<float> roi(DataPrepare::NUM);
      roi[DataPrepare::LABEL] = label;
      roi[DataPrepare::X1] = x1;
      roi[DataPrepare::Y1] = y1;
      roi[DataPrepare::X2] = x2;
      roi[DataPrepare::Y2] = y2;
      rois.push_back(roi);
      diff.push_back(diff_);
    }
    return ok=true;
  }
  enum RoiDataField { LABEL, X1, Y1, X2, Y2, NUM };

private:
  vector<vector<float> > rois;
  vector<int> diff;
  string image_path;
  int image_index;
  bool ok;
};

// image and box
template <typename Dtype>
class Point4f {
public:
  Dtype Point[4]; // x1 y1 x2 y2
  Point4f(Dtype x1 = 0, Dtype y1 = 0, Dtype x2 = 0, Dtype y2 = 0) {
    Point[0] = x1; Point[1] = y1;
    Point[2] = x2; Point[3] = y2;
  }
  Point4f(const float data[4]) {
    for (int i=0;i<4;i++) Point[i] = data[i]; 
  }
  Point4f(const double data[4]) {
    for (int i=0;i<4;i++) Point[i] = data[i]; 
  }
  Point4f(const Point4f &other) { memcpy(Point, other.Point, sizeof(Point)); }
  Dtype& operator[](const unsigned int id) { return Point[id]; }
  const Dtype& operator[](const unsigned int id) const { return Point[id]; }

  std::string to_string() const {
    char buff[100];
    snprintf(buff, sizeof(buff), "%.1f %.1f %.1f %.1f", Point[0], Point[1], Point[2], Point[3]);
    return std::string(buff);
  }

};

template <typename Dtype>
class BBox : public Point4f<Dtype> {
public:
  Dtype confidence;
  int id;

  BBox(Dtype x1 = 0, Dtype y1 = 0, Dtype x2 = 0, Dtype y2 = 0,
       Dtype confidence = 0, int id = 0)
      : Point4f<Dtype>(x1, y1, x2, y2), confidence(confidence), id(id) {}
  BBox(Point4f<Dtype> box, Dtype confidence = 0, int id = 0)
      : Point4f<Dtype>(box), confidence(confidence), id(id) {}

  BBox &operator=(const BBox &other) {
    memcpy(this->Point, other.Point, sizeof(this->Point));
    confidence = other.confidence;
    id = other.id;
    return *this;
  }

  bool operator<(const class BBox &other) const {
    if (confidence != other.confidence)
      return confidence > other.confidence;
    else
      return id < other.id;
  }

  std::string to_string() const {
    char buff[100];
    snprintf(buff, sizeof(buff), "cls:%3d -- (%.3f): %.2f %.2f %.2f %.2f", id,
             confidence, this->Point[0], this->Point[1], this->Point[2], this->Point[3]);
    return std::string(buff);
  }

  std::string to_short_string() const {
    char buff[100];
    snprintf(buff, sizeof(buff), "cls:%1d -- (%.2f)", id, confidence);
    return std::string(buff);
  }
 };

template <typename Dtype>
Dtype get_iou(const Point4f<Dtype> &A, const Point4f<Dtype> &B);

template <typename Dtype>
std::vector<vector<Dtype> > get_ious(const std::vector<Point4f<Dtype> > &A, const std::vector<Point4f<Dtype> > &B);

template <typename Dtype>
std::vector<Dtype> get_ious(const Point4f<Dtype> &A, const std::vector<Point4f<Dtype> > &B);

float get_scale_factor(int width, int height, int short_size, int max_long_size);

// config
typedef std::map<std::string, std::string> str_map;

str_map parse_json_config(const std::string file_path);

std::string extract_string(std::string target_key,
     str_map& default_map);

float extract_float(std::string target_key, 
    str_map& default_map);

int extract_int(std::string target_key, 
    str_map& default_map);

std::vector<float> extract_vector(std::string target_key,
     str_map& default_map);

// file 
std::vector<std::string> get_file_list (const std::string& path,
    const std::string& ext);

template <typename Dtype>
void print_vector(std::vector<Dtype> data); 

std::string anchor_to_string(std::vector<float> data);

std::string float_to_string(const std::vector<float> data);

std::string float_to_string(const float *data);

} // namespace Frcnn

} // namespace caffe

#endif // CAFFE_FRCNN_UTILS_HPP_
