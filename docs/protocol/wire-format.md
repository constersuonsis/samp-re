# Wire format

Cross-cutting rules of the client protocol. Individual message layouts live in
the headers under `include/samp/protocol/`; this document covers the conventions
that span messages and are easy to get wrong when reading any single one of them.

## Packet framing

Every outgoing packet is framed and encrypted before it reaches the socket:

```
byte 0        checksum
byte 1        padding length in the low nibble, filler in the high nibble
bytes 2..     padding, then the payload
```

The whole frame is padded to a multiple of eight bytes and encrypted with XTEA.
The checksum covers everything except itself and is computed **before**
encryption, so a receiver decrypts first and validates second.

Two details are easy to miss. The padding length occupies only the low nibble —
the high nibble is filler, and reading the byte whole gives a nonsense length.
And the cipher is XTEA with a four-*byte* key array rather than the textbook
four 32-bit words, so a stock XTEA implementation will not interoperate.

See `include/samp/crypto/`.

## Field encodings

Boolean values are **not** consistently encoded. Some occupy a single bit, some
a full byte, and which is which follows no rule beyond what each handler does:

| Single bit | Whole byte |
| --- | --- |
| camera target reports | widescreen |
| textdraw selection | siren, landing gear |
| object edit target kind | line-of-sight test |
| the four actor animation switches | actor visibility |
| the eleven session settings | most other flags |
| object attachment present | |
| vehicle collisions, stunt bonus | |

A message containing bit-sized fields is generally not a whole number of bytes.
Reading such a field as a byte leaves everything after it shifted.

## Message bodies

Messages fall into three groups, and a dispatcher has to treat them differently:

1. **Body used.** The ordinary case.
2. **Body read and discarded.** The bytes must still be consumed or the stream
   desynchronises. `EmptyPacket` is the example.
3. **No body at all.** The id is the whole instruction. `HasEmptyBody()` in
   `rpc_ids.h` lists them; the set includes several commands whose close
   relatives *do* carry fields, so the distinction cannot be guessed from a name.

## Strings

Five different encodings are in use:

| Encoding | Used by |
| --- | --- |
| byte length + data | chat, dialogs, animations, plates, names |
| 32-bit length + data, capped at 255 | client messages |
| fixed 32-byte field, terminator-delimited | shop name |
| Huffman-compressed, bit length prefix | dialog body, material text, 3D labels |
| 16-bit length + data | textdraws |

Length limits are per-field and none of them repeat: 24 for nicknames, 31 for
texture and font names, 32 for plates and shop names, 144 for chat bubbles, 200
for game text, 255 for the general case, 800 for textdraws, 2048 for material
and label text, 4096 for dialog bodies.

The compressed form uses a Huffman tree built from a frequency table baked into
both ends. The numbers never travel — only the tree they produce — so the table
must be reproduced exactly. Symbols with zero frequency are given weight one,
which is the only reason bytes outside the ASCII range remain encodable.

## Pool limits

Ids from the network index fixed arrays and are bounds-checked first. The limits
differ, and so does whether the top value is accepted:

| Pool | Limit | Top value accepted |
| --- | --- | --- |
| players | 1004 | yes |
| vehicles | 2000 | no |
| objects | 1000 | yes |
| actors | 1000 | no |
| 3D labels | 2048 | no |
| textdraws | 2304 | no |
| pickups | 4096 | no |

Objects and actors share a limit but not the comparison. See `pool_limits.h`.

## Absent values

There is no single marker for "not set". Within one message alone three
different ones appear:

| Field | Absent when |
| --- | --- |
| body colour | `0xFF` |
| paint job | `0` |
| modification colour | `-1` |
| attachment target | `0xFFFF` |
| killer in a death message | `0xFFFF` |
| team | `0xFF` |
| player colour on join | `0` |
| weapon slot | `-1` |
| material model, object creation | above 20000 |
| material model, material update | exactly `0xFFFF` |

The last pair is worth restating: the same material entry format is interpreted
differently depending on which message carried it.

## Synchronisation

Outgoing sync is sent as a raw block of bytes with a leading id, and only when a
field changed or half a second passed since the last send. Incoming vehicle sync
arrives **bit-packed** instead — the server re-encodes it on relay — so the two
directions do not share a format.

The compressed form quantises: rotations lose their fourth component and are
rebuilt from the unit-length constraint, velocities are split into a magnitude
and a direction, and health and armour are cut to a nibble each in steps of
seven. A value of 1 through 6 cannot be represented; rounding it down would
report a living player as dead.
