# samp-client-r3

> **Open-source reimplementation of SA-MP R3 client library (`samp.dll`)**

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Version](https://img.shields.io/badge/SA--MP-R3-orange.svg)]()
[![Status](https://img.shields.io/badge/status-WIP-yellow.svg)]()

---

## About

This project is a full reverse-engineered, clean C++ reimplementation of `samp.dll` — the core client-side library of **San Andreas Multiplayer (SA-MP) R3**.

The goal is to give the SA-MP community complete transparency into the client protocol, enable third-party client development, and preserve the SA-MP ecosystem for the long term.

The original binary was fully decompiled using **IDA Pro with Hex-Rays**, resulting in **6 244 named and documented functions** covering every major subsystem of the client.

---

## Motivation

SA-MP has been one of the most played GTA San Andreas multiplayer mods for over a decade, with millions of players and thousands of active servers worldwide. Despite this, the client has always remained a black box. This project changes that by:

- Making the client-side protocol fully public and auditable
- Enabling custom clients, tools, and bots without guesswork
- Allowing server developers to understand exactly how the client behaves
- Preserving the SA-MP ecosystem independently of the original authors

---

## Coverage

All **6 244 functions** in `samp.dll R3` have been identified, named, and decompiled. The codebase is organized into the following major subsystems:

### 🌐 Networking
| Module | Description |
|--------|-------------|
| `RakNet` | Core RakNet networking layer (~154 functions) |
| `RakNetWrapper` / `RakNetPeer` | SA-MP wrappers around RakNet (~163 functions) |
| `BitStream` | Packet serialization/deserialization (~40 functions) |
| `RingBuffer` / `CStreamBuffer` | Buffered I/O for network data |
| `PacketLogger` | Network packet logging |

### 📡 Protocol / RPC
| Module | Description |
|--------|-------------|
| `RPC_*` | All RPC handlers: `REQUESTSPAWN`, `REQUESTCLASS`, `SETINTERIORID`, `GIVETAKEDAMAGE`, `CAMERATARGET`, `GIVEACTORDAMAGE` and more (~144 functions) |
| `CSAMPClient` | Main client class: sync sending, incoming packet processing, state management (~90 functions) |

**Documented sync types:**
- Player sync (`SendPlayerSync`, `ApplyPlayerSyncData`)
- Vehicle sync (`SendVehicleSync`, `ApplyVehicleSyncData`, `CheckVehicleUpdate`)
- Unoccupied vehicle sync
- Passenger sync
- Aim sync
- Bullet sync
- Trailer sync
- Weapons update
- Spectator position

### 🧠 Game Interface
| Module | Description |
|--------|-------------|
| `GTA_*` | Direct GTA SA game engine hooks and accessors (~205 functions) |
| `GamePtr` | Managed pointers into the GTA SA address space (~96 functions) |
| `Camera` | Camera control and sync (~25 functions) |

### 👥 Player & Vehicle
| Module | Description |
|--------|-------------|
| `CPlayerInfo` | Player state, stats, and metadata (~134 functions) |
| `CPlayerSyncState` | Per-player sync state tracking (~80 functions) |
| `CPlayerPool` | Player pool management (~17 functions) |
| `CVehicleInfo` | Vehicle state and metadata (~62 functions) |

### 🖥️ UI / HUD
| Module | Description |
|--------|-------------|
| `ChatWindow` | In-game chat rendering and input (~35 functions) |
| `CTextDrawManager` / `CTextDrawTable` | Textdraw system |
| `CScoreboard` | Scoreboard rendering (~39 functions) |
| `CDialog` / `Dialog` | Dialog box handling (~53 functions) |
| `CUIElement` / `CUIListBox` | Base UI widgets (~69 functions) |
| `CTextInput` | Text input field (~24 functions) |
| `IME` | Input Method Editor support (~42 functions) |
| `Settings` | Client settings/config (~69 functions) |
| `DisplayMode` | Resolution and display mode management (~54 functions) |

### 🔧 Objects & World
| Module | Description |
|--------|-------------|
| `CObjectManager` | World object management (~91 functions) |
| `CObjectPool` | Object pool allocator (~17 functions) |
| `CBSP` | BSP tree for spatial queries (~35 functions) |
| `CParticleEmitter` | Particle system interface (~37 functions) |

### 🔐 Crypto & Compression
| Module | Description |
|--------|-------------|
| `AES` | AES encryption (~29 functions) |
| `RSA` | RSA key exchange (~16 functions) |
| `BigInt` | Big integer math for RSA (~134 functions) |
| `HuffmanCode` / `HuffmanQueue` | Huffman compression (~52 functions) |
| `LZMA` | LZMA compression (~90 functions) |
| `CZlibBuffer` | Zlib compression wrapper (~22 functions) |

### 📦 Data Structures
| Module | Description |
|--------|-------------|
| `DynArray` | Dynamic array (~190 functions) |
| `ObjVec` / `OpcodeVec` / `OpcodeArray` | Typed vectors |
| `BTree` / `RBTree` | Binary/red-black trees (~113 functions) |
| `CircList` | Circular linked list (~34 functions) |
| `IntPtrArray` | Integer/pointer array (~34 functions) |

### 🎮 Scripting & Download
| Module | Description |
|--------|-------------|
| `ScriptEngine` | Client-side script engine (~85 functions) |
| `ScriptOpcodeQueue` | Opcode execution queue (~15 functions) |
| `CDownloadManager` | Asset/script download management (~64 functions) |
| `CFileEntryArray` | File entry management (~22 functions) |

### 🔊 Audio & Misc
| Module | Description |
|--------|-------------|
| `BASS` | BASS audio library integration (~17 functions) |
| `IRC` | IRC-style chat protocol (~20 functions) |
| `CServerBrowser` | Server browser (~43 functions) |
| `Logger` | Internal logging (~20 functions) |
| `Cmd` | Command processing (~16 functions) |

---

## Repository Structure

```
samp-client-r3/
├── src/
│   ├── network/        # RakNet, BitStream, RakNetWrapper
│   ├── protocol/       # RPC handlers, CSAMPClient, sync structs
│   ├── game/           # GTA hooks, GamePtr, Camera
│   ├── player/         # CPlayerInfo, CPlayerSyncState, CPlayerPool
│   ├── vehicle/        # CVehicleInfo
│   ├── ui/             # Chat, HUD, Dialog, TextDraw, Scoreboard
│   ├── objects/        # CObjectManager, CObjectPool, CBSP
│   ├── crypto/         # AES, RSA, BigInt
│   ├── compression/    # LZMA, Huffman, Zlib
│   ├── scripting/      # ScriptEngine, CDownloadManager
│   └── util/           # Data structures, Logger, Cmd
├── include/
│   └── samp/           # Public headers
├── ida/
│   ├── samp_dll.h      # IDA-generated type definitions
│   └── samp_named_dump.txt  # Full IDA function dump (6244 functions)
├── docs/
│   └── protocol/       # SA-MP R3 protocol documentation (WIP)
├── LICENSE
└── README.md
```

---

## Building

> Work in progress. Build system coming soon.

**Requirements:**
- MSVC 2019+ or MinGW-w64
- CMake 3.20+
- Windows SDK (for Win32 types)

```bash
git clone https://github.com/constersuonsis/samp-re
cd samp-re
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

---

## IDA Database

The full reverse engineering artifacts are included in `ida/`:

- `samp_dll.h` — IDA-exported type library with all structs, unions, and typedefs
- `samp_named_dump.txt` — Full decompiled dump of all 6 244 functions with decompiled C pseudocode

These files are provided for reference and documentation purposes.

---

## Disclaimer

This project is a clean-room reimplementation based on reverse engineering for interoperability and preservation purposes. It is not affiliated with or endorsed by the SA-MP team or Rockstar Games. GTA San Andreas is a trademark of Rockstar Games.

This project does **not** include any original binary code from `samp.dll`.

---

## License

This project is licensed under the **GNU General Public License v3.0**.  
See [LICENSE](LICENSE) for full terms.

In short: you are free to use, study, modify, and distribute this code, but any derivative work must also be released under GPL v3. Nobody can take your work and close it.

---

## Contributing

All contributions are welcome — whether it's improving function documentation, writing unit tests, completing struct definitions, or working on protocol documentation.

Please open an issue before starting large refactors so we can coordinate.
