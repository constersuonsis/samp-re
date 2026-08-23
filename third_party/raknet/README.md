# RakNet (vendored)

Third-party networking library used by the SA-MP wire protocol. This directory
is not maintained as part of this project beyond the changes listed below.

## Origin

RakNet 3.x generation, Copyright 2003 Kevin Jenkins (Rakkarsoft).

The copy this project started from came from a SA-MP bot framework, which had
bolted a bot-multiplexing hook onto the RPC path. That hook has been removed —
see below.

## License

The upstream headers offer a choice of licenses, one of which is:

> the GNU General Public License as published by the Free Software Foundation;
> either version 2 of the License, or (at your option) any later version.

The project takes that option. Because it is "version 2 or any later version",
it is compatible with the GPL v3 the rest of this repository is under.

The per-file copyright notices must stay intact. In particular, the project-wide
rule against comments in source files does **not** apply here: these comments
belong to the library's authors and carry its license.

## Changes from upstream

1. **The bot hook was replaced with a plain RPC callback.** Upstream RakNet
   dispatches remote calls by registered name, but SA-MP addresses them by
   numeric id, so a catch-all notification is needed. The version inherited here
   provided one as `RegisterRPCHandle(void*, uint64_t botID)`: an untyped
   function pointer plus a bot identifier, delivering the payload by value.

   It is now:

   ```cpp
   typedef void ( *RpcHandler )( unsigned char rpcId, RakNet::BitStream *payload,
                                 RakPeerInterface *peer, void *context );

   virtual void SetRpcHandler( RpcHandler handler, void *context ) = 0;
   ```

   The signature is typed, the payload travels by pointer instead of being
   copied, and the `void *context` lets a caller find its own object again.
   Without that context the wrapper had to cast a function pointer through
   `void *` and keep a global peer-to-session map behind a mutex; none of that
   is needed now.

   Touched: `rakpeerinterface.h`, `rakpeer.h`, `RakPeer.cpp`,
   `RakClientInterface.h`, `RakClient.h`, `RakClient.cpp`.

2. **The callback pointer is now initialised.** `RakPeer`'s constructor never
   set it, and the dispatch path tested it for null, so the first RPC on a fresh
   peer read an indeterminate pointer.

3. **Dead bot code was deleted** from `RakPeer::HandleRPCPacket` — a commented-out
   `pAlphaBot` block and a commented-out debug logger.

4. **`CMakeLists.txt` in this directory is ours.** It lists sources explicitly
   rather than globbing, because the upstream tree also ships a console server,
   an admin command parser and table serialisation that a game client has no use
   for.

No other source file has been edited.
