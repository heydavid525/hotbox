#include <glog/logging.h>
#include <string>
#include "transform/transform_api.hpp"
#include "transform/kmeans_transform.hpp"
#include "transform/transform_util.hpp"
#include <sstream>
#include "util/util.hpp"
#include "schema/flexi_datum.hpp"
#include <fstream>  
#include <string>  
#include <iostream>  
#include <cmath>

namespace hotbox {

void KmeansTransform::TransformSchema(const TransformParam& param,
    TransformWriter* writer) const {
  const KmeansTransformConfig& config =
    param.GetConfig().kmeans_transform();
  BigInt num_cluster_centers = config.num_cluster_centers();
  writer->AddFeatures(num_cluster_centers);
}

std::function<void(TransDatum*)> KmeansTransform::GenerateTransform(
    const TransformParam& param) const {
  const KmeansTransformConfig& config = param.GetConfig().kmeans_transform();
  const std::multimap<std::string, WideFamilySelector>& w_family_selectors
    = param.GetWideFamilySelectors();
  CHECK_EQ(1, w_family_selectors.size());
  WideFamilySelector selector = w_family_selectors.begin()->second;
  std::string model_path = config.model_path();
  BigInt feature_dim = config.feature_dim();
  BigInt num_cluster_centers = config.num_cluster_centers();
  std::ifstream in(model_path);  
  std::string line;
  std::vector<std::vector<float>> cluster_centers;

  if(in){
  	for(int i = 0; i < num_cluster_centers; i++){
	  std::vector<float> center;
	  for(int j = 0; j < feature_dim; j++){
	    double feature;
		in >> feature;
		center.push_back(feature);
	  }
	  cluster_centers.push_back(center); 
  	}
	LOG(INFO) << "kmeans model read finished";
  }  
  else {
    LOG(FATAL) << "model file " << model_path << " not found";
  }  
  
  return [selector, cluster_centers] (TransDatum* datum) {
  	std::vector<float> datum_val = GetDenseVals(*datum, selector);
	double min_distance = KmeansTransform::MAX_VALUE;
	int min_index = 0;
	for(int i = 0; i < cluster_centers.size(); i++){
	  double distance = 0;
	  for(int j = 0; j < datum_val.size(); j++){
		distance += pow(datum_val[j] - cluster_centers[i][j], 2);
	  }
	  if(distance < min_distance){
		min_distance = distance;
		min_index = i;
	  }
	}
	int offset = 0;
	for (int j = 0; j < cluster_centers.size(); ++j) {
	  if(j == min_index)
	    datum->SetFeatureValRelativeOffset(offset, 1);
	  else
	  	datum->SetFeatureValRelativeOffset(offset, 0);
      offset++;
	}	
  };
}

} // namespace hotbox

