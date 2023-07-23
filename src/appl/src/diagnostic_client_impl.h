/* Diagnostic Server library
 * Copyright (C) 2023  Avijit Dey
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef DIAG_SERVER_LIB_APPL_SRC_DIAGNOSTIC_CLIENT_IMPL_H_
#define DIAG_SERVER_LIB_APPL_SRC_DIAGNOSTIC_CLIENT_IMPL_H_

#include <memory>
#include <string_view>

#include "common/diagnostic_manager.h"
#include "common_header.h"
#include "dcm/dcm_client.h"
#include "include/diagnostic_client.h"
#include "include/diagnostic_client_conversation.h"

namespace diag {
namespace server {
class DiagClientImpl final : public diag::server::DiagClient {
public:
  // ctor
  explicit DiagClientImpl(std::string_view dm_client_config);

  // dtor
  ~DiagClientImpl() override = default;

  // Initialize
  void Initialize() override;

  // De-Initialize
  void DeInitialize() override;

  // Get Required Conversation based on Conversation Name
  diag::server::conversation::DiagClientConversation& GetDiagnosticClientConversation(
      std::string_view conversation_name) override;

  // Send Vehicle Identification Request and get response
  std::pair<diag::server::DiagClient::VehicleResponseResult,
            diag::server::vehicle_info::VehicleInfoMessageResponseUniquePtr>
  SendVehicleIdentificationRequest(
      diag::server::vehicle_info::VehicleInfoListRequestType vehicle_info_request) override;

private:
  // dcm server instance
  std::unique_ptr<diag::server::common::DiagnosticManager> dcm_instance_ptr;

  // thread to hold dcm server instance
  std::thread dcm_thread_;
};
}  // namespace server
}  // namespace diag
#endif  // DIAG_SERVER_LIB_APPL_SRC_DIAGNOSTIC_SERVER_IMPL_H_
