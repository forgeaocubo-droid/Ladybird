#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <WiFiUdp.h>

#include <functional>
#include <vector>

#include "includes/Config.hpp"

namespace chimera {

struct WifiNetwork {
  String ssid;
  int32_t rssi;
  uint32_t lastSeenMs;
};
struct StationInfo {
  String mac;
  uint32_t lastSeenMs;
};
enum class WifiStatus : uint8_t { Disconnected, Connecting, Connected, Failed };
enum class WifiEventType : uint8_t {
  StationConnected,
  StationDisconnected,
  GotIp,
  ApStationConnected,
  ApStationDisconnected
};

/** Compile-time AP or station Wi-Fi wrapper with TCP transport. */
class WifiAPSTA {
 public:
  /** Creates the compile-time-selected Wi-Fi service. */
  WifiAPSTA();
  /** Starts the selected mode and TCP server. @return True when the mode
   * starts or a station connection attempt is initiated. */
  bool initialize();
  /** Starts or collects an asynchronous scan. @return Cached visible networks.
   */
  std::vector<WifiNetwork> staScan();
  /** Starts a station connection using compile-time credentials. @param ssid
   * Configured network name. @return True when an attempt starts. */
  bool staConnect(const String& ssid);
  /** Returns current station status. @return Connection state. */
  WifiStatus staStatus() const;
  /** Disconnects the station interface. */
  void staDisconnect();
  /** Returns the station IP address. @return Current station IP. */
  IPAddress staLocalIP() const;
  /** Returns the connected SSID. @return Current station network name. */
  String staSsid() const;
  /** Returns the connected signal strength. @return RSSI in dBm. */
  int32_t staRssi() const;
  /** Registers a lightweight station event callback. @param type Event to
   * observe. @param callback Callback invoked during update(). */
  void staOnEvent(WifiEventType type,
                  std::function<void(WifiEventType)> callback);
  /** Returns connected AP client count. @return Number of stations. */
  int apGetStationNum() const;
  /** Returns connected AP station MAC addresses. @return Station details. */
  std::vector<StationInfo> apGetStationList() const;
  /** Registers a lightweight AP event callback. @param type Event to observe.
   * @param callback Callback invoked during update(). */
  void apOnEvent(WifiEventType type,
                 std::function<void(WifiEventType)> callback);
  /** Writes bytes to the accepted TCP client. @param data Bytes. @param length
   * Byte count. @return Bytes written. */
  size_t tcpWrite(const uint8_t* data, size_t length);
  /** Reads bytes from an accepted TCP client. @param data Destination. @param
   * length Capacity. @return Bytes read. */
  size_t tcpRead(uint8_t* data, size_t length);
  /** Accepts pending clients and reports buffered data. @return Available byte
   * count. */
  int tcpAvailable();
  /** Returns the active TCP client's session generation. @return A value
   * incremented for each newly accepted client. */
  uint32_t tcpSessionId() const;
  /** Starts a UDP packet. @param ip Destination address. @param port
   * Destination port. @return True when begun. */
  bool udpBeginPacket(const IPAddress& ip, uint16_t port);
  /** Writes bytes to the current UDP packet. @param data Bytes. @param length
   * Byte count. @return Bytes written. */
  size_t udpWrite(const uint8_t* data, size_t length);
  /** Completes the current UDP packet. @return True when sent. */
  bool udpEndPacket();
  /** Parses an incoming UDP packet. @return Packet size, or zero. */
  int udpParsePacket();
  /** Reads bytes from the current UDP packet. @param buffer Destination.
   * @param length Capacity. @return Bytes read. */
  int udpRead(uint8_t* buffer, size_t length);
  /** Advances scans, connection state, and events without waiting. */
  void update();
  /** Reports whether the selected Wi-Fi mode has an operational connection.
   * @return True when a station is connected to the AP or STA has connected. */
  bool hasOperationalConnection();
  /** Reports whether runtime credential files are required. @return Always
   * false; credentials are compiled into firmware. */
  bool credentialsMissing() const;
  /** Reports whether the soft access point started. @return True when AP is
   * available. */
  bool apAvailable() const;
  /** Returns the configured AP name. @return AP SSID. */
  String apName() const;
  /** Returns the configured AP password. @return Development AP password. */
  String apPassword() const;

 private:
  struct EventCallback {
    WifiEventType type;
    std::function<void(WifiEventType)> callback;
  };
  void fire(std::vector<EventCallback>& callbacks, WifiEventType type);
  void refreshApStationList();
  WiFiServer server_{kWifiTcpPort};
  WiFiClient client_;
  WiFiUDP udp_;
  std::vector<WifiNetwork> networks_;
  std::vector<StationInfo> apStations_;
  std::vector<EventCallback> stationCallbacks_;
  std::vector<EventCallback> apCallbacks_;
  String apName_;
  String apPassword_;
  String connectingSsid_;
  uint32_t connectStartedMs_ = 0;
  int previousApStations_ = 0;
  bool scanRunning_ = false;
  bool connecting_ = false;
  bool apStarted_ = false;
  bool failed_ = false;
  bool previousStaConnected_ = false;
  uint32_t tcpSessionId_ = 0;
};

}  // namespace chimera