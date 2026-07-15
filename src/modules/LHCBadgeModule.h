#pragma once

#ifdef LHC_BADGE_2025_FULL

#include "Channels.h"
#include "Observer.h"
#include "SinglePortModule.h"
#include "Status.h"
#include "input/InputBroker.h"
#include <string>

class LHCBadgeModule : public SinglePortModule {
public:
  static constexpr const char *CONFIG_CHANNEL_NAME = "LHCBADGECFG";
  static constexpr const char *LHC_CHANNEL_NAME = "LHC";

  LHCBadgeModule();

  static bool isConfigChannel(ChannelIndex channel);
  static bool isLhcChannel(ChannelIndex channel);

  // Handles commands sent by a directly connected client without transmitting
  // them over LoRa.
  bool handleLocalConfigPacket(meshtastic_MeshPacket &packet);

protected:
  ProcessMessage handleReceived(const meshtastic_MeshPacket &packet) override;
  bool wantPacket(const meshtastic_MeshPacket *packet) override;

private:
  CallbackObserver<LHCBadgeModule, const meshtastic::Status *>
      bluetoothStatusObserver =
          CallbackObserver<LHCBadgeModule, const meshtastic::Status *>(
              this, &LHCBadgeModule::onBluetoothStatusUpdate);
  CallbackObserver<LHCBadgeModule, const InputEvent *> inputObserver =
      CallbackObserver<LHCBadgeModule, const InputEvent *>(
          this, &LHCBadgeModule::onInputEvent);

  int onBluetoothStatusUpdate(const meshtastic::Status *status);
  int onInputEvent(const InputEvent *event);
  std::string executeCommand(const std::string &command);
  void sendResponse(const meshtastic_MeshPacket &request,
                    const std::string &response);
};

extern LHCBadgeModule *lhcBadgeModule;

#endif
