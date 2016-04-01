#include <glog/logging.h>
#include "parse/parser_if.hpp"
#include "schema/schema_util.hpp"
#include "schema/constants.hpp"

namespace hotbox {

namespace {

const int kMaxParseTries = 2;

}  // anonymous namespace

ParserIf::~ParserIf() { }

// Parse and add features to schema if not found.
DatumBase ParserIf::ParseAndUpdateSchema(const std::string& line,
    Schema* schema, StatCollector* stat_collector, bool* invalid) noexcept {
  for (int i = 0; i < kMaxParseTries; ++i) {
    DatumProto* proto = CreateDatumProtoFromOffset(schema->GetAppendOffset());
    stat_collector->DatumCreateBegin();
    DatumBase datum(proto, stat_collector);
    // By default a datum is valid. invalid is set to true by Parse if a line
    // is a comment.
    std::vector<TypedFeatureFinder> not_found_features = Parse(line, schema, &datum,
        invalid);
    if (not_found_features.size() > 0) {
      // Add the missing features to schema.
      for (const TypedFeatureFinder& finder : not_found_features) {
        // Default to sparse store type.
        FeatureStoreType store_type;
        switch (finder.type) {
          case FeatureType::CATEGORICAL:
            store_type = FeatureStoreType::SPARSE_CAT;
            break;
          case FeatureType::NUMERICAL:
            store_type = FeatureStoreType::SPARSE_NUM;
            break;
          case FeatureType::BYTES:
            store_type = FeatureStoreType::SPARSE_BYTES;
            break;
          default:
            LOG(FATAL) << "Unrecognized FeatureType: " << finder.type;
        }
        Feature feature = CreateFeature(store_type);
        schema->AddFeature(finder.family_name, &feature, finder.family_idx);
        LOG(INFO) << "feature global offset: " << feature.global_offset();
        stat_collector->AddFeatureStat(feature);
      }
    } else {
      // No missing feature in schema.
      //auto stats_output = stat_collector->DatumCreateEnd();
      stat_collector->DatumCreateEnd();
      // Comment(wdai): Don't worry about detecting factor feature for now.
      /*
      for (int j = 0; j < stats_output.num_updates; ++j) {
        // Convert features with too many unique values to non-factor feature.
        if (stats_output.num_unique[j] >=
            schema->GetConfig().num_unique_values_factor()) {
          const auto& f = stats_output.updated_features[j];
          // Set this feature to factor in schema.
          Feature& schema_f = schema->GetMutableFeature(f);
          schema_f.set_is_factor(false);

          // Clear out unique_cat_values() in all feature stat.
          auto& stats = stat_collector->GetStats();
          for (auto& stat : stats) {
            stat.GetMutableFeatureStat(f).clear_unique_cat_values();
          }
        }
      }
      */
      return datum;
    }
  }
  LOG(FATAL) << "Attempted to parse " << kMaxParseTries << ". Report bug";
  return DatumBase();
}

// Infer float or int.
FeatureType ParserIf::InferType(float val) {
  return std::ceil(val) == val ? FeatureType::CATEGORICAL :
    FeatureType::NUMERICAL;
}

void ParserIf::SetLabelAndWeight(Schema* schema, DatumBase* datum,
    float label, float weight) {
  const auto& intern_family = schema->GetFamily(kInternalFamily);
  //const Feature& feature = intern_family.GetFeature(kLabelFamilyIdx);
  auto ret = intern_family.GetFeatureNoExcept(kLabelFamilyIdx);
  CHECK(ret.second);
  datum->SetFeatureVal(ret.first, label);
  if (weight != 1.) {
    auto ret = intern_family.GetFeatureNoExcept(kWeightFamilyIdx);
    CHECK(ret.second);
    datum->SetFeatureVal(ret.first, weight);
  }
}

DatumProto* ParserIf::CreateDatumProtoFromOffset(
    const DatumProtoStoreOffset& offset) {
  DatumProto* proto = new DatumProto;
  proto->mutable_dense_cat_store()->Resize(offset.offsets(
        FeatureStoreType::DENSE_CAT), 0);
  proto->mutable_dense_num_store()->Resize(offset.offsets(
        FeatureStoreType::DENSE_NUM), 0.);
  // TODO(wdai): Figure out how to deal with dense_bytes_store.
  //proto->mutable_dense_bytes_store()->Reserve(offset.offsets(
  //      FeatureStoreType::DENSE_BYTES));
  return proto;
}

}   // namespace hotbox
