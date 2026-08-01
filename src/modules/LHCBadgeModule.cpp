#ifdef LHC_BADGE_2025_FULL

#include "LHCBadgeModule.h"
#include "AmbientLightingThread.h"
#include "BluetoothStatus.h"
#include "Channels.h"
#include "MeshService.h"
#include "NodeDB.h"
#include "Router.h"
#include "main.h"
#include "modules/LHCBadgeCommands.h"
#include <algorithm>
#include <cctype>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <vector>

namespace
{
constexpr size_t RESPONSE_CHUNK_SIZE = 190;
constexpr NodeNum RESPONSE_SENDER = 1107566697;

std::string trim(const std::string &value)
{
    const auto first = std::find_if_not(value.begin(), value.end(), [](unsigned char c) { return std::isspace(c); });
    const auto last = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char c) { return std::isspace(c); }).base();
    return first < last ? std::string(first, last) : std::string();
}

bool getArgument(const std::string &command, const char *longName, char alias, std::string &argument)
{
    const size_t longLength = strlen(longName);
    if (command.compare(0, longLength, longName) == 0 &&
        (command.size() == longLength || std::isspace(static_cast<unsigned char>(command[longLength])))) {
        argument = trim(command.substr(longLength));
        return true;
    }
    size_t argumentOffset = 0;
    if (lhc_badge::matchShortCommand(command, alias, &argumentOffset)) {
        argument = trim(command.substr(argumentOffset));
        return true;
    }
    return false;
}

bool parseInteger(const std::string &text, int &value)
{
    if (text.empty()) {
        return false;
    }
    char *end = nullptr;
    const long parsed = strtol(text.c_str(), &end, 10);
    if (end == text.c_str() || *end != '\0' || parsed < INT_MIN || parsed > INT_MAX) {
        return false;
    }
    value = static_cast<int>(parsed);
    return true;
}
} // namespace

LHCBadgeModule *lhcBadgeModule = nullptr;

LHCBadgeModule::LHCBadgeModule() : SinglePortModule("lhc-badge", meshtastic_PortNum_TEXT_MESSAGE_APP)
{
    bluetoothStatusObserver.observe(&bluetoothStatus->onNewStatus);
    if (inputBroker) {
        inputObserver.observe(inputBroker);
    }
}

bool LHCBadgeModule::isConfigChannel(ChannelIndex channel)
{
    return channel < channels.getNumChannels() && strcmp(channels.getByIndex(channel).settings.name, CONFIG_CHANNEL_NAME) == 0;
}

bool LHCBadgeModule::isLhcChannel(ChannelIndex channel)
{
    return channel < channels.getNumChannels() && strcmp(channels.getByIndex(channel).settings.name, LHC_CHANNEL_NAME) == 0;
}

bool LHCBadgeModule::handleLocalConfigPacket(meshtastic_MeshPacket &packet)
{
    if (!isConfigChannel(packet.channel) || packet.which_payload_variant != meshtastic_MeshPacket_decoded_tag ||
        packet.decoded.portnum != meshtastic_PortNum_TEXT_MESSAGE_APP) {
        return false;
    }

    if (packet.id == 0) {
        packet.id = generatePacketId();
    }

    const std::string command(reinterpret_cast<const char *>(packet.decoded.payload.bytes), packet.decoded.payload.size);
    sendResponse(packet, executeCommand(trim(command)));

    meshtastic_QueueStatus queueStatus = router->getQueueStatus();
    service->sendQueueStatusToPhone(queueStatus, ERRNO_OK, packet.id);

    meshtastic_MeshPacket ackSource = packet;
    ackSource.from = nodeDB->getNodeNum();
    service->sendRoutingErrorResponse(meshtastic_Routing_Error_NONE, &ackSource);
    return true;
}

ProcessMessage LHCBadgeModule::handleReceived(const meshtastic_MeshPacket &packet)
{
    if (isLhcChannel(packet.channel) && packet.decoded.payload.size >= 5 &&
        memcmp(packet.decoded.payload.bytes, "disco", 5) == 0 && ambientLightingThread) {
        ambientLightingThread->showAlert();
    }
    return ProcessMessage::CONTINUE;
}

bool LHCBadgeModule::wantPacket(const meshtastic_MeshPacket *packet)
{
    return packet->which_payload_variant == meshtastic_MeshPacket_decoded_tag &&
           packet->decoded.portnum == meshtastic_PortNum_TEXT_MESSAGE_APP;
}

std::string LHCBadgeModule::executeCommand(const std::string &command)
{
    if (!ambientLightingThread) {
        return "Lighting controller unavailable";
    }

    if (command == "/help" || lhc_badge::isShortCommand(command, 'h')) {
        return "Available commands:\n"
               "Aliases accept an optional leading / (for example /e 12).\n"
               "/help (h) - Show this help\n"
               "/effect (e) <0-70> - Set animation effect\n"
               "/brightness (b) <0-255> - Set brightness\n"
               "/speed (s) <1-10000> - Set animation speed\n"
               "/color (c) <R> <G> <B> - Set RGB color\n"
               "/show (d) - Display current settings\n"
               "/next (n) - Go to next effect\n"
               "/prev (p) - Go to previous effect\n"
               "/on - Turn on ambient lighting\n"
               "/off - Turn off ambient lighting\n"
               "/reboot - Restart the device";
    }

    std::string argument;
    int value = 0;
    if (getArgument(command, "/effect", 'e', argument)) {
        if (!parseInteger(argument, value) || !ambientLightingThread->setEffect(value)) {
            return "Invalid animation effect. Valid range: 0-70";
        }
        return "Effect set to: " + std::to_string(value);
    }
    if (getArgument(command, "/brightness", 'b', argument)) {
        if (!parseInteger(argument, value) || value < 0 || value > 255) {
            return "Invalid brightness. Valid range: 0-255";
        }
        ambientLightingThread->setBrightness(static_cast<uint8_t>(value));
        return "Brightness set to: " + std::to_string(value);
    }
    if (getArgument(command, "/speed", 's', argument)) {
        if (!parseInteger(argument, value) || !ambientLightingThread->setSpeed(value)) {
            return "Invalid speed. Valid range: 1-10000";
        }
        return "Speed set to: " + std::to_string(value);
    }
    if (getArgument(command, "/color", 'c', argument)) {
        std::istringstream values(argument);
        int red = -1;
        int green = -1;
        int blue = -1;
        std::string extra;
        if (!(values >> red >> green >> blue) || values >> extra || red < 0 || red > 255 || green < 0 || green > 255 ||
            blue < 0 || blue > 255) {
            return "Invalid RGB format. Usage: /color <R> <G> <B>";
        }
        ambientLightingThread->setColor(static_cast<uint8_t>(red), static_cast<uint8_t>(green), static_cast<uint8_t>(blue));
        return "Color set to: R=" + std::to_string(red) + ", G=" + std::to_string(green) + ", B=" + std::to_string(blue);
    }
    if (command == "/show" || lhc_badge::isShortCommand(command, 'd')) {
        std::ostringstream settings;
        settings << "Current settings:\n"
                 << "Status: " << (ambientLightingThread->isEnabled() ? "ON" : "OFF") << "\n"
                 << "Effect: " << static_cast<int>(moduleConfig.ambient_lighting.animation) << "\n"
                 << "Brightness: " << static_cast<int>(moduleConfig.ambient_lighting.brightness) << "\n"
                 << "Speed: " << moduleConfig.ambient_lighting.speed << "\n"
                 << "Color: R=" << static_cast<int>(moduleConfig.ambient_lighting.red)
                 << ", G=" << static_cast<int>(moduleConfig.ambient_lighting.green)
                 << ", B=" << static_cast<int>(moduleConfig.ambient_lighting.blue);
        return settings.str();
    }
    if (command == "/next" || lhc_badge::isShortCommand(command, 'n')) {
        return "Animation effect changed to: " + std::to_string(ambientLightingThread->nextEffect());
    }
    if (command == "/prev" || lhc_badge::isShortCommand(command, 'p')) {
        return "Animation effect changed to: " + std::to_string(ambientLightingThread->previousEffect());
    }
    if (command == "/on") {
        ambientLightingThread->setEnabled(true);
        return "Ambient lighting turned ON";
    }
    if (command == "/off") {
        ambientLightingThread->setEnabled(false);
        return "Ambient lighting turned OFF";
    }
    if (command == "/reboot") {
        rebootAtMsec = millis() + 4000;
        return "Device will reboot in 4 seconds...";
    }

    return "Unknown command. Use /help for available commands";
}

void LHCBadgeModule::sendResponse(const meshtastic_MeshPacket &request, const std::string &response)
{
    const size_t totalParts = std::max<size_t>(1, (response.size() + RESPONSE_CHUNK_SIZE - 1) / RESPONSE_CHUNK_SIZE);
    for (size_t part = 0; part < totalParts; ++part) {
        std::string chunk = response.substr(part * RESPONSE_CHUNK_SIZE, RESPONSE_CHUNK_SIZE);
        if (totalParts > 1) {
            chunk = "[" + std::to_string(part + 1) + "/" + std::to_string(totalParts) + "] " + chunk;
        }

        meshtastic_MeshPacket *packet = packetPool.allocCopy(request);
        if (!packet) {
            LOG_ERROR("Unable to allocate LHC badge response packet");
            return;
        }
        packet->id = generatePacketId();
        packet->from = RESPONSE_SENDER;
        packet->to = nodeDB->getNodeNum();
        packet->want_ack = false;
        packet->decoded.want_response = false;
        packet->decoded.request_id = 0;
        packet->decoded.reply_id = request.id;
        packet->decoded.payload.size = std::min(chunk.size(), sizeof(packet->decoded.payload.bytes));
        memcpy(packet->decoded.payload.bytes, chunk.data(), packet->decoded.payload.size);
        service->sendToPhone(packet);
    }
}

int LHCBadgeModule::onBluetoothStatusUpdate(const meshtastic::Status *status)
{
    if (!status || status->getStatusType() != STATUS_TYPE_BLUETOOTH ||
        config.bluetooth.mode != meshtastic_Config_BluetoothConfig_PairingMode_FIXED_PIN || !ambientLightingThread) {
        return 0;
    }

    const auto *bluetooth = static_cast<const meshtastic::BluetoothStatus *>(status);
    if (bluetooth->getConnectionState() == meshtastic::BluetoothStatus::ConnectionState::PAIRING) {
        ambientLightingThread->showPairingCode(bluetooth->getPasskey());
    } else {
        ambientLightingThread->restorePersistedLighting();
    }
    return 0;
}

int LHCBadgeModule::onInputEvent(const InputEvent *event)
{
    if (!event || strcmp(event->source, "LHCBadgeButton") != 0 || !ambientLightingThread) {
        return 0;
    }

    switch (event->inputEvent) {
    case INPUT_BROKER_LIGHT_NEXT:
        ambientLightingThread->nextEffect();
        return 1;
    case INPUT_BROKER_LIGHT_PREVIOUS:
        ambientLightingThread->previousEffect();
        return 1;
    case INPUT_BROKER_LIGHT_TOGGLE:
        ambientLightingThread->setEnabled(!ambientLightingThread->isEnabled());
        return 1;
    default:
        return 0;
    }
}

#endif
