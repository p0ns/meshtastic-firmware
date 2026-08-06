# LHC Badge 2025

This variant contains the complete LHC 2025 event firmware behavior. The
hardware-only port is maintained separately on `feat/lhc-badge-2025`; this
branch adds the historical event experience on top of that variant.

## Build

```sh
pio run -e lhc-badge-2025
```

The target uses an ESP32-S3-WROOM-1-N16R8, an SX1262 radio, and the 16 MB
partition layout. Build artifacts are written under
`.pio/build/lhc-badge-2025/`.

## Lighting

Eight GRB NeoPixels on GPIO 8 run WS2812FX 1.4.5 effects 0 through 70. The
following settings persist in the ambient-lighting module configuration:

- effect
- brightness
- speed
- RGB color
- on/off state

Defaults are effect 39, brightness 50, speed 200, and a node-number-derived
color. Configurations created before the animation fields existed are migrated
when their speed is zero.

The secondary button on GPIO 38 controls lighting:

- single click: next effect
- double click: previous effect
- one-second hold: toggle lighting

A message beginning with `disco` on the `LHC` channel displays effect 12 for
five seconds, then restores the persisted settings.

## Bluetooth Pairing

Random-PIN mode generates a new six-digit code containing only digits 4, 5,
and 6 whenever Bluetooth starts. The first six NeoPixels display the code:

- 4: red
- 5: green
- 6: blue

The remaining pixels are off. In fixed-PIN mode, all pixels show purple while
the configured PIN is requested. In no-PIN mode, all pixels show cyan after a
client connects. The normal effect resumes after successful pairing,
disconnect, or a 30-second timeout.

Use USB serial to return the badge to RGB-code pairing:

```sh
meshtastic --port PORT --set bluetooth.enabled true --set bluetooth.mode RANDOM_PIN
```

The configuration update restarts the badge. Existing bonded clients can
reconnect without another pairing prompt, so remove the bond from the client
before a complete pairing test.

## Local Command Channel

Text sent by a directly connected client on `LHCBADGECFG` is handled locally
and is not transmitted over LoRa. Responses are returned to the client as text
from the historical synthetic badge node ID.

| Command                     | Action                     |
| --------------------------- | -------------------------- |
| `/help` or `h`              | Show help                  |
| `/effect N` or `e N`        | Select effect 0-70         |
| `/brightness N` or `b N`    | Set brightness 0-255       |
| `/speed N` or `s N`         | Set speed 1-10000          |
| `/color R G B` or `c R G B` | Set color components 0-255 |
| `/show` or `d`              | Show current settings      |
| `/next` or `n`              | Select the next effect     |
| `/prev` or `p`              | Select the previous effect |
| `/on`                       | Enable lighting            |
| `/off`                      | Disable lighting           |
| `/reboot`                   | Reboot after four seconds  |

Remote text on the configuration channel is hidden from normal message
history, matching the original event firmware.

## Porting Notes

The complete port intentionally does not reproduce obsolete implementation
details from the old fork:

- no `Router` or `MeshService` access-control widening
- no duplicate NeoPixel/WS2812FX objects
- no uncommitted one-off SH1107 display define
- no manual-only protobuf header edits
- no stale `.roocoderules`

Animation fields are defined in the protobuf source and regenerated into the
firmware nanopb structures. The schema remains additive and wire-compatible
with the legacy field tags 6 through 8.
