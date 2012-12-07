// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2011,2012 Preferred Infrastructure and Nippon Telegraph and Telephone Corporation.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License version 2.1 as published by the Free Software Foundation.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

#include "linear_mixer.hpp"

#include <glog/logging.h>
#include <pficommon/concurrent/lock.h>
#include <pficommon/lang/bind.h>
#include <pficommon/system/time_util.h>
#include "../../common/exception.hpp"
#include "../../common/membership.hpp"
#include "../../common/mprpc/rpc_client.hpp"
#include "../mixable.hpp"

using namespace std;
using namespace pfi::concurrent;
using jubatus::common::mprpc::byte_buffer;

namespace jubatus {
namespace framework {
namespace mixer {

namespace {
class linear_communication_impl : public linear_communication {
public:
  linear_communication_impl(const common::cshared_ptr<common::lock_service>& zk,
                            const std::string& type, const std::string& name, int timeout_sec);

  size_t update_members();
  pfi::lang::shared_ptr<common::try_lockable> create_lock();
  void get_diff(common::mprpc::rpc_result_object&) const;
  void put_diff(const common::mprpc::byte_buffer&) const;

private:
  common::cshared_ptr<common::lock_service> zk_;
  string type_;
  string name_;
  int timeout_sec_;
  vector<pair<string, int> > servers_;
};

linear_communication_impl::linear_communication_impl(const common::cshared_ptr<common::lock_service>& zk,
                                                     const std::string& type, const std::string& name,
                                                     int timeout_sec)
    : zk_(zk),
      type_(type),
      name_(name),
      timeout_sec_(timeout_sec) {
}

pfi::lang::shared_ptr<common::try_lockable> linear_communication_impl::create_lock() {
  string path;
  common::build_actor_path(path, type_, name_);
  return pfi::lang::shared_ptr<common::try_lockable>(new common::lock_service_mutex(*zk_, path + "/master_lock"));
}

size_t linear_communication_impl::update_members() {
  common::get_all_actors(*zk_, type_, name_, servers_);
  return servers_.size();
}

void linear_communication_impl::get_diff(common::mprpc::rpc_result_object& result) const {
  // TODO: to be replaced to new client with socket connection pooling
  common::mprpc::rpc_mclient client(servers_, timeout_sec_);
#ifndef NDEBUG
  for(size_t i =0; i < servers_.size(); i++) {
    DLOG(INFO) << "get diff from " << servers_[i].first << ":" << servers_[i].second;
  }
#endif
  result = client.call("get_diff", 0);
}

void linear_communication_impl::put_diff(const common::mprpc::byte_buffer& mixed) const {
  // TODO: to be replaced to new client with socket connection pooling
  common::mprpc::rpc_mclient client(servers_, timeout_sec_);
#ifndef NDEBUG
  for(size_t i =0; i < servers_.size(); i++) {
    DLOG(INFO) << "pull diff to " << servers_[i].first << ":" << servers_[i].second;
  }
#endif
  client.call("put_diff", mixed);
}

} // namespace

pfi::lang::shared_ptr<linear_communication>
linear_communication::create(const common::cshared_ptr<common::lock_service>& zk,
                        const std::string& type, const std::string& name,
                        int timeout_sec) {
  return pfi::lang::shared_ptr<linear_communication_impl>(new linear_communication_impl(zk, type, name, timeout_sec));
}

linear_mixer::linear_mixer(pfi::lang::shared_ptr<linear_communication> communication,
                           unsigned int count_threshold, unsigned int tick_threshold)
    : communication_(communication),
      count_threshold_(count_threshold),
      tick_threshold_(tick_threshold),
      counter_(0),
      ticktime_(0),
      mix_count_(0),
      is_running_(false),
      t_(pfi::lang::bind(&linear_mixer::mixer_loop, this)) {}

void linear_mixer::register_api(pfi::network::mprpc::rpc_server& server) {
  server.add<common::mprpc::byte_buffer(int)>
      ("get_diff",
       pfi::lang::bind(&linear_mixer::get_diff, this, pfi::lang::_1));
  server.add<int(common::mprpc::byte_buffer)>
      ("put_diff",
       pfi::lang::bind(&linear_mixer::put_diff, this, pfi::lang::_1));
}

void linear_mixer::set_mixable_holder(pfi::lang::shared_ptr<mixable_holder> m) {
  mixable_holder_ = m;
}

void linear_mixer::start() {
  scoped_lock lk(m_);
  if (!is_running_) {
    is_running_ = true;
    t_.start();
  }
}

void linear_mixer::stop() {
  scoped_lock lk(m_);
  if (is_running_) {
    is_running_ = false;
    t_.join();
  }
}

void linear_mixer::updated() {
  scoped_lock lk(m_);
  unsigned int new_ticktime = time(NULL);
  ++counter_;
  if(counter_ > count_threshold_ ||
     new_ticktime - ticktime_ > tick_threshold_){
    c_.notify(); // FIXME: need sync here?
  }
}

void linear_mixer::get_status(server_base::status_t& status) const {
  scoped_lock lk(m_);
  status["linear_mixer.count"] = pfi::lang::lexical_cast<string>(counter_);
  status["linear_mixer.ticktime"] = pfi::lang::lexical_cast<string>(ticktime_);  // since last mix
}

void linear_mixer::mixer_loop() {
  while (is_running_) {
    pfi::lang::shared_ptr<common::try_lockable> zklock = communication_->create_lock();
    try {
      {
        scoped_lock lk(m_);

        c_.wait(m_, 1);
        unsigned int new_ticktime = time(NULL);
        if (counter_ > count_threshold_ || new_ticktime - ticktime_ > tick_threshold_) {
          if (zklock->try_lock()) {
            LOG(INFO) << "starting mix:";
            counter_ = 0;
            ticktime_ = new_ticktime;
          } else {
            continue;
          }
        } else {
          continue;
        }

      } //unlock
      mix();
      LOG(INFO) << ".... " << mix_count_ << "th mix done.";
    } catch (const jubatus::exception::jubatus_exception& e) {
      LOG(ERROR) << e.diagnostic_information(true);
    }
  }
}

void linear_mixer::mix() {
  using namespace pfi::system::time;

  clock_time start = get_clock_time();
  size_t s = 0;

  size_t servers_size = communication_->update_members();
  if (servers_size == 0) {
    LOG(WARNING) << "no other server. ";
    return;
  } else {
    try {
      diff_model_bundler* bundler = mixable_holder_->get_mix_bundler<diff_model_bundler>();

      common::mprpc::rpc_result_object result;
      communication_->get_diff(result);

      // get first object as a raw data
      byte_buffer mixed = diff_model_bundler::mixed_raw_data(result.response.front().response.a3);
      for (size_t i = 1; i < result.response.size(); ++i) {
        msgpack::object tmp = result.response[i].response.a3;
        bundler->mix(tmp, mixed, mixed);
      }

      communication_->put_diff(mixed);
      // TODO: output log when result has error

      s += mixed.size();
    } catch (const std::exception& e) {
      LOG(WARNING) << e.what() << " : mix failed";
      return;
    }
  }

  clock_time end = get_clock_time();
  LOG(INFO) << "mixed with " << servers_size << " servers in " << (double)(end - start) << " secs, "
            << s << " bytes (serialized data) has been put.";
  mix_count_++;
}

byte_buffer linear_mixer::get_diff(int) {
  scoped_lock lk_read(rlock(mixable_holder_->rw_mutex()));
  scoped_lock lk(m_);

  return mixable_holder_->get_mix_bundler<diff_model_bundler>()->get_diff();
}

int linear_mixer::put_diff(const common::mprpc::byte_buffer& diff) {
  scoped_lock lk_write(wlock(mixable_holder_->rw_mutex()));
  scoped_lock lk(m_);

  mixable_holder_->get_mix_bundler<diff_model_bundler>()->put_diff(diff);

  counter_ = 0;
  ticktime_ = time(NULL);
  return 0;
}

}
}
}
