# Message catalogue

Sizes of the messages whose layout is fixed, and notes on the ones that are not.
Names and ids live in `rpc_ids.h`; structures live in `rpc_payloads.h` and
`sync_structures.h`. This page exists for the sizes, which are the part that
cannot be read off a struct definition — the payloads are serialised field by
field rather than copied, so `sizeof` says nothing about the wire.

## Sync payloads

These are the exception: they travel as a block of bytes, so the struct layout
*is* the wire layout and is pinned by static assertions.

| Message | Id | Bytes |
| --- | --- | --- |
| Aim | 0xCB | 31 |
| Bullet | 0xCE | 40 |
| Player (on foot) | 0xCF | 68 |
| Unoccupied vehicle | 0xD1 | 67 |
| Trailer | 0xD2 | 54 |
| Passenger | 0xD3 | 24 |
| Vehicle (decoded state) | — | 63 |

Vehicle sync is listed without an id because the client only ever receives it,
and it arrives bit-packed rather than as a block.

## Fixed-size RPC payloads

| Message | Bytes |
| --- | --- |
| ExitVehicle, ServerQuit, SetPlayerSpecialAction, SetVehicleDoors | 3–4 |
| EnterVehicle, PutPlayerInVehicle | 5, 3 |
| DeathMessage | 5 |
| SetPlayerColour, SetVehicleHealth, SetVehicleZAngle | 6 |
| ClientCheckResponse | 6 |
| SetPlayerSkillLevel, GivePlayerWeapon | 8 |
| ClientCheckRequest | 9 |
| VehicleDamageStatus | 12 |
| SetPlayerPosition, SetPlayerPositionFindZ | 12 |
| SetObjectSpeed, SetActorPosition | 14 |
| SetVehiclePosition | 14 |
| SetCheckpoint, SetPlayerWorldBounds | 16 |
| PlaySound | 16 |
| ScmEvent | 18 |
| SetPlayerMapIcon | 19 |
| RemoveBuildingForPlayer | 20 |
| ApplyObjectMovement | 22 |
| ShowActor | 27 |
| AttachObjectToPlayer | 28 |
| SetRaceCheckpoint | 29 |
| SetPlayerShopName | 32 |
| MoveObject | 42 |
| SpawnInfo (also nested in RequestClassResponse) | 46 |
| WorldPlayerAdd | 50 |
| WorldVehicleAdd | 63 |
| ServerNetStats | 296 |

Single-field messages of one, two or four bytes are not listed individually.

## Messages with no fixed size

| Message | Why |
| --- | --- |
| CreateObject | attachment block optional, material list follows |
| SetObjectMaterial, object material entries | two entry shapes, compressed text |
| ShowTextDraw | fixed 63-byte style block plus variable text |
| Create3DTextLabel | fixed header plus compressed text |
| ChatMessage, ClientMessage, DialogHeader | length-prefixed strings |
| ChatBubble, SetVehicleNumberPlate, SetPlayerName | length-prefixed strings |
| ApplyActorAnimation | two strings and four single-bit switches |
| SetPlayerAttachedObject | 52-byte description present only when attaching |
| InitGame | eleven single-bit flags, a name, a 212-byte block |
| UpdateScoresPingsIPs | entries run to the end of the packet |

## Messages with no body

Receiving the id is the whole instruction. `HasEmptyBody()` is the authority:

EnterEditObject, CancelEdit, DisableCheckpoint, DisableRaceCheckpoint,
GameModeRestart, StopAudioStream, ForceClassSelection, SetCameraBehindPlayer,
RemovePlayerFromVehicle.

`EmptyPacket` is **not** in this group: it carries two bytes that the client
reads and discards. They still have to be consumed.

## Bit-sized messages

Not a whole number of bytes, and reading their leading field as a byte
misaligns everything after it:

| Message | Bits |
| --- | --- |
| ToggleCameraTarget, EnableStuntBonus, DisableVehicleCollisions | 1 |
| EditObject | 17 |
| SelectTextDraw | 33 |
| SetPlayerAttachedObject (clearing a slot) | 49 |
| ApplyActorAnimation, InitGame | variable |
