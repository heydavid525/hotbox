
//Very important !!! No invalid input is allowed here, If read a blank token, put the value 0.0 in the field

#include <glog/logging.h>
#include "parse/csv_parser.hpp"
#include "schema/constants.hpp"
#include <cctype>
#include <utility>
#include <cstdint>
#include <string>
#include <cctype>
#include <utility>
#include <cstdint>
#include <stdlib.h>
#include <iostream>
#include <cstddef>

namespace hotbox {
//specify the parameter for the CSV Parser function
void CSVParser::SetConfig(const ParserConfig& config) {
  // Defaul is to not change feature nor label (feature_one_based_ and
  // label_one_based_ are false).
  if (config.has_csv_config()) {
    const CSVParserConfig&  csv_config = config.csv_config();
    //Oct 31 add three config parameter herer
    
    //label_front is the location of the label bool: 1 label at front of a line, 0 label not at the front of a line
    //label_end is the location of the label bool: 1 label at the end of a l
    
    //has_header is the location of the header line: 0 no header, 1 header at the first line
    
    has_header_ = csv_config.has_header();
    label_front_ = csv_config.label_front();
   // label_end = csv_config.label_end();
  }
    LOG(INFO) << "has_header_: " << has_header_;
    LOG(INFO) << "label_front_: " << label_front_;
   // LOG(INFO) << "label_end: " << lable_end;
}

//, Schema* schema,
//DatumBase* datum

void CSVParser::Parse(const std::string& line,Schema* schema,DatumBase* datum)  const {
  if(has_header_ > 0){
    has_header_--;
    return;
  }
  LOG(INFO) << "parsing: " << line;
  char* ptr = nullptr, *endptr = nullptr;
  
  //Assume the length of every feature string is within 1000
  //do not new char* every time, use a global char*
  std::cout << "line size: " << line.size() << std::endl;
  char* str_val = new char[line.size()+1];
  
  // Read label. same as libsvm. assume first column to be the label
  // this is wrong here, need to change according to label position
  float label;
 
  std::string myline (line);

  if(label_front_){
    label = strtof(line.data(), &endptr);
    //std::cout << "label: " << label << endl;
    //check
    this->SetLabelAndWeight(schema, datum, label);
    ptr = endptr;
    //G(INFO) << "parsing: " << line;
    //first char after read label is comma, skip it wrong!!!!
    ++ptr;
  }
  //feature at the end of the line
  else{
    std::size_t found = line.find_last_of(",");
    label = strtof(line.substr(found+1).c_str(),NULL);
    LOG(INFO) << "Myline before: " << myline;
    myline = myline.substr(0,found);
    this->SetLabelAndWeight(schema, datum, label);
    myline.insert(myline.begin(),',');
    endptr = &myline.at(0);
    ptr = &myline.at(1);
    LOG(INFO) << "Myline after: " << myline ;
  }
  
  // Where is kDefaultFamily
  const auto& family = schema->GetOrCreateFamily(kDefaultFamily);
  
  std::vector<TypedFeatureFinder> not_found_features;
  
  //escape all blank char until reach first token
  while (std::isspace(*ptr) && ptr - myline.data() < myline.size()) ++ptr;
  
  //feature_id represents the column number from CSV file
  int32_t feature_id = 2;
 // if(!label_front_){
 //   feature_id = 0;
 // }
  char comma;
  int index = 0;
  bool innerComma = false;
  bool nan = false;
  
  //For the next char
  //case 1 ptr is pointing to comma, nan value
  //case 2 ptr is pointing to " ,string with comma inside !!!Only support ,"string content",-->comma must follow double quote
  //case 3 ptr is pointing to other char ,normal string
  LOG(INFO) << "*PTR: " << *ptr ;
  while(*ptr!='\0'){
    if(*ptr == ','){
      //nan value
      str_val[0] =  '\0';
      endptr = ptr+1;
      nan = true;
    }
    else if(*ptr == '\"'){
      innerComma = true;
      //this string has comma inside, read until find end flag(".)
      //case 2
      //wait until the next ", to appear
      char double_quote = *(ptr+1);
      char comma = *(ptr+2);
      
      index = 1;
      
      while((double_quote!='\"' || comma!=',') && comma!='\0'){
        double_quote = *(ptr+index);
        comma = *(ptr+index+1);
        index++;
      }
      
      endptr = ptr+index+1;
      
      strncpy(str_val, ptr+1, index-2);
      str_val[index-2] = ',';
    }
    else{
      //case 3
      //read until the next , to appear
    // if(label_front_){
     LOG(INFO) << "case3 ptr: " << *ptr;
      comma = *ptr;
    // }else{
    //   for(index = 0; comma!=','&&comma!='\0';index++){
    //     comma = *(ptr+index);
    //   }
    // }
     //find next comma 
      for (index = 0; comma!=','&& comma !='\0'; index++) {
        comma = *(ptr+index);
      }
    //  if(*ptr==','){
    //    ptr+=index;
    //  }
      
      endptr = ptr+index;
      strncpy(str_val, ptr, index-1);
      str_val[index] = ',';
      std::string str(str_val,0,index-1);
      str.erase(str.find_last_not_of(" \n\r\t")+1);
      
      str.erase(0,str.find_first_not_of("0 \n\r\t"));
      
      if(str.length()==0){
        str.append("0");
      }

      str.append(",");
      const char* tempCharStr = str.c_str();
      int strLength = str.length();
      
      strncpy(str_val,tempCharStr,strLength);
    }
    //detect whether conversion is successful
    char *conversionFlagNumerical;
    char *conversionFlagCategorical;
    
    float val = strtof(str_val, &conversionFlagNumerical);
    long catVal = strtol(str_val, &conversionFlagCategorical,10);
    
    // std::cout << "conversionFlagNumerical: " << *conversionFlagNumerical << '\t';
    // std::cout << "conversionFlagCategorical: " << *conversionFlagCategorical << '\t';
    try {
    
    //if this throws an exception, put the unkonw feature type into not_found_features
      const Feature& feature = family.GetFeature(feature_id);
      LOG(INFO) << "Setting feature: " << feature.global_offset() << " val: " << str_val[0];    
    //need to modify this API (datum->SetFeatureValString) to assign an bytes value to the datum
    

      if(*conversionFlagNumerical==','){
        //feature is numerical or categorical byte type
        if(*conversionFlagCategorical==','){
        
          datum->SetFeatureVal(feature,catVal);
          std::cout << "Categorical: " << catVal << '\t';
        }else{
          //feature is categorical
          datum->SetFeatureVal(feature,val);
          std::cout << "Numerical: " << val << '\t';
        }
      }
      else{
        //feature is string
      //std::cout << "String: ";
        int length;
        if(innerComma){
          length = index - 2;
          datum->SetFeatureValString(feature,str_val,length);
          for(int i =0;i < index-2;i++){
            std::cout << str_val[i];
          }
        }else if(nan){
          //Oct 20
          //setValue(0); set numerical 0 to nan value
          datum->SetFeatureVal(feature,0.0);
          //std::cout << "Not A Value. ";
        }else{
          length = index - 1;
          datum->SetFeatureValString(feature,str_val,length);
          for(int i =0;i < index-1;i++){
            std::cout << str_val[i];
          }

        }
        //std::cout << std::endl;
      }
    }
    catch (const FeatureNotFoundException& e) {
      LOG(INFO) << "not found feature";

      TypedFeatureFinder typed_finder(e.GetNotFoundFeature(),
     this->InferType(val));
     not_found_features.push_back(typed_finder);
    }
    
    //exit condition
    if(comma == '\0'){
      break;
    }
    
    ptr = endptr;
    innerComma = false;
    nan = false;
    feature_id++;
  }
  if (not_found_features.size() > 0) {
    TypedFeaturesNotFoundException e;
    e.SetNotFoundTypedFeatures(std::move(not_found_features));
    throw e;
  }
}

}
