// Copyright (c) 2019 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "nav2_lifecycle_manager/lifecycle_manager_client.hpp"

#include <cmath>
#include <memory>
#include <string>
#include <utility>

#include "tf2_geometry_msgs/tf2_geometry_msgs.h"
#include "nav2_util/geometry_utils.hpp"

namespace nav2_lifecycle_manager
{
using nav2_util::geometry_utils::orientationAroundZAxis;

LifecycleManagerClient::LifecycleManagerClient(
  const std::string & name,
  std::string namespace_)
{
  manage_service_name_ = name + std::string("/manage_nodes");
  active_service_name_ = name + std::string("/is_active");

  // Create the node to use for all of the service clients
  node_ = std::make_shared<rclcpp::Node>(name + "_service_client", namespace_);

  // Create the service clients
  manager_client_ = node_->create_client<ManageLifecycleNodes>(manage_service_name_);
  is_active_client_ = node_->create_client<std_srvs::srv::Trigger>(active_service_name_);
}

bool
LifecycleManagerClient::startup(const std::chrono::nanoseconds timeout)
{
  return callService(ManageLifecycleNodes::Request::STARTUP, timeout);
}

bool
LifecycleManagerClient::shutdown(const std::chrono::nanoseconds timeout)
{
  return callService(ManageLifecycleNodes::Request::SHUTDOWN, timeout);
}

bool
LifecycleManagerClient::pause(const std::chrono::nanoseconds timeout)
{
  return callService(ManageLifecycleNodes::Request::PAUSE, timeout);
}

bool
LifecycleManagerClient::resume(const std::chrono::nanoseconds timeout)
{
  return callService(ManageLifecycleNodes::Request::RESUME, timeout);
}

bool
LifecycleManagerClient::reset(const std::chrono::nanoseconds timeout)
{
  return callService(ManageLifecycleNodes::Request::RESET, timeout);
}

SystemStatus
LifecycleManagerClient::is_active(const std::chrono::nanoseconds timeout)
{
  auto request = std::make_shared<std_srvs::srv::Trigger::Request>();

  RCLCPP_INFO(
    node_->get_logger(), "Waiting for the %s service...",
    active_service_name_.c_str());

  if (!is_active_client_->wait_for_service(timeout)) {
    return SystemStatus::TIMEOUT;
  }

  RCLCPP_INFO(
    node_->get_logger(), "Sending %s request",
    active_service_name_.c_str());
  auto future_result = is_active_client_->async_send_request(request);

  if (rclcpp::spin_until_future_complete(node_, future_result, timeout) !=
    rclcpp::FutureReturnCode::SUCCESS)
  {
    return SystemStatus::TIMEOUT;
  }

  if (future_result.get()->success) {
    return SystemStatus::ACTIVE;
  } else {
    return SystemStatus::INACTIVE;
  }
}

bool
LifecycleManagerClient::callService(uint8_t command, const std::chrono::nanoseconds timeout)
{
  auto request = std::make_shared<ManageLifecycleNodes::Request>();
  request->command = command;

  RCLCPP_INFO(
    node_->get_logger(), "Waiting for the %s service...",
    manage_service_name_.c_str());

  while (!manager_client_->wait_for_service(std::chrono::seconds(1))) {
    if (!rclcpp::ok()) {
      RCLCPP_ERROR(node_->get_logger(), "Client interrupted while waiting for service to appear");
      return false;
    }
    RCLCPP_INFO(node_->get_logger(), "Waiting for service to appear...");
  }

  RCLCPP_INFO(
    node_->get_logger(), "Sending %s request",
    manage_service_name_.c_str());
  auto future_result = manager_client_->async_send_request(request);

  if (rclcpp::spin_until_future_complete(node_, future_result, timeout) !=
    rclcpp::FutureReturnCode::SUCCESS)
  {
    return false;
  }

  return future_result.get()->success;
}

}  // namespace nav2_lifecycle_manager
