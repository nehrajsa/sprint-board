#ifdef ESP32

#include "controller.h"
#include <Arduino.h>
#include <NimBLEDevice.h>

volatile uint8_t controllerDirection = 0;

static const NimBLEUUID HID_SVC_UUID("1812");
static const NimBLEUUID HID_REPORT_UUID("2a4d");

static NimBLEClient *bleClient = nullptr;
static volatile bool deviceFound = false;
static NimBLEAddress foundAddress;
static volatile bool connecting = false;
static bool bleInitialized = false;
static volatile bool pairingRequested = false;

// ---------- HID report parsing ----------
// Xbox One S / Series X in BLE mode (hat-switch d-pad):
//   byte 0-1 : buttons
//   byte 2   : d-pad nibble (0=up 1=NE 2=right 3=SE 4=down 5=SW 6=left 7=NW 8=center)
//   bytes 3-4: left stick X (int16 LE)
//   bytes 5-6: left stick Y (int16 LE, negative = up)

static uint8_t parseReport(const uint8_t *d, size_t len)
{
  if (len < 3)
    return 0;

  uint8_t hat = d[2] & 0x0F;
  if      (hat == 0 || hat == 1 || hat == 7) return 1; // up
  else if (hat == 2 || hat == 3)             return 2; // right
  else if (hat == 4 || hat == 5)             return 3; // down
  else if (hat == 6 || hat == 7)             return 4; // left

  if (len >= 7)
  {
    int16_t ax = (int16_t)(d[3] | (d[4] << 8));
    int16_t ay = (int16_t)(d[5] | (d[6] << 8));
    const int16_t DEAD = 10000;
    if      (ay < -DEAD) return 1;
    else if (ay >  DEAD) return 3;
    else if (ax >  DEAD) return 2;
    else if (ax < -DEAD) return 4;
  }
  return 0;
}

// ---------- BLE callbacks ----------

static void notifyCallback(NimBLERemoteCharacteristic *c,
                           uint8_t *data, size_t len, bool isNotify)
{
  uint8_t dir = parseReport(data, len);
  if (dir != 0)
    controllerDirection = dir;
}

class ClientCallbacks : public NimBLEClientCallbacks
{
  void onConnect(NimBLEClient *) override
  {
    Serial.println("[BLE] Controller connected");
  }
  void onDisconnect(NimBLEClient *client, int reason) override
  {
    Serial.println("[BLE] Controller disconnected");
    bleClient = nullptr;
  }
};

class ScanCallbacks : public NimBLEScanCallbacks
{
  void onResult(const NimBLEAdvertisedDevice *dev) override
  {
    if (connecting || bleClient || deviceFound)
      return;
    bool isHID  = dev->isAdvertisingService(HID_SVC_UUID);
    bool isXbox = dev->getName().find("Xbox") != std::string::npos;
    if (isHID || isXbox)
    {
      Serial.printf("[BLE] Found: %s\n", dev->getName().c_str());
      foundAddress = dev->getAddress();
      deviceFound  = true;
      NimBLEDevice::getScan()->stop();
    }
  }
};

// ---------- BLE init + scan task (runs off the loop task to avoid watchdog) ----------

static void bleInitTask(void *param)
{
  if (!bleInitialized)
  {
    NimBLEDevice::init("IKEA-LED");
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);

    // Enable bonding so Xbox controller completes its pairing handshake
    NimBLEDevice::setSecurityAuth(true, false, true);
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);

    NimBLEScan *scan = NimBLEDevice::getScan();
    scan->setScanCallbacks(new ScanCallbacks(), false);
    scan->setActiveScan(true);
    scan->setInterval(160); // 100 ms interval
    scan->setWindow(48);    // 30 ms window — 30% duty cycle leaves WiFi room

    bleInitialized = true;
    Serial.println("[BLE] NimBLE initialized");
  }

  Serial.println("[BLE] Scanning for controller (15 s)...");
  NimBLEDevice::getScan()->start(15, false);
  vTaskDelete(NULL);
}

// ---------- Connect task ----------

static void doConnectTask(void *param)
{
  NimBLEClient *client = NimBLEDevice::createClient(foundAddress);
  client->setClientCallbacks(new ClientCallbacks(), false);

  if (!client->connect())
  {
    Serial.println("[BLE] Connection failed");
    NimBLEDevice::deleteClient(client);
    connecting = false;
    vTaskDelete(NULL);
    return;
  }

  NimBLERemoteService *svc = client->getService(HID_SVC_UUID);
  if (!svc)
  {
    Serial.println("[BLE] HID service not found");
    client->disconnect();
    NimBLEDevice::deleteClient(client);
    connecting = false;
    vTaskDelete(NULL);
    return;
  }

  bool subscribed = false;
  for (auto &c : svc->getCharacteristics(true))
  {
    if (c->getUUID() == HID_REPORT_UUID && c->canNotify())
    {
      c->subscribe(true, notifyCallback);
      subscribed = true;
    }
  }

  if (subscribed)
  {
    bleClient = client;
    Serial.println("[BLE] Subscribed to HID reports — controller ready");
  }
  else
  {
    Serial.println("[BLE] No notifiable HID reports found");
    client->disconnect();
    NimBLEDevice::deleteClient(client);
  }
  connecting = false;
  vTaskDelete(NULL);
}

// ---------- Public API ----------

void initController()
{
  // NimBLE is initialized lazily in enableControllerPairing() to avoid
  // hanging or crashing during boot before the screen task is created.
}

const char *getControllerStatus()
{
  if (!bleInitialized)                          return "idle";
  if (bleClient && bleClient->isConnected())    return "connected";
  if (connecting)                               return "connecting";
  if (NimBLEDevice::getScan()->isScanning())    return "scanning";
  return "idle";
}

void updateController()
{
  if (pairingRequested)
  {
    pairingRequested = false;
    if (bleClient && bleClient->isConnected())
    {
      Serial.println("[BLE] Controller already connected");
    }
    else
    {
      xTaskCreatePinnedToCore(bleInitTask, "bleInit", 8192, NULL, 1, NULL, 1);
    }
  }

  if (deviceFound && !connecting && !bleClient)
  {
    deviceFound = false;
    connecting = true;
    xTaskCreatePinnedToCore(doConnectTask, "bleConnect", 8192, NULL, 1, NULL, 1);
  }
}

void enableControllerPairing()
{
  // Just set a flag — all BLE work happens in updateController() on Core 1
  // so we don't block the async web server task (Core 0) and drop the WS connection.
  pairingRequested = true;
}

#endif
