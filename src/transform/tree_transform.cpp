
#include <glog/logging.h>
#include <string>
#include "transform/transform_api.hpp"
#include "transform/tree_transform.hpp"
#include <sstream>
#include <stdio.h>
#include <fstream>
#include <string>
#include <iostream>
#include <vector>
#include <algorithm>
#include "util/util.hpp"

namespace hotbox {

struct Node {
  int id;
  bool isRoot = false;
  bool isLeaf = false;
  std::string feature_name;
  double split;
  std::string opr;
  int yes_node_idx;
  int no_node_idx;
  int miss_node_idx;
  int level;
};


Node processNodeString(std::string line) {
  size_t idx_last_tab = line.rfind('\t');
  size_t idx_colon = line.find(':');
  int idsize;
  
  Node myNode;
  
  if (idx_last_tab == -1) {
    myNode.isRoot = true;
    idsize = idx_colon;
    myNode.id = stod(line.substr(0,idsize));

  } else {
    idsize = idx_colon - idx_last_tab-1;
    myNode.id = stod(line.substr(idx_last_tab+1,idsize));
  }
  
  myNode.level = idx_last_tab + 1;
  size_t idx_leaf = line.find("leaf");
  if (idx_leaf == -1) {
    myNode.isLeaf = false;
    size_t idx_f = line.find("f");
    size_t idx_operator = line.find("<") == -1 ? 
          ((line.find(">") == -1 ? line.find("="):line.find(">"))):line.find("<");
    size_t count = idx_operator - idx_f;
    myNode.feature_name = line.substr(idx_f,count);
    myNode.opr = line.substr(idx_operator,1);
    size_t idx_right_bracket = line.find("]");
    size_t lenth = idx_right_bracket - idx_operator;
    myNode.split = std:: stof(line.substr(idx_operator+1,lenth));
    
    size_t idx_yes = line.find("yes");
    size_t idx_no = line.find("no");
    size_t idx_mis = line.find("missing");
    int yeslength = idx_no-1-(idx_yes+4);
    int nolength = idx_mis-1-(idx_no+3);
    int mislength = line.size() - (idx_mis+8);
    myNode.yes_node_idx =  std::stod(line.substr(idx_yes+4,yeslength));
    myNode.no_node_idx = std:: stod(line.substr(idx_no+3,nolength));
    myNode.miss_node_idx = std:: stod(line.substr(idx_mis+8,mislength));
    
  } else {
    myNode.isLeaf = true;
    myNode.feature_name = "leaf";
    myNode.opr = "=";
    myNode.split = std:: stof(line.substr(idx_leaf+5,line.size()-(idx_leaf+5)));
  }
  return myNode;
}


// store dump file
// each vector of nodes represent one tree in dump file
// there may be multiple tree so put all trees into a vector
std::vector<std::vector<Node>> GenerateTransform(std::string filename) {
  std::ifstream inFile;
 // inFile.open(filename.c_str());
  inFile.open(filename);
  
  if (inFile.fail()) {
    std:: cout << "Not able to open" << std:: endl;
    exit(1);
  }
  std::string line;
  int lineNum = 0;
  std:: string tree_line;
  std:: vector<std::string> tempVec;
  std:: vector<std::vector<Node>> TreeVec;
  
  // first line
  getline(inFile,line);
  
  while (getline(inFile,line)) {
    if (line.substr(0,7) == "booster") {
      std:: cout << int(tempVec.size()) << std:: endl;
      // process the tree vector here
      std::vector<Node> tempNodeVec(tempVec.size());
      
      for (int i = 0 ; i < tempVec.size(); i++) {
        Node tempNode = processNodeString(tempVec.at(i));
        
        tempNodeVec[tempNode.id] = tempNode;
      }
      // put the Nodes in the tempNodeVec into an array of Node, leaf node
      
      tempVec.clear();
      TreeVec.push_back(tempNodeVec);
    }
    else {
      tempVec.push_back(line);
    }
    
    lineNum++;
  }
  // process the last tree here
  std:: cout << int(tempVec.size()) << std:: endl;
  
  std::vector<Node> tempNodeVec(tempVec.size());
  for (int i = 0 ; i < tempVec.size(); i++) {
    Node tempNode = processNodeString(tempVec.at(i));
    tempNodeVec[tempNode.id] = tempNode;
  }
  
  tempVec.clear();
  TreeVec.push_back(tempNodeVec);
  
  return TreeVec;
}


// void BucketizeTransform::TransformSchema(const TransformParam& param,
//     TransformWriter* writer) const { 

//   const BucketizeTransformConfig& config =
//     param.GetConfig().bucketize_transform();
  
//   const std::vector<Feature>& input_features = param.GetInputFeatures(); 

//   const std::vector<std::string>& input_features_desc =
//     param.GetInputFeaturesDesc();

//   for (int i = 0; i < input_features.size(); ++i) {
//     const auto& input_feature = input_features[i];
//     CHECK(IsNumber(input_feature));
//     const auto& buckets = config.buckets();
//     int num_buckets = config.buckets_size() - 1;
//     for (int j = 0; j < num_buckets; ++j) {
//       std::string range_str = "[" + ToString(buckets.Get(j)) + "," +
//         ToString(buckets.Get(j+1)) + ")";
//       auto feature_name = input_features_desc[i] + range_str;
//       writer->AddFeature(feature_name);
//     }
//   }
// }

void TreeTransform::TransformSchema(const TransformParam& param,
    TransformWriter* writer) const {

  const TreeTransformConfig& config =
    param.GetConfig().tree_transform();
  
  const auto& tree_dump_file_name = config.tree_dump_file_name();

  const std::vector<Feature>& input_features = param.GetInputFeatures();

  std::vector<std::vector<Node>> tree_parse_output  = GenerateTransform(tree_dump_file_name);

  for (int i = 0; i < tree_parse_output.size(); ++i) {
    for (int j = 0; j < tree_parse_output.at(i).size(); ++j) {
      std::string feature_name = tree_parse_output.at(i).at(j).feature_name;
      writer->AddFeature(feature_name);
    }
  }
}

// std::function<void(TransDatum*)> BucketizeTransform::GenerateTransform(
//     const TransformParam& param) const {
//   const BucketizeTransformConfig& config =
//     param.GetConfig().bucketize_transform();
//   std::vector<std::function<void(TransDatum*)>> transforms;
//   const auto& input_features = param.GetInputFeatures();
//   BigInt offset = 0;
//   for (int i = 0; i < input_features.size(); ++i) {
//     const auto& input_feature = input_features[i];
//     const auto& buckets = config.buckets();
//     transforms.push_back(
//       [input_feature, buckets, offset]
//       (TransDatum* datum) {
//         float val = datum->GetFeatureVal(input_feature);
//         for (int j = 0; j < buckets.size() - 1; ++j) {
//           if (val >= buckets.Get(j) && val < buckets.Get(j+1)) {
//             datum->SetFeatureValRelativeOffset(offset + j, 1);
//             break;
//           }
//         }
//       });
//     // The buckets include both ends of the bucket boundaries.
//     offset += config.buckets_size() - 1;
//   }
//     return [transforms] (TransDatum* datum) {
//       for (const auto& transform : transforms) {
//         transform(datum);
//       }};
//   }

  std::function<void(TransDatum*)> TreeTransform::GenerateTransform(
      const TransformParam& param) const {
    const TreeTransformConfig& config =
      param.GetConfig().tree_transform();
    std::vector<std::function<void(TransDatum*)>> transforms;
    
    const auto& input_features = param.GetInputFeatures();
    BigInt offset = 0;
    for (int i = 0; i < input_features.size(); ++i) {
      const auto& input_feature = input_features[i];
      const auto& buckets = config.buckets();
      transforms.push_back(
        [input_feature, buckets, offset]
        (TransDatum* datum) {
          float val = datum->GetFeatureVal(input_feature);

          // 
          for (int j = 0; j < buckets.size() - 1; ++j) {
            if (val >= buckets.Get(j) && val < buckets.Get(j+1)) {
              datum->SetFeatureValRelativeOffset(offset + j, 1);
              break;
            }
          }
 
        });
      // The buckets include both ends of the bucket boundaries.
      offset += config.buckets_size() - 1;
    }
    return [transforms] (TransDatum* datum) {
      for (const auto& transform : transforms) {
        transform(datum);
      }};
  }

}
