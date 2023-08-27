/* Diagnostic Server library
 * Copyright (C) 2023  Rui Peng
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef DIAGNOSTIC_SERVER_LIB_APPL_SRC_DCM_CONVERSATION_DMCONVERSATION_H
#define DIAGNOSTIC_SERVER_LIB_APPL_SRC_DCM_CONVERSATION_DMCONVERSATION_H
/* includes */
#include <string_view>
#include <queue>

#include "include/diagnostic_server_conversation.h"
#include "src/dcm/conversation/dm_conversation_state_impl.h"
#include "uds_transport/connection.h"
#include "uds_transport/conversion_handler.h"
#include "uds_transport/protocol_types.h"
#include "utility/sync_timer.h"

#include "src/dcm/service/service_base.h"
#include "common/doip_payload_type.h"

namespace diag {
namespace server {



namespace conversation {

using ConversationState = conversation_state_impl::ConversationState;

/*
 @ Class Name        : DmConversation
 @ Class Description : Class to establish connection with Diagnostic Server                           
 */
class DmConversation final : public ::diag::server::conversation::DiagServerConversation {
public:
  using SyncTimer = utility::sync_timer::SyncTimer<std::chrono::steady_clock>;
  using SyncTimerState = SyncTimer::TimerState;

  // ctor
  DmConversation(uint16_t logical_address,
                 ::uds_transport::conversion_manager::ConversionIdentifierType &conversion_identifier);

  // dtor
  ~DmConversation() override;

  // startup
  void Startup() override;

  // shutdown
  void Shutdown() override;

  void Register(uint8_t sid, std::unique_ptr<ServiceBase> service) {
    uds_services_[sid] = std::move(service);
  }

  ConnectResult ListenDiagClientConnect(std::uint16_t target_address, IpAddress host_ip_addr) {
    return ConnectResult::kConnectSuccess;
  }

  DisconnectResult CloseDiagClientConnect() {
    return DisconnectResult::kDisconnectSuccess;
  }
  // Description   : Function to connect to Diagnostic Server
  // @param input  : Nothing
  // @return value : ConnectResult
  // ConnectResult ConnectToDiagServer(std::uint16_t target_address, IpAddress host_ip_addr) override;

  // Description   : Function to disconnect from Diagnostic Server
  // @param input  : Nothing
  // @return value : DisconnectResult
  // DisconnectResult DisconnectFromDiagServer() override;  

  // 
  bool GetClientDiagState() override;

  // Description   : Function to send Diagnostic Request and receive response
  // @param input  : Nothing
  // @return value : DisconnectResult
  std::pair<DiagResult, uds_message::UdsResponseMessagePtr> SendDiagnosticRequest(
      uds_message::UdsRequestMessageConstPtr message) override;

  // Register Connection
  void RegisterConnection(std::shared_ptr<::uds_transport::Connection> connection);

  std::shared_ptr<::uds_transport::ConversionHandler> &GetConversationHandler();

  // Indicate message Diagnostic message reception over TCP to user
  std::pair<::uds_transport::UdsTransportProtocolMgr::IndicationResult, ::uds_transport::UdsMessagePtr> IndicateMessage(
      ::uds_transport::UdsMessage::Address source_addr, ::uds_transport::UdsMessage::Address target_addr,
      ::uds_transport::UdsMessage::TargetAddressType type, ::uds_transport::ChannelID channel_id, std::size_t size,
      ::uds_transport::Priority priority, ::uds_transport::ProtocolKind protocol_kind,
      std::vector<uint8_t> payload_info);

  // Hands over a valid message to conversion
  void HandleMessage(::uds_transport::UdsMessagePtr message);

  // shared pointer to store the conversion handler
  std::shared_ptr<::uds_transport::ConversionHandler> dm_conversion_handler_;

  static DiagServerConversation::DiagResult ConvertResponseType(
      ::uds_transport::UdsTransportProtocolMgr::TransmissionResult result_type);

private:
  // Type for active diagnostic session
  enum class SessionControlType : uint8_t {
    kDefaultSession = 0x01,
    kProgrammingSession = 0x02,
    kExtendedSession = 0x03,
    kSystemSafetySession = 0x04
  };
  // Type for active security level
  enum class SecurityLevelType : uint8_t {
    kLocked = 0x00,
    kUnLocked = 0x01,
  };
  // Type of current activity status
  enum class ActivityStatusType : uint8_t { kActive = 0x00, kInactive = 0x01 };

  // Function to wait for response
  void WaitForResponse(std::function<void()> &&timeout_func, std::function<void()> &&cancel_func, int msec);

  // Function to cancel the synchronous wait
  void WaitCancel();

  // Function to get payload type
  static auto GetDoIPPayloadType(std::vector<uint8_t> payload) noexcept -> uint16_t;

  // Function to get payload length
  static auto GetDoIPPayloadLength(std::vector<uint8_t> payload) noexcept -> uint32_t;

  // Function to create the generic header
  static void CreateDoipGenericHeader(std::vector<uint8_t> &doipHeader, std::uint16_t payload_type,
                                      std::uint32_t payload_len);

  void SendRoutingActivationResponse(const DoipMessage &);

  void SendDiagnosticMessageAckResponse(const DoipMessage &);

  void SendDiagnosticMessageResponse(const DoipMessage &);

  void SendDiagnosticPendingMessageResponse(const DoipMessage &);
private:
  // Conversion activity Status
  ActivityStatusType activity_status_;
  // Dcm session
  SessionControlType active_session_;
  // Dcm Security
  SecurityLevelType active_security_;
  // Reception buffer
  uint32_t rx_buffer_size_;
  // p2 server time
  uint16_t p2_server_max_;
  // p2 star Server time
  uint16_t p2_star_server_max_;
  // logical Source address
  uint16_t source_address_;
  // logical target address
  uint16_t target_address_;
  // Vehicle broadcast address
  std::string broadcast_address;
  // remote Ip Address
  std::string remote_address_;

  // logical address
  uint16_t logical_address_;

  // conversion name
  std::string conversation_name_;
  // queue to hold task
  std::queue<std::function<void(void)>> job_queue_;

  // threading var
  std::thread thread_;

  // flag to terminate the thread
  std::atomic_bool exit_request_;

  // flag th start the thread
  std::atomic_bool running_;

  // conditional variable to block the thread
  std::condition_variable cond_var_;

  // locking critical section
  std::mutex mutex_;

  // Tp connection
  std::shared_ptr<::uds_transport::Connection> connection_ptr_;
  // timer
  SyncTimer sync_timer_;
  // rx buffer to store the uds response
  ::uds_transport::ByteVector payload_rx_buffer;
  // conversation state
  conversation_state_impl::ConversationStateImpl conversation_state_;

  std::unordered_map<uint8_t, std::unique_ptr<ServiceBase> > uds_services_;
  void Service();
};

/*
 @ Class Name        : DmConversationHandler
 @ Class Description : Class to establish connection with Diagnostic Server                           
 */
class DmConversationHandler : public ::uds_transport::ConversionHandler {
public:
  // ctor
  DmConversationHandler(::uds_transport::conversion_manager::ConversionHandlerID handler_id,
                        DmConversation &dm_conversion);

  // dtor
  ~DmConversationHandler() = default;

  // Indicate message Diagnostic message reception over TCP to user
  std::pair<::uds_transport::UdsTransportProtocolMgr::IndicationResult, ::uds_transport::UdsMessagePtr> IndicateMessage(
      ::uds_transport::UdsMessage::Address source_addr, ::uds_transport::UdsMessage::Address target_addr,
      ::uds_transport::UdsMessage::TargetAddressType type, ::uds_transport::ChannelID channel_id, std::size_t size,
      ::uds_transport::Priority priority, ::uds_transport::ProtocolKind protocol_kind,
      std::vector<uint8_t> payload_info) override;

  // Hands over a valid message to conversion
  void HandleMessage(::uds_transport::UdsMessagePtr message) override;

private:
  DmConversation &dm_conversation_;
};
}  // namespace conversation
}  // namespace server
}  // namespace diag
#endif  // DIAGNOSTIC_SERVER_LIB_APPL_SRC_DCM_CONVERSATION_DMCONVERSATION_H
