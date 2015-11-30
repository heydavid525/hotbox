#pragma once

#include "db/stat.hpp"
#include <vector>

namespace hotbox {

// Return info from stats update. Designed to avoid creating std::vector
// repeatedly.
struct StatUpdateOutput {
  StatUpdateOutput(const std::vector<Feature>& _updated_features,
      const std::vector<int>& _num_unique, int _num_updates) :
    updated_features(_updated_features), num_unique(_num_unique), 
    num_updates(_num_updates) { }

  const std::vector<Feature>& updated_features;
  const std::vector<int>& num_unique;
  const int num_updates;
};

// StatCollector provides interface to update Stat in "atomic" fashion. It
// records update operations between DatumCreateBegin() and DatumCreateEnd(),
// and apply the stats update only when DatumCreateEnd is called. This is
// needed because datum parsing throws exception when a feature is not found
// in the schema, so update to Stat need to be aborted in that case.
// StatCollector works closely with DatumBase.
class StatCollector {
public:
  // Does not take ownership of stats. stats must outlive StatCollector.
  StatCollector(std::vector<Stat>* stats);

  // Return the underlying Stat, which should outlive StatCollector.
  std::vector<Stat>& GetStats();

  // A datum creation process can be aborted if some features aren't found in
  // the schema. StatCollector records all the feature values being added
  // between DatumCreateBegin() and DatumCreateEnd(). When DatumCreateEnd()
  // is called these feature values are `committed' to stats. If
  // DatumCreateEnd() isn't called then the datum in creation must have been
  // aborted and all feature values will be discarded.
  void DatumCreateBegin();

  void AddFeatureStat(const Feature& feature);

  // UpdateStats records updates and apply them in DatumCreateEnd(). See
  // Stat::UpdateStat for update semantic.
  void UpdateStat(const Feature& feature, float val);

  // Commit all pending updates to stats_. Return # of unique values for each
  // updated feature.
  StatUpdateOutput DatumCreateEnd();

private:
  // Set of Stat to update.
  std::vector<Stat>* stats_;

  // # of pending feature values to update stats_.
  int num_pending_updates_;

  // pending_update_features_ and vals_ have the same length >=
  // num_pending_updates_.
  std::vector<Feature> pending_update_features_;
  // feature vals.
  std::vector<float> vals_;

  // num_unique_[i] is the max # of unique values for i-th updated feature
  // across all stats_.
  std::vector<int> num_unique_;
};

}  // namespace hotbox
