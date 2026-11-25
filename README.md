# ericOS

Firmware for the **E.R.I.C. handheld** (RP2350 / Pico 2W).

This is the community-facing release snapshot: the verified firmware image and the
first-party source needed to study or rebuild it. It contains no development history,
old snapshots, or machine-specific paths.

**License:** [MIT](LICENSE). The project is licensed to make study, modification and
contribution straightforward for the community.

## Verified release

| | |
|---|---|
| Version | v1.6 |
| Firmware | [`firmware/ericOS-v1.6.uf2`](firmware/ericOS-v1.6.uf2) |
| SHA-256 | `0358c2a3e25709e8e35ccb852cf839c844a993b1019e3b8244e2bba58a38c376` |
| UF2 | 2,992 blocks, 765,952 B payload |
| Flash range | `0x10000000..0x100bb000` (RP2350_RISCV) |
| Program | 732,824 B (23%) |
| Static RAM | 410,372 B (78%), 113,916 B free |
| License | [MIT](LICENSE) |

Every shipped feature was confirmed on hardware: **S.O.U.N.D.**, **R.A.D.I.O.**,
**F.I.L.E.S.**, **G.A.M.E.**, **S.T.O.R.M.**, **T.R.A.C.K.S.**, image viewing, A.D.D.,
and phone file transfer.

## Install

1. Hold **BOOTSEL** while powering the board, then connect USB.
2. Copy `firmware/ericOS-v1.6.uf2` to the board's USB drive.
3. Eject the drive. The board reboots into ericOS automatically.

To verify the image before copying:

```sh
python3 verify_uf2.py firmware/ericOS-v1.6.uf2
```

## What is inside

| Path | Contents |
|---|---|
| `firmware/` | The verified UF2 release image |
| `EricOS/` | Arduino sketch and first-party C++ sources |
| `EricOS/src/` | `.cpp` / `.h` files for the apps, audio, Wi-Fi, weather, DAW and UI |
| `EricOS/libraries/EricAudioCompat/` | Local compatibility shim for the audio API |
| `build.sh` | Compile, verify and optionally deploy |
| `verify_uf2.py` | UF2 structure validator |
| `LICENSE` | MIT license for the ericOS project code |

Third-party libraries are intentionally not bundled. Install them separately as described
under [Building from source](#building-from-source).

## Apps

- **S.O.U.N.D.** — local MP3 playback from SD card.
- **R.A.D.I.O.** — four internet radio stations; connects over plain HTTP by design.
- **F.I.L.E.S.** — browse the SD card, open images and text files.
- **G.A.M.E.** — Game Boy cartridge emulation with a Quick Menu overlay.
- **S.T.O.R.M.** — *Standard Temperature & Overcast Radar Module*. City lookup and a
  seven-day forecast using the keyless Open-Meteo endpoints. A real clock is required for
  certificate validation, so the firmware starts NTP before each weather request.
- **T.R.A.C.K.S.** — a 6-track, 16-step sequencer with a dedicated synth PWM output.
- **A.D.D.** — arithmetic decoder/device with a clipped function graph.
- **W.I.F.I.** — network setup and phone file transfer.

## Phone file transfer

From **W.I.F.I.**, press **F** to start the transfer hotspot:

| Setting | Value |
|---|---|
| SSID | `ERIC-Files` |
| Password | `eosfiles` |
| Browser | `http://192.168.4.1` |

Uploads land in `/ERIC/INBOX/`. The hotspot runs in `WIFI_AP_STA` mode, so **R.A.D.I.O.**
keeps its own station connection. Pressing **Back** stops the web service but leaves the
access point associated for a quick return; it shuts down after three idle minutes.

The password is a shared device credential and is published here on purpose. To use a
different one, edit `EricOS/src/file_transfer.cpp` and the on-screen hint in
`EricOS/src/main.cpp`, then rebuild.

## Radio stations

| Slot | Stream |
|---|---|
| RAP | `http://bigrradio.cdnstream1.com/5105_128` (2000 FM) |
| R&B | `http://listen.181fm.com/181-rnb_128k.mp3` (181.FM True RnB) |
| HOUSE | `http://stream.radioabf.com:8000/abf-sd.mp3` |
| ROCK | `http://stream.radioparadise.com/rock-192` |

Plain HTTP is deliberate: the compatibility audio shim has no TLS, and an `https://`
URL is refused with a clear on-screen message.

## T.R.A.C.K.S. keys

| Key | Function |
|---|---|
| SPC | Play / stop |
| ENT | Toggle the step at the cursor |
| +/-, `]`, `[` | Change note (octaves 1-6) |
| `,`, `.`, `<`, `>` | Change BPM (40-200) |
| LEFT / RIGHT | Move the cursor |
| UP / DOWN | Switch track |
| BACK | Stop playback |

The 16 x 6 grid is centred on the visible 320 x 320 panel. Only the changed cell or step
indicator is repainted during editing.

## Building from source

Use the pinned toolchain for a reproducible image:

- `arduino-cli` 1.5.1 or newer
- core `rp2040:rp2040@6.1.1`
- libraries `Arduino_GFX`, `BackgroundAudio`, `TJpg_Decoder`
- the bundled `EricAudioCompat` shim

`arduino-cli` ignores libraries inside a sketch folder, so the shim must also be visible
from the Arduino libraries directory:

```sh
arduino-cli core update-index --additional-urls \
  https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
arduino-cli core install rp2040:rp2040@6.1.1
arduino-cli lib install Arduino_GFX BackgroundAudio TJpg_Decoder

mkdir -p "$HOME/Arduino/libraries"
ln -s "$PWD/EricOS/libraries/EricAudioCompat" "$HOME/Arduino/libraries/EricAudioCompat"

./build.sh
```

`build.sh` uses the FQBN
`rp2040:rp2040:rpipico2w:flash=4194304_1048576,freq=200,opt=Optimize2` and verifies the
resulting UF2. To flash directly, put the board into BOOTSEL mode and run:

```sh
RPI_RP2=/path/to/board ./build.sh --deploy
```

Different library or compiler versions can produce a different binary. Compare SHA-256
hashes rather than file sizes when identifying a build.

## License and attribution

The ericOS project code is released under the **MIT License**; see [`LICENSE`](LICENSE).
MIT is used deliberately: it permits reuse, modification and redistribution with minimal
restriction, it is compatible with the MIT-licensed Peanut-GB and SameBoy portions below,
and it does not require derivative projects to adopt the same license. As with all MIT
software, it is provided without warranty and does not restrict commercial use.

`EricOS/src/peanut_gb.h` is Peanut-GB, MIT-licensed, Copyright (c) 2018-2023 Mahyar
Koshkouei. It includes portions derived from SameBoy, Copyright (c) 2015-2019 Lior
Halphon. The upstream notice is preserved in that file.

The separately installed libraries keep their own licenses and are not redistributed here.
