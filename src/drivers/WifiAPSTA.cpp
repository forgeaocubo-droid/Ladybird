#include "drivers/WifiAPSTA.hpp"

#include <esp_wifi.h>
#include <stdio.h>

#include <algorithm>

#include "includes/Config.hpp"
#include "utils/Logger.hpp"

namespace chimera {

WifiAPSTA::WifiAPSTA() = default;

bool WifiAPSTA::initialize() {
  gLogger.logDebug("Wi-Fi: applying configured interface mode");
  apName_ = kApSsid;
  apPassword_ = kApPassword;

  if (kWifiMode == WifiMode::AccessPointStation) {
    WiFi.mode(WIFI_AP_STA);
    gLogger.logInfo("Wifi starting in AP+STA mode");
    apStarted_ = WiFi.softAP(apName_.c_str(), apPassword_.c_str());
    if (!apStarted_) {
      gLogger.logError("Wi-Fi AP failed to start: " + apName_);
    }
    WiFi.begin(kStaSsid, kStaPassword);
    connecting_ = true;
    connectStartedMs_ = millis();
  } else {
    WiFi.mode(kWifiMode == WifiMode::AccessPoint ? WIFI_AP : WIFI_STA);
    WiFi.setSleep(false);
    WiFi.setAutoReconnect(true);
    if (kWifiMode == WifiMode::AccessPoint) {
      gLogger.logInfo("Wifi starting in AP mode");
      apStarted_ = WiFi.softAP(apName_.c_str(), apPassword_.c_str());
      if (!apStarted_) gLogger.logError("Wi-Fi AP failed to start: " + apName_);
    } else {
      gLogger.logInfo("Wifi starting in STA mode");
      WiFi.begin(kStaSsid, kStaPassword);
      connecting_ = true;
      connectStartedMs_ = millis();
    }
  }

  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
  server_.begin();
  server_.setNoDelay(true);
  gLogger.logDebug(String("Wi-Fi: TCP server listening on port ") +
                   String(kWifiTcpPort));
  if (kWifiMode == WifiMode::AccessPoint || kWifiMode == WifiMode::AccessPointStation)
    previousApStations_ = WiFi.softAPgetStationNum();
  return (kWifiMode == WifiMode::AccessPoint ? apStarted_ : true) ||
         kWifiMode == WifiMode::AccessPointStation;
}

std::vector<WifiNetwork> WifiAPSTA::staScan() {
  const int result = WiFi.scanComplete();
  if (scanRunning_ && result != WIFI_SCAN_RUNNING) {
    scanRunning_ = false;
    if (result >= 0) {
      const uint32_t now = millis();
      for (int index = 0; index < result; ++index) {
        const String ssid = WiFi.SSID(index);
        auto network = std::find_if(
            networks_.begin(), networks_.end(),
            [&ssid](const WifiNetwork& item) { return item.ssid == ssid; });
        if (network == networks_.end()) {
          networks_.push_back({ssid, WiFi.RSSI(index), now});
        } else {
          network->rssi = WiFi.RSSI(index);
          network->lastSeenMs = now;
        }
      }
    }
    WiFi.scanDelete();
  } else if (!scanRunning_ && result == WIFI_SCAN_FAILED) {
    scanRunning_ = WiFi.scanNetworks(true, true) == WIFI_SCAN_RUNNING;
  }
  const uint32_t now = millis();
  networks_.erase(std::remove_if(networks_.begin(), networks_.end(),
                                 [now](const WifiNetwork& network) {
                                   return now - network.lastSeenMs >
                                          kWifiNetworkTtlMs;
                                 }),
                  networks_.end());
  return networks_;
}

bool WifiAPSTA::staConnect(const String& ssid) {
  if (kWifiMode != WifiMode::Station || ssid != kStaSsid) return false;
  WiFi.begin(kStaSsid, kStaPassword);
  connectingSsid_ = kStaSsid;
  connecting_ = true;
  failed_ = false;
  connectStartedMs_ = millis();
  return true;
}

WifiStatus WifiAPSTA::staStatus() const {
  if (WiFi.status() == WL_CONNECTED) return WifiStatus::Connected;
  if (connecting_) return WifiStatus::Connecting;
  return failed_ ? WifiStatus::Failed : WifiStatus::Disconnected;
}

void WifiAPSTA::staDisconnect() {
  WiFi.disconnect(false, false);
  connecting_ = false;
  failed_ = false;
  if (!scanRunning_) WiFi.scanDelete();
}
IPAddress WifiAPSTA::staLocalIP() const {
  return WiFi.status() == WL_CONNECTED ? WiFi.localIP() : IPAddress();
}
String WifiAPSTA::staSsid() const {
  return WiFi.status() == WL_CONNECTED ? WiFi.SSID() : String();
}
int32_t WifiAPSTA::staRssi() const {
  return WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0;
}

void WifiAPSTA::staOnEvent(WifiEventType type,
                           std::function<void(WifiEventType)> callback) {
  stationCallbacks_.push_back({type, std::move(callback)});
}

int WifiAPSTA::apGetStationNum() const { return WiFi.softAPgetStationNum(); }

std::vector<StationInfo> WifiAPSTA::apGetStationList() const {
  return apStations_;
}

void WifiAPSTA::refreshApStationList() {
  wifi_sta_list_t list{};
  const uint32_t now = millis();
  if (esp_wifi_ap_get_sta_list(&list) != ESP_OK) {
    apStations_.erase(std::remove_if(apStations_.begin(), apStations_.end(),
                                     [now](const StationInfo& station) {
                                       return now - station.lastSeenMs >
                                              kApClientTtlMs;
                                     }),
                      apStations_.end());
    return;
  }

  std::vector<StationInfo> stations;
  stations.reserve(list.num);
  for (int index = 0; index < list.num; ++index) {
    char mac[18];
    snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
             list.sta[index].mac[0], list.sta[index].mac[1],
             list.sta[index].mac[2], list.sta[index].mac[3],
             list.sta[index].mac[4], list.sta[index].mac[5]);
    stations.push_back({String(mac), now});
  }
  apStations_.swap(stations);
}

void WifiAPSTA::apOnEvent(WifiEventType type,
                          std::function<void(WifiEventType)> callback) {
  apCallbacks_.push_back({type, std::move(callback)});
}

size_t WifiAPSTA::tcpWrite(const uint8_t* data, size_t length) {
  return client_ && client_.connected() ? client_.write(data, length) : 0;
}
size_t WifiAPSTA::tcpRead(uint8_t* data, size_t length) {
  if (!client_ || !client_.available()) return 0;
  const size_t received = client_.read(data, length);
  if (received > 0) {
    gLogger.logDebug(String("Wi-Fi TCP: read ") + String(received) +
                     " bytes from active client");
  }
  return received;
}

int WifiAPSTA::tcpAvailable() {
  if (server_.hasClient()) {
    WiFiClient incoming = server_.available();
    if (!client_ || !client_.connected()) {
      client_ = incoming;
      ++tcpSessionId_;
      gLogger.logDebug(String("Wi-Fi TCP: accepted client session ") +
                       String(tcpSessionId_));
    } else {
      gLogger.logDebug(
          "Wi-Fi TCP: rejected additional client; one active client is supported");
      incoming.stop();
    }
  }
  return client_ && client_.connected() ? client_.available() : 0;
}

uint32_t WifiAPSTA::tcpSessionId() const { return tcpSessionId_; }

bool WifiAPSTA::udpBeginPacket(const IPAddress& ip, uint16_t port) {
  return udp_.beginPacket(ip, port) == 1;
}
size_t WifiAPSTA::udpWrite(const uint8_t* data, size_t length) {
  return udp_.write(data, length);
}
bool WifiAPSTA::udpEndPacket() { return udp_.endPacket() == 1; }
int WifiAPSTA::udpParsePacket() { return udp_.parsePacket(); }
int WifiAPSTA::udpRead(uint8_t* buffer, size_t length) {
  return udp_.read(buffer, length);
}

void WifiAPSTA::fire(std::vector<EventCallback>& callbacks,
                     WifiEventType type) {
  for (EventCallback& item : callbacks)
    if (item.type == type && item.callback) item.callback(type);
}

void WifiAPSTA::update() {
  if (kWifiMode == WifiMode::AccessPoint || kWifiMode == WifiMode::AccessPointStation)
    refreshApStationList();
  const bool stationConnected = WiFi.status() == WL_CONNECTED;
  if ((kWifiMode == WifiMode::Station || kWifiMode == WifiMode::AccessPointStation) &&
      stationConnected != previousStaConnected_) {
    previousStaConnected_ = stationConnected;
    connecting_ = false;
    failed_ = false;
    if (stationConnected) {
      gLogger.logInfo("Wi-Fi client connected");
      fire(stationCallbacks_, WifiEventType::StationConnected);
      fire(stationCallbacks_, WifiEventType::GotIp);
    } else {
      gLogger.logInfo("Wi-Fi client disconnected; automatic reconnect enabled");
      fire(stationCallbacks_, WifiEventType::StationDisconnected);
    }
  }
  if (connecting_) {
    if (stationConnected) {
      connecting_ = false;
      failed_ = false;
      fire(stationCallbacks_, WifiEventType::StationConnected);
      fire(stationCallbacks_, WifiEventType::GotIp);
    } else if (millis() - connectStartedMs_ >= kWifiConnectTimeoutMs) {
      connecting_ = false;
      failed_ = true;
      gLogger.logError("Wi-Fi client connection is still pending");
      fire(stationCallbacks_, WifiEventType::StationDisconnected);
    }
  }
  if (kWifiMode != WifiMode::AccessPoint && kWifiMode != WifiMode::AccessPointStation)
    return;
  const int stations = WiFi.softAPgetStationNum();
  if (stations > previousApStations_) {
    gLogger.logInfo("Wi-Fi AP client connected");
    fire(apCallbacks_, WifiEventType::ApStationConnected);
  }
  if (stations < previousApStations_) {
    gLogger.logInfo("Wi-Fi AP client disconnected");
    fire(apCallbacks_, WifiEventType::ApStationDisconnected);
  }
  previousApStations_ = stations;
}

bool WifiAPSTA::hasOperationalConnection() {
  if (kWifiMode == WifiMode::AccessPointStation) {
    return (client_ && client_.connected()) || WiFi.status() == WL_CONNECTED;
  }
  return kWifiMode == WifiMode::AccessPoint
             ? client_ && client_.connected()
             : WiFi.status() == WL_CONNECTED;
}

bool WifiAPSTA::credentialsMissing() const { return false; }
bool WifiAPSTA::apAvailable() const { return apStarted_; }
String WifiAPSTA::apName() const { return apName_; }
String WifiAPSTA::apPassword() const { return apPassword_; }

}  // namespace chimera