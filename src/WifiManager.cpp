#include <WiFi.h>
#include <Logger.h>
#include <WifiManager.h>

#define WIFI_CREDENTIALS_FILE "/wifi_credentials.txt"
#define AP_SSID "ESP32_AP"
#define WIFI_TIMEOUT_MS 20000 // 20 second WiFi connection timeout
#define WIFI_ENABLE_PIN 14
#define WIFI_ENABLE_DEBOUNCE_MS 50
#define WIFI_RETRY_INTERVAL_MS 30000

using namespace RidenDongle;

WifiManagerClass RidenDongle::WifiManager;

namespace
{
volatile bool wifiEnabled = false;
bool enablePinCandidate = false;
uint32_t enablePinCandidateSince = 0;
uint32_t wifiConnectStartedAt = 0;
uint32_t wifiRetryAt = 0;
bool wifiConnectPending = false;
}

void WifiManagerClass::begin()
{
    pinMode(WIFI_ENABLE_PIN, INPUT_PULLUP);
    wifiEnabled = digitalRead(WIFI_ENABLE_PIN) == HIGH;
    enablePinCandidate = wifiEnabled;
    enablePinCandidateSince = millis();

    if (readCredentials())
    {
        Serial.println("WiFi credentials read successfully.");
        Serial.printf("SSID: %s, Password: %s\n", ssid.c_str(), _password.c_str());
    }
    else
    {
        Serial.println("WiFi credentials not found or invalid.");
    }

    if (!wifiEnabled)
    {
        // Initialize the network stack used by AsyncTCP and the Ethernet services
        // without enabling the Wi-Fi radio.
        WiFi.mode(WIFI_STA);
        WiFi.mode(WIFI_OFF);
        Serial.println("WiFi disabled by GPIO14.");
        return;
    }

    if (ssid.isEmpty() || _password.isEmpty())
    {
        Serial.println("Starting AP mode.");
        startAPMode();
    }
    else
    {
        connect();
    }
}

void WifiManagerClass::loop()
{
    const bool pinState = digitalRead(WIFI_ENABLE_PIN) == HIGH;
    if (pinState != enablePinCandidate)
    {
        enablePinCandidate = pinState;
        enablePinCandidateSince = millis();
        return;
    }

    if (pinState == wifiEnabled ||
        millis() - enablePinCandidateSince < WIFI_ENABLE_DEBOUNCE_MS)
    {
        return;
    }

    wifiEnabled = pinState;
    if (wifiEnabled)
    {
        Serial.println("WiFi enabled by GPIO14.");
        if (ssid.isEmpty() || _password.isEmpty())
        {
            startAPMode();
        }
        else
        {
            startStationConnection();
        }
    }
    else
    {
        Serial.println("WiFi disabled by GPIO14.");
        WiFi.mode(WIFI_OFF);
        wifiConnectPending = false;
    }

    if (!wifiEnabled)
    {
        return;
    }

    if (wifiConnectPending)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            wifiConnectPending = false;
            Serial.println("Connected to WiFi successfully.");
            Serial.print("IP Address: ");
            Serial.println(WiFi.localIP());
        }
        else if (millis() - wifiConnectStartedAt >= WIFI_TIMEOUT_MS)
        {
            wifiConnectPending = false;
            wifiRetryAt = millis() + WIFI_RETRY_INTERVAL_MS;
            Serial.println("Failed to connect to WiFi. Starting AP mode.");
            startAPMode();
        }
    }
    else if (!ssid.isEmpty() && !_password.isEmpty() &&
             WiFi.status() != WL_CONNECTED &&
             static_cast<int32_t>(millis() - wifiRetryAt) >= 0)
    {
        startStationConnection();
    }
}

bool WifiManagerClass::isEnabled() const
{
    return wifiEnabled;
}

bool WifiManagerClass::clearCredentials()
{
    if (LittleFS.remove(WIFI_CREDENTIALS_FILE))
    {
        if (wifiEnabled)
        {
            WiFi.eraseAP();
        }
        LittleFS.end();
        ESP.restart();
        return true;
    }
    return false;
}

bool WifiManagerClass::readCredentials()
{
  File file = LittleFS.open(WIFI_CREDENTIALS_FILE, FILE_READ);
  if (!file)
  {
    Serial.println("Failed to open wifi credentials file");
    return false;
  }

  ssid = file.readStringUntil('\n');
  _password = file.readStringUntil('\n');

  // Remove newline characters
  ssid.trim();
  _password.trim();

  file.close();

  return !ssid.isEmpty() && !_password.isEmpty();
}

void WifiManagerClass::startAPMode()
{
  if (!wifiEnabled)
  {
    return;
  }

  WiFi.softAP(AP_SSID);

  Serial.println("Access Point started:");
  Serial.print("SSID: ");
  Serial.println(AP_SSID);
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());
}

void WifiManagerClass::connect()
{
    if (!wifiEnabled)
    {
        return;
    }

    startStationConnection();
}

void WifiManagerClass::startStationConnection()
{
    if (!wifiEnabled)
    {
        return;
    }

    LOG_LN("[WIFI] Connecting");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), _password.c_str());
    wifiConnectStartedAt = millis();
    wifiConnectPending = true;
}

void WifiManagerClass::setCredentials(const String &ssid, const String &password)
{
    this->ssid = ssid;
    _password = password;
}

bool WifiManagerClass::saveCredentials()
{
  File file = LittleFS.open(WIFI_CREDENTIALS_FILE, FILE_WRITE);
  if (!file)
  {
    Serial.println("Failed to open wifi credentials file for writing");
    return false;
  }

  file.println(ssid);
  file.println(_password);

  file.close();
  return true;
}
