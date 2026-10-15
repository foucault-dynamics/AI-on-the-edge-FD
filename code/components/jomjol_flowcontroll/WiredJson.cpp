#include "../../include/defines.h"

#ifdef ENABLE_WIRED_JSON

#include "WiredJson.h"

#include <string>

#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "ClassLogFile.h"
#include "MainFlowControl.h"

static const char *TAG = "WIREDJSON";

// Set by MainFlowControl.cpp once the flow is running, the same check handler_json makes
extern bool bTaskAutoFlowCreated;

#define WIRED_JSON_UART UART_NUM_0  // The console UART: U0RXD GPIO 3, U0TXD GPIO 1
#define WIRED_JSON_RX_BUFFER 1024
#define WIRED_JSON_MAX_LINE 64     // Longer lines are not requests and are dropped
#define WIRED_JSON_STX '\x02'
#define WIRED_JSON_ETX '\x03'

static void sendFrame(const std::string &body)
{
    // One write, so a log line printed meanwhile cannot land inside the frame
    std::string frame;
    frame.reserve(body.length() + 2);
    frame += WIRED_JSON_STX;
    frame += body;
    frame += WIRED_JSON_ETX;
    uart_write_bytes(WIRED_JSON_UART, frame.data(), frame.length());
}

static void handleLine(const std::string &line)
{
    if (line.rfind("GET ", 0) != 0)
    {
        return; // Not a request, for example stray bytes while the C3 boots
    }

    std::string path = line.substr(4);
    std::string body;

    if (path == "/json" && bTaskAutoFlowCreated)
    {
        body = flowctrl.getJSON();
    }

    ESP_LOGD(TAG, "%s -> %d byte reply", line.c_str(), (int)body.length());
    sendFrame(body); // Unknown path or flow not started yet: empty frame
}

static void wiredJsonTask(void *pvParameter)
{
    std::string line;
    uint8_t b;

    while (true)
    {
        if (uart_read_bytes(WIRED_JSON_UART, &b, 1, portMAX_DELAY) != 1)
        {
            continue;
        }

        if (b == '\r')
        {
            continue;
        }

        if (b == '\n')
        {
            if (line.length() <= WIRED_JSON_MAX_LINE)
            {
                handleLine(line);
            }
            line.clear();
            continue;
        }

        if (line.length() <= WIRED_JSON_MAX_LINE)
        {
            line += (char)b;
        }
    }
}

void StartWiredJsonTask(void)
{
    // UART0 is already set up as the console at 115200 8N1, only the driver is missing.
    // Without the driver nothing can read it. Logging carries on unchanged.
    if (!uart_is_driver_installed(WIRED_JSON_UART))
    {
        esp_err_t err = uart_driver_install(WIRED_JSON_UART, WIRED_JSON_RX_BUFFER, 0, 0, NULL, 0);
        if (err != ESP_OK)
        {
            LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "UART driver install failed: " + std::string(esp_err_to_name(err)));
            return;
        }
    }

    if (xTaskCreate(&wiredJsonTask, "wired_json", 6 * 1024, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS)
    {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Task creation failed");
        return;
    }

    LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Answering GET /json on UART0 (GPIO 1 TX, GPIO 3 RX)");
}

#endif // ENABLE_WIRED_JSON
