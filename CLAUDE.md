# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

PlatformIO/Arduino firmware for ESP32-S3 boards that act as I/O modules for a CyBears fork of the
Freezy Arena FRC field management system. One source tree builds three different physical devices,
selected entirely by preprocessor flags. The boards replace a Modbus PLC by pushing button state to,
and polling light state from, the arena's HTTP API.

## Commands

`pio` is not on PATH. Use the absolute path:

```bash
PIO=~/.platformio/penv/bin/pio

$PIO run -e field_estops                 # build (default env is team_estops if -e omitted)
$PIO run -e field_estops -t upload       # flash over USB
$PIO run -e field_estops -t upload -t monitor
$PIO device monitor -b 115200            # serial monitor, 115200 baud
$PIO run -t clean -e field_estops
```

Environments: `team_estops`, `field_estops`, `hub`, `esp32dev` (experimental).

There is no test suite. `test/`, `include/`, and `lib/` contain only placeholder READMEs, and no
`pio test` targets are configured. Verification is done by flashing hardware and reading serial output.

## Build-flag architecture

Everything is selected by `-D` flags in `platformio.ini`, in two independent axes:

| Axis | Flags | Effect |
|---|---|---|
| Device role | `PLC_MODEL_TEAM`, `PLC_MODEL_FIELD_TABLE`, `PLC_MODEL_FIELD_HUB` | Which model header compiles, `NUM_LEDS`, which loop work runs |
| Network | `CON_ETH`, `CON_WIFI` | `NetworkEthernet.h` (W5500 SPI) vs `NetworkWiFi.h` |
| Board | `ESP32_S3_DEVKITM_1`, `ESP32DEV` | GPIO pin maps |
| Optional | `ONBATTERY` | Compiles battery ADC sampling and battery POSTs (hub only) |

`field_estops` = `CON_ETH` + `PLC_MODEL_FIELD_TABLE`, the score-table box with a field e-stop and a
4-layer field stack light.

## Header-as-implementation pattern (important)

Almost all logic lives in `.h` files that **define** functions and globals rather than declaring them.
`main.cpp` is the only translation unit that includes them, so this links only because each header is
included exactly once. Consequences that will bite you:

- `Field_stack_lightStatus.h`, `Team_stack_lightStatus.h`, and `Field_hub_lightStatus.h` each define
  their own `CRGB g_LEDs[NUM_LEDS]` and `setupLEDs()`. Exactly one may be compiled per environment.
- `GlobalSettings.h` defines globals at file scope (`allianceColor`, `arenaIP`, `arenaPort`,
  `deviceIP`, `useDHCP`, `gatewayIP`, `netmask`). Including it from a second `.cpp` produces duplicate
  symbols. Other translation units must use `extern` instead, as `WebServerSetup.cpp` does.
- Include order in `main.cpp` is a contract, not style. `LEDSTRIP` and `NUM_LEDS` must be `#define`d
  before the model header. `connected()`, `local_ip`, `gateway`, and `subnet` must exist before the
  network header. Reordering the includes breaks the build in non-obvious ways.

`main.cpp` `loop()` never delays. Each subsystem rate-limits itself against `millis()` (200 ms poll
interval for light status, 5 s for battery). HTTP calls are synchronous and blocking, which caps how
smooth LED animation can be. Blink timing is shared global state in `BlinkState.h`.

## Configuration precedence

Two parallel sources for the arena address exist and can disagree:

- `baseUrl`, a hard-coded `const char*` in `main.cpp`, is used by `postStopStatus.h` and `StartMatch.h`.
- `arenaIP` + `arenaPort`, read from NVS Preferences, are used by the light-status and hub-status headers.

So a device can successfully poll lights from one host while POSTing e-stops to another. When
debugging "it talks but nothing happens", check both.

Runtime settings are edited through the on-device async web server on port 80 (`/setup`), persisted to
the `settings` NVS namespace, and require a reboot. Note that `setup()` loads `deviceIP` and
`allianceColor` from Preferences with defaults of `""` and `"Red"`, which overwrite the compile-time
values in `GlobalSettings.h` on any device that has never been configured through the web UI.

`allianceColor` is one of `Red`, `Blue`, `Field`. It does double duty: it selects the channel offset
for e-stop POSTs, and it is the query parameter and JSON key for light GETs.

## Arena API contract

The server lives in a separate repo at `/Users/chrisjohnson/Documents/Code/fms-freesy-arena`
(GitHub `Byron-MN-FRC/fms-freesy-arena`, forked from `Freezy-Arena/freezy-arena`). It is not a sibling
directory of this repo despite being referred to as `../fms-freesy-arena`. Relevant files there:

- `web/web.go` route table
- `web/alternateIO.go` all `/api/freezy/*` handlers
- `plc/plc.go` the `input` enum that defines channel numbers, plus coil state
- `plc/esp32IO.go` per-module health and last-seen tracking
- `field/arena.go` where PLC state is actually consumed

| Endpoint | Method | Used by | Payload |
|---|---|---|---|
| `/api/freezy/eStopState` | POST | team, field table | `[{"channel":int,"state":bool}]` |
| `/api/freezy/field_stack_light` | GET | field table | `{redStackLight,blueStackLight,orangeStackLight,greenStackLight}` all bool |
| `/api/freezy/team_stack_light?alliance=` | GET | team | `{red:[3],blue:[3]}`, each `{lightStates:[2]}` of `{color,blink}` |
| `/api/freezy/hub_status?alliance=` | GET | hub | `{red:{color,blink,rgb},blue:{...}}` |
| `/api/freezy/hub_status?alliance=` | POST | hub | `{voltage,percent}` |
| `/api/freezy/startMatch` | POST | team, field table | any body |

Channel numbers are indices into the `input` enum in `plc/plc.go`:

```
0  fieldEStop
1  red1EStop    2  red1AStop    3  red2EStop    4  red2AStop    5  red3EStop    6  red3AStop
7  blue1EStop   8  blue1AStop   9  blue2EStop  10  blue2AStop  11  blue3EStop  12  blue3AStop
```

That is why `postAllStopStatus` uses offset 0 for Red and 6 for Blue. Both team boards also post
channel 0 from their own field-stop button.

### Polarity

The arena inverts on read (`GetFieldEStop()` and `GetTeamEStops()` return `!plc.inputs[...]`), so the
wire convention is **`state: true` means released, `state: false` means pressed**.

The firmware does not apply that inversion uniformly. In `processButtonStates()`, indices 2, 4 and 6
(the A-stops) are handled with the opposite sense from the field stop and E-stops. All pins are
`INPUT_PULLDOWN`. Getting this backwards is easy, so check `main.cpp` against `plc.go` rather than
assuming.

### Arena-side gating (most common cause of "it posts but nothing happens")

In `field/arena.go`, `handlePlcInputOutput()` processes team stops unconditionally, then returns early
unless `Plc.IsEnabled() || EventSettings.AlternateIOEnabled`. Field e-stop abort and every
`SetStackLights()` call sit below that return. With "Enable Alternate IO" unchecked in arena Settings
and no PLC address configured, POSTs still return HTTP 200 and `field_stack_light` still returns all
false forever.

The inverse also holds: if a real PLC address is set and connected, `readInputs()` overwrites the whole
input array roughly every 100 ms and discards anything an ESP32 posted.

Arena Settings also has per-module IP fields (Score Table, Red/Blue Alliance, Red/Blue Hub). Those
drive the TCP health check in `plc/esp32IO.go` and the IP-based fallback in `UpdateLastSeenFromAddress`,
so a device on an unexpected address shows as unhealthy on the match play page even while its API calls
are working. Loading a match calls `Plc.ResetEstops()`, which sets every stop input back to released.

## Known quirks in this firmware

These are verified, not hypothetical. Treat them as landmines when debugging.

- **Preferences override the static IP, and the parsed value is then discarded.** `initNetwork()` in
  `NetworkEthernet.h` parses `deviceIP` into a local `IPAddress localIP` for validation but passes the
  file-scope `local_ip` to `ETH.config()`. `local_ip` was constructed at static-init time from the
  compile-time `GlobalSettings.h` value. On a device that has never been configured via the web UI,
  `deviceIP` loads as `""`, validation fails, and it silently falls back to DHCP even though
  `useDHCP` is false. Editing `GlobalSettings.h` alone does not reliably set the address.
- **The first button state is recorded but never sent.** `processButtonStates()` updates
  `lastStopButtonStates` and clears its change flag even when the POST is skipped because the network
  is not up yet. Arena inputs default to false, which reads as e-stop active, until a match load calls
  `ResetEstops()` or someone physically toggles the button.
- **`sizeof` misuse in the LED index loops.** `setFieldIndicator()` iterates
  `sizeof(ledMatrix[0][0])` and `setDSIndicator()` iterates `sizeof(ledMatrix[0][0][0])`, both of which
  evaluate to 4 (`sizeof(int)`), not an element count. It happens to equal the real row length. The
  bounds checks compare against `sizeof(...)` the same way and are effectively meaningless.
- `Field_stack_lightStatus.h` calls `FastLED.show()` once per poll, while `Team_stack_lightStatus.h`
  calls it inside `setDSIndicator()`, so the team build shows partial frames mid-update.

## Hardware reference

ESP32-S3-DevKitM-1 with W5500 Ethernet (PoE). Stop button pins in GPIO order are Field, 1E, 1A, 2E, 2A,
3E, 3A on 33 and 34 through 39. Start match is 40, LED strip data is 17, battery ADC is GPIO1.

LED strips are 12 V WS2811 in **BRG** order with three physical LEDs per addressable pixel. Team stack
lights are 24 pixels wired DS2 to DS3 to DS1, two layers each. The field stack light is 16 pixels in
four layers. Index maps are the `ledMatrix` tables in the respective model headers. Layer semantics
follow the FMS whitepaper field status indicator spec; `stacklights/README.md` has the state table and
3D print/wiring details.
