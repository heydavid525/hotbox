#include <glog/logging.h>
#include "parse/parser_if.hpp"
#include "schema/schema_util.hpp"
#include "schema/constants.hpp"

namespace mldb {

ParserIf::~ParserIf() { }

// Parse and add features to schema if not found.
DatumBase ParserIf::ParseAndUpdateSchema(const std::string& line,
    Schema* schema) noexcept {
    DatumProto* proto =
      CreateDatumProtoFromOffset(schema->GetDatumProtoOffset());
    DatumBase datum(proto);
    bool success = false;
    do {
      try {
        Parse(line, schema, &datum);
        success = true;
      } catch (const TypedFeaturesNotFoundException& e) {
        // Add the missing features to schema.
        const auto& not_found_features = e.GetNotFoundTypedFeatures();
        for (const TypedFeatureFinder& finder : not_found_features) {
          // Default to sparse store type.
          Feature feature = CreateFeature(finder.type,
              FeatureStoreType::SPARSE);
          schema->AddFeature(finder.family_name, finder.family_idx, &feature);
        }
      }
    } while (!success);
    return datum;
}

// Infer float or int.
FeatureType ParserIf::InferType(float val) {
  return std::ceil(val) == val ? FeatureType::CATEGORICAL :
    FeatureType::NUMERICAL;
}

void ParserIf::SetLabelAndWeight(Schema* schema, DatumBase* datum,
    float label, float weight) {
  const auto& intern_family = schema->GetOrCreateFamily(kInternalFamily);
  const Feature& feature = intern_family.GetFeature(kLabelFamilyIdx);
  datum->SetFeatureVal(feature.loc(), label);
  if (weight != 1.) {
    const Feature& feature = intern_family.GetFeature(kWeightFamilyIdx);
    datum->SetFeatureVal(feature.loc(), weight);
  }
}

DatumProto* ParserIf::CreateDatumProtoFromOffset(
    const DatumProtoOffset& offset) {
  DatumProto* proto = new DatumProto;
  proto->mutable_dense_cat_store()->Resize(offset.dense_cat_store(), 0);
  proto->mutable_dense_num_store()->Resize(offset.dense_num_store(), 0.);
  proto->mutable_dense_bytes_store()->Reserve(offset.dense_bytes_store());
  return proto;
}

}   // namespace mldb
