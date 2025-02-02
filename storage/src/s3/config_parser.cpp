// Concord
//
// Copyright (c) 2018 VMware, Inc. All Rights Reserved.
//
// This product is licensed to you under the Apache 2.0 license (the "License").
// You may not use this product except in compliance with the Apache 2.0
// License.
//
// This product may include a number of subcomponents with separate copyright
// notices and license terms. Your use of these subcomponents is subject to the
// terms and conditions of the subcomponent's license, as noted in the LICENSE
// file.
//
#include <fstream>
#include <algorithm>
#include <cstring>

#include "s3/config_parser.hpp"

using std::string;

namespace concord::storage::s3 {

concord::storage::s3::StoreConfig ConfigFileParser::parse() {
  if (!parser_.Parse()) throw std::runtime_error("failed to parse" + parser_.getConfigFileName());

  concord::storage::s3::StoreConfig config;
  config.bucketName = get_value<string>("s3-bucket-name");
  config.accessKey = get_value<string>("s3-access-key");
  config.protocol = get_value<string>("s3-protocol");
  config.url = get_value<string>("s3-url");
  config.secretKey = get_value<string>("s3-secret-key");
  // TesterReplica is used for Apollo tests. Each test is executed against new blockchain, so we need brand new
  // bucket for the RO replica. To achieve this we use a hack - set the prefix to a unique value so each RO replica
  // writes in the same bucket but in different directory.
  // So if s3-path-prefix is NOT SET it is initialized to a unique value based on current time.
  config.pathPrefix = get_optional_value<string>(
      "s3-path-prefix", std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
  config.operationTimeout = get_optional_value<std::uint32_t>("s3-operation-timeout", 60000);
  return config;
}
}  // namespace concord::storage::s3
