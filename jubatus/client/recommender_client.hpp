// This file is auto-generated from recommender.idl(1.0.5-6-g051a909) with jenerator version 0.9.4-42-g70f7539/develop
// *** DO NOT EDIT ***

#ifndef JUBATUS_CLIENT_RECOMMENDER_CLIENT_HPP_
#define JUBATUS_CLIENT_RECOMMENDER_CLIENT_HPP_

#include <map>
#include <string>
#include <vector>
#include <jubatus/client/common/client.hpp>
#include <jubatus/client/common/datum.hpp>
#include "recommender_types.hpp"

namespace jubatus {
namespace recommender {
namespace client {

class recommender : public jubatus::client::common::client {
 public:
  recommender(const std::string& host, uint64_t port, const std::string& name,
      unsigned int timeout_sec)
      : client(host, port, name, timeout_sec) {
  }

  bool clear_row(const std::string& id) {
    msgpack::rpc::future f = c_.call("clear_row", name_, id);
    return f.get<bool>();
  }

  bool update_row(const std::string& id,
      const jubatus::client::common::datum& row) {
    msgpack::rpc::future f = c_.call("update_row", name_, id, row);
    return f.get<bool>();
  }

  bool clear() {
    msgpack::rpc::future f = c_.call("clear", name_);
    return f.get<bool>();
  }

  jubatus::client::common::datum complete_row_from_id(const std::string& id) {
    msgpack::rpc::future f = c_.call("complete_row_from_id", name_, id);
    return f.get<jubatus::client::common::datum>();
  }

  jubatus::client::common::datum complete_row_from_datum(
      const jubatus::client::common::datum& row) {
    msgpack::rpc::future f = c_.call("complete_row_from_datum", name_, row);
    return f.get<jubatus::client::common::datum>();
  }

  std::vector<id_with_score> similar_row_from_id(const std::string& id,
      uint32_t size) {
    msgpack::rpc::future f = c_.call("similar_row_from_id", name_, id, size);
    return f.get<std::vector<id_with_score> >();
  }

  std::vector<id_with_score> similar_row_from_id_and_score(
      const std::string& id, double score) {
    msgpack::rpc::future f = c_.call("similar_row_from_id_and_score", name_, id,
        score);
    return f.get<std::vector<id_with_score> >();
  }

  std::vector<id_with_score> similar_row_from_id_and_rate(const std::string& id,
      float rate) {
    msgpack::rpc::future f = c_.call("similar_row_from_id_and_rate", name_, id,
        rate);
    return f.get<std::vector<id_with_score> >();
  }

  std::vector<id_with_score> similar_row_from_datum(
      const jubatus::client::common::datum& row, uint32_t size) {
    msgpack::rpc::future f = c_.call("similar_row_from_datum", name_, row,
        size);
    return f.get<std::vector<id_with_score> >();
  }

  std::vector<id_with_score> similar_row_from_datum_and_score(
      const jubatus::client::common::datum& row, double score) {
    msgpack::rpc::future f = c_.call("similar_row_from_datum_and_score", name_,
        row, score);
    return f.get<std::vector<id_with_score> >();
  }

  std::vector<id_with_score> similar_row_from_datum_and_rate(
      const jubatus::client::common::datum& row, float rate) {
    msgpack::rpc::future f = c_.call("similar_row_from_datum_and_rate", name_,
        row, rate);
    return f.get<std::vector<id_with_score> >();
  }

  jubatus::client::common::datum decode_row(const std::string& id) {
    msgpack::rpc::future f = c_.call("decode_row", name_, id);
    return f.get<jubatus::client::common::datum>();
  }

  std::vector<std::string> get_all_rows() {
    msgpack::rpc::future f = c_.call("get_all_rows", name_);
    return f.get<std::vector<std::string> >();
  }

  double calc_similarity(const jubatus::client::common::datum& lhs,
      const jubatus::client::common::datum& rhs) {
    msgpack::rpc::future f = c_.call("calc_similarity", name_, lhs, rhs);
    return f.get<double>();
  }

  double calc_l2norm(const jubatus::client::common::datum& row) {
    msgpack::rpc::future f = c_.call("calc_l2norm", name_, row);
    return f.get<double>();
  }
};

}  // namespace client
}  // namespace recommender
}  // namespace jubatus

#endif  // JUBATUS_CLIENT_RECOMMENDER_CLIENT_HPP_
