// Concord
//
// Copyright (c) 2021 VMware, Inc. All Rights Reserved.
//
// This product is licensed to you under the Apache 2.0 license (the "License").
// You may not use this product except in compliance with the Apache 2.0
// License.
//
// This product may include a number of subcomponents with separate copyright
// notices and license terms. Your use of these subcomponents is subject to the
// terms and conditions of the subcomponent's license, as noted in the
// LICENSE file.

#include "gtest/gtest.h"

#include "SourceSelector.hpp"
#include "Logger.hpp"

namespace {

using bftEngine::bcst::impl::SourceSelector;
using bftEngine::bcst::impl::NO_REPLICA;

constexpr auto kRetransmissionTimeoutMs = 100;
constexpr auto kReplicaReplacementTimeoutMs = 300;
constexpr auto maxFetchRetransmissions = 2;

constexpr uint64_t kSampleCurrentTimeMs = 1000;
constexpr auto kSmallDeltaTime = kSampleCurrentTimeMs + 1;
constexpr auto kBigDeltaTime = 10 * kSampleCurrentTimeMs;
constexpr uint16_t kSampleReplicaId = 1;

const auto replicas = std::set<uint16_t>{1, 2, 3};

class SourceSelectorTestFixture : public ::testing::Test {
 public:
  SourceSelectorTestFixture()
      : source_selector(
            replicas, kRetransmissionTimeoutMs, kReplicaReplacementTimeoutMs, maxFetchRetransmissions, GL){};

 protected:
  SourceSelector source_selector;
};

TEST_F(SourceSelectorTestFixture, on_construction_there_is_no_current_replica) {
  ASSERT_EQ(source_selector.currentReplica(), NO_REPLICA);
}

TEST_F(SourceSelectorTestFixture, has_no_source_on_construction) { ASSERT_FALSE(source_selector.hasSource()); }

TEST_F(SourceSelectorTestFixture, has_source_after_selection) {
  source_selector.updateSource(kSampleCurrentTimeMs);
  ASSERT_TRUE(source_selector.hasSource());
}

TEST_F(SourceSelectorTestFixture, zero_preferred_replica_on_construction) {
  ASSERT_EQ(source_selector.numberOfPreferredReplicas(), 0);
}

TEST_F(SourceSelectorTestFixture, no_replica_is_preferred_on_construction) {
  for (const auto& replica : replicas) {
    ASSERT_FALSE(source_selector.isPreferred(replica));
  }
}

TEST_F(SourceSelectorTestFixture, should_change_source_after_construction) {
  // Make sure that the current replica is "NO_REPLICA".
  // When there is no current replica the selector should select an initial replica as source.
  ASSERT_EQ(source_selector.currentReplica(), NO_REPLICA);
  ASSERT_TRUE(source_selector.shouldReplaceSource(kSampleCurrentTimeMs, false));
}

TEST_F(SourceSelectorTestFixture, should_change_source_when_bad_data_is_received_from_the_current_source) {
  // Select a source
  source_selector.updateSource(kSampleCurrentTimeMs);
  // Ensuring that the change of the selected source is not caused by a timeout is
  // done by intentionally passing a value than is not much bigger than the one stated as current time during the
  // selection process.
  ASSERT_TRUE(source_selector.shouldReplaceSource(kSmallDeltaTime, true));
}

TEST_F(SourceSelectorTestFixture, should_change_source_when_the_replacement_time_has_elapsed) {
  source_selector.updateSource(kSampleCurrentTimeMs);
  ASSERT_TRUE(source_selector.shouldReplaceSource(kBigDeltaTime, false));
}

TEST_F(SourceSelectorTestFixture, source_should_not_be_changed_when_working_and_replacement_time_has_not_elapsed_yet) {
  source_selector.updateSource(kSampleCurrentTimeMs);
  ASSERT_FALSE(source_selector.shouldReplaceSource(kSmallDeltaTime, false));
}

// Remove current replica
TEST_F(SourceSelectorTestFixture, remove_current_replica_sets_the_current_replica_to_no_replica) {
  ASSERT_EQ(source_selector.currentReplica(), NO_REPLICA);
  source_selector.updateSource(kSampleCurrentTimeMs);
  ASSERT_NE(source_selector.currentReplica(), NO_REPLICA);
  source_selector.removeCurrentReplica();
  ASSERT_EQ(source_selector.currentReplica(), NO_REPLICA);
}

TEST_F(SourceSelectorTestFixture, removing_the_current_replica_makes_it_non_preferred_as_well) {
  source_selector.updateSource(kSampleCurrentTimeMs);
  const auto current_replica = source_selector.currentReplica();
  ASSERT_TRUE(source_selector.isPreferred(current_replica));
  source_selector.removeCurrentReplica();
  ASSERT_FALSE(source_selector.isPreferred(current_replica));
}

// Reset tests
TEST_F(SourceSelectorTestFixture, on_construction_is_reset_reports_true) { ASSERT_TRUE(source_selector.isReset()); }

TEST_F(SourceSelectorTestFixture, leave_initial_state_when_there_are_any_preferred_replicas) {
  ASSERT_TRUE(source_selector.isReset());
  source_selector.setAllReplicasAsPreferred();
  ASSERT_FALSE(source_selector.isReset());
}

TEST_F(SourceSelectorTestFixture, leave_initial_state_when_the_source_selection_time_is_not_zero) {
  ASSERT_TRUE(source_selector.isReset());
  source_selector.setSourceSelectionTime(kSampleCurrentTimeMs);
  ASSERT_FALSE(source_selector.isReset());
}

TEST_F(SourceSelectorTestFixture, leave_initial_state_when_the_fetching_timestamp_is_not_zero) {
  ASSERT_TRUE(source_selector.isReset());
  source_selector.setFetchingTimeStamp(kSampleCurrentTimeMs, true);
  ASSERT_FALSE(source_selector.isReset());
}

TEST_F(SourceSelectorTestFixture, reset_removes_preferred_replicas) {
  ASSERT_EQ(source_selector.numberOfPreferredReplicas(), 0);
  source_selector.setAllReplicasAsPreferred();
  ASSERT_EQ(source_selector.numberOfPreferredReplicas(), replicas.size());
  source_selector.reset();
  ASSERT_EQ(source_selector.numberOfPreferredReplicas(), 0);
}

TEST_F(SourceSelectorTestFixture, reset_removes_current_replica) {
  ASSERT_EQ(source_selector.currentReplica(), NO_REPLICA);
  ASSERT_TRUE(source_selector.isReset());
  source_selector.updateSource(kSampleCurrentTimeMs);
  // Set the selection and fetching times to zero in order to ensure that the reset function's
  // result is solely based on the current replica.
  source_selector.setSourceSelectionTime(0);
  source_selector.setFetchingTimeStamp(0, true);
  ASSERT_FALSE(source_selector.isReset());
  source_selector.reset();
  ASSERT_TRUE(source_selector.isReset());
  ASSERT_EQ(source_selector.currentReplica(), NO_REPLICA);
}

TEST_F(SourceSelectorTestFixture, reset_sets_selection_time_to_zero) {
  ASSERT_TRUE(source_selector.isReset());
  source_selector.setSourceSelectionTime(kSampleCurrentTimeMs);
  ASSERT_FALSE(source_selector.isReset());
  source_selector.reset();
  ASSERT_TRUE(source_selector.isReset());
}

TEST_F(SourceSelectorTestFixture, reset_sets_the_fetching_timestamp_to_zero) {
  ASSERT_TRUE(source_selector.isReset());
  source_selector.setFetchingTimeStamp(kSampleCurrentTimeMs, true);
  ASSERT_FALSE(source_selector.isReset());
  source_selector.reset();
  ASSERT_TRUE(source_selector.isReset());
}

// Others
TEST_F(SourceSelectorTestFixture, set_all_replicas_as_preferred) {
  ASSERT_FALSE(source_selector.hasPreferredReplicas());
  source_selector.setAllReplicasAsPreferred();
  ASSERT_TRUE(source_selector.hasPreferredReplicas());

  for (const auto& replica : replicas) {
    ASSERT_TRUE(source_selector.isPreferred(replica));
  }
}

TEST_F(SourceSelectorTestFixture, select_an_unique_replica_on_each_selection) {
  std::set<uint16_t> selected_replicas;

  // Select an initial replica
  source_selector.updateSource(kSampleCurrentTimeMs);
  ASSERT_TRUE(selected_replicas.insert(source_selector.currentReplica()).second);

  // Loop until the only preferred replica left is the one that has been selected initially
  while (source_selector.numberOfPreferredReplicas() > 1) {
    source_selector.updateSource(kSampleCurrentTimeMs);
    ASSERT_TRUE(selected_replicas.insert(source_selector.currentReplica()).second);
  }

  ASSERT_EQ(replicas, selected_replicas);
}

// Operating with an empty set of replicas
TEST_F(SourceSelectorTestFixture, cannot_select_source_without_initial_replicas) {
  auto source_selector =
      SourceSelector({}, kRetransmissionTimeoutMs, kReplicaReplacementTimeoutMs, maxFetchRetransmissions, GL);
  ASSERT_DEATH(source_selector.updateSource(kSampleCurrentTimeMs), "");
}

TEST_F(SourceSelectorTestFixture, add_preferred_replica) {
  auto source_selector =
      SourceSelector({}, kRetransmissionTimeoutMs, kReplicaReplacementTimeoutMs, maxFetchRetransmissions, GL);
  source_selector.addPreferredReplica(kSampleReplicaId);
  ASSERT_TRUE(source_selector.isPreferred(kSampleReplicaId));
}

TEST_F(SourceSelectorTestFixture, select_the_only_preferred_replica) {
  auto source_selector =
      SourceSelector({}, kRetransmissionTimeoutMs, kReplicaReplacementTimeoutMs, maxFetchRetransmissions, GL);
  source_selector.addPreferredReplica(kSampleReplicaId);
  ASSERT_TRUE(source_selector.isPreferred(kSampleReplicaId));
  source_selector.updateSource(kSampleCurrentTimeMs);
  ASSERT_EQ(source_selector.currentReplica(), kSampleReplicaId);
}

TEST_F(SourceSelectorTestFixture, select_amongst_preferred_replicas) {
  auto source_selector =
      SourceSelector({}, kRetransmissionTimeoutMs, kReplicaReplacementTimeoutMs, maxFetchRetransmissions, GL);

  for (const auto& r : replicas) {
    source_selector.addPreferredReplica(r);
  }

  source_selector.updateSource(kSampleCurrentTimeMs);
  ASSERT_TRUE(source_selector.isPreferred(source_selector.currentReplica()));
}

TEST_F(SourceSelectorTestFixture, change_source_after_too_many_retransmissions) {
  auto source_selector =
      SourceSelector({}, kRetransmissionTimeoutMs, kReplicaReplacementTimeoutMs, maxFetchRetransmissions, GL);

  for (const auto& r : replicas) {
    source_selector.addPreferredReplica(r);
  }
  auto curTimeMs = kSampleCurrentTimeMs;
  // should return false - source not selected
  ASSERT_FALSE(source_selector.retransmissionTimeoutExpired(curTimeMs));
  source_selector.updateSource(kSampleCurrentTimeMs);

  // should return false - fetch time never set
  ASSERT_FALSE(source_selector.retransmissionTimeoutExpired(curTimeMs));

  // operation: send fetch msg and than advance 10ms
  // validate: source should not yet be replaced and retransmission not expired
  source_selector.setFetchingTimeStamp(curTimeMs, true);
  curTimeMs += 10;
  ASSERT_FALSE(source_selector.retransmissionTimeoutExpired(curTimeMs));
  ASSERT_FALSE(source_selector.shouldReplaceSource(curTimeMs, false));

  // operation: send fetch msg and than advance kRetransmissionTimeoutMs + 10 ms
  // validate: source should not yet be replaced and retransmission expired (counter = 1)
  source_selector.setFetchingTimeStamp(curTimeMs, true);
  curTimeMs += kRetransmissionTimeoutMs + 10;
  ASSERT_TRUE(source_selector.retransmissionTimeoutExpired(curTimeMs));
  ASSERT_FALSE(source_selector.shouldReplaceSource(curTimeMs, false));

  // operation: send fetch msg and than advance kRetransmissionTimeoutMs + 10 ms
  // validate: source should be replaced and retransmission expired (counter = 2 and >= maxFetchRetransmissions)
  source_selector.setFetchingTimeStamp(curTimeMs, true);
  curTimeMs += kRetransmissionTimeoutMs + 10;
  ASSERT_TRUE(source_selector.retransmissionTimeoutExpired(curTimeMs));
  ASSERT_TRUE(source_selector.shouldReplaceSource(curTimeMs, false));

  // operation: update source, send fetch msg and than advance 10ms
  // validate: source should not yet be replaced and retransmission not expired
  source_selector.updateSource(curTimeMs);
  source_selector.setFetchingTimeStamp(curTimeMs, true);
  curTimeMs += 10;
  ASSERT_FALSE(source_selector.retransmissionTimeoutExpired(curTimeMs));
  ASSERT_FALSE(source_selector.shouldReplaceSource(curTimeMs, false));
}

}  // namespace

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}