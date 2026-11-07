#ifndef RidenStatus_h
#define RidenStatus_h

#include "Arduino.h"
#include "stdlib_noniso.h"

#include "AsyncTCP.h"
#include "ESPAsyncWebServer.h"
#include "WiFi.h"
#include <riden_modbus/riden_modbus.h>

// typedef std::function<void(uint8_t *data, size_t len)> WSLMessageHandler;
// typedef std::function<void(const String& msg)> WSLStringMessageHandler;
namespace RidenDongle
{
class RidenStatusClass
{
  public:
    void begin(AsyncWebServer *server, RidenModbus *modbus, const char *url = "/ridenstatus");
    void start();
    void end();
    size_t getConnectionCount();
    void sendStatus();
    bool updateStatus(bool full=false);
    bool getAllValues(RidenDongle::AllValues& values);
    bool queueSetVoltage(double voltage);
    bool queueSetCurrent(double current);
    bool queueToggleOutput();
    void loop();

  private:
    enum class CommandType {
        SetVoltage,
        SetCurrent,
        ToggleOutput,
    };

    struct Command {
        CommandType type;
        double value;
    };

    static constexpr size_t COMMAND_QUEUE_CAPACITY = 8;

    AsyncWebServer *_server;
    RidenModbus *_modbus;
    AsyncWebSocket *_ws;
    TaskHandle_t _xHandle;
    RidenDongle::AllValues _allValues{};
    portMUX_TYPE _statusMux = portMUX_INITIALIZER_UNLOCKED;
    portMUX_TYPE _commandMux = portMUX_INITIALIZER_UNLOCKED;
    Command _commandQueue[COMMAND_QUEUE_CAPACITY]{};
    size_t _commandHead = 0;
    size_t _commandTail = 0;
    size_t _commandCount = 0;
    bool _hasValues = false;
    bool _started = false;
    bool enqueueCommand(CommandType type, double value = 0.0);
    bool dequeueCommand(Command& command);
    void processCommand(const Command& command);
    String _serializeAllValues(const RidenDongle::AllValues& values);
};

} // namespace RidenDongle

extern RidenDongle::RidenStatusClass RidenStatus;

#endif