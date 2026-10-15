#pragma once

#ifndef WIREDJSON_H
#define WIREDJSON_H

// Wired JSON: answers "GET /json" on UART0 with the same JSON the /json HTTP
// handler sends, framed STX (0x02) ... ETX (0x03), so a board wired to the cam
// (the Foucault Dynamics CV node) can read it without Wi-Fi.
//
// Protocol and wiring: IoT-Firmware, docs/CV_WIRED_LINK.md.
// Enabled with -D ENABLE_WIRED_JSON in platformio.ini.

// Starts the task that listens on UART0. Safe to call before the flow
// exists: until then every request gets an empty frame.
void StartWiredJsonTask(void);

#endif // WIREDJSON_H
