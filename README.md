# BeatEm

> A privacy-preserving chat that hides **who is talking to whom**, not
> just **what they say**.

Most encrypted chat tools protect message content but leak metadata: an
observer can still see *that* you and your peer are talking, *when*, and
*how often*. That graph is often more revealing than the messages
themselves.

BeatEm refuses to leak it. Every connected client emits a fixed-size
packet on a fixed schedule — either a real message or CSPRNG random
padding, indistinguishable on the wire. The server broadcasts the union
of every client's slot to every client. A passive observer sees the
exact same traffic pattern whether you're chatting at 3am or sleeping.

End-to-end encryption is libsodium `crypto_box` (X25519 + XSalsa20 +
Poly1305) with monotonic replay-counter protection. The server cannot
read your messages — and cannot tell which slots in its own broadcast
are real.

## Quickstart

```sh
sudo apt install build-essential libsodium-dev xxd
git clone <this-repo> beatem && cd beatem
cd build && make all
./beatem_server
# open http://localhost:27015/ in any browser
```

The server bundles the web client (installable as a PWA) on the same
TCP port as the WebSocket — no separate web server, no static-file
hosting. A terminal client `beatem_client` is also built.

For LAN / ngrok / Docker / VPS deployment, the PWA install, QR-based
peer exchange, and CLI usage, see **[USERS_GUIDE.md](USERS_GUIDE.md)**.

For architecture, wire format, threading model, and contribution
conventions, see **[CLAUDE.md](CLAUDE.md)**.

## How it works (short version)

```
[ 24-byte nonce ][ 16-byte Poly1305 MAC ][ ciphertext ]
                                          ↓ decrypts to
                [ 32-byte sender_pk ][ 4-byte counter ][ text ]
```

- Every client sends exactly one packet per heartbeat (default 2 s,
  128 B). If you typed something, it's encrypted to your peer;
  otherwise it's random bytes.
- The server collects each heartbeat's slots and broadcasts the
  concatenation to every connected client. Empty slots get padded with
  random bytes so the broadcast size is constant.
- Each receiver tries to decrypt every slot in every broadcast against
  every peer's public key it knows. The slot that decrypts is yours.
  Everything else fails authentication and is silently dropped.
- The embedded sender pubkey + monotonic counter give per-peer
  authentication and replay protection without any server state.

## What BeatEm does **not** protect against

Spelled out plainly because metadata-resistance is a strong claim and
there are real limits:

- **The server you connect to.** A modified server can log
  connections, fake any field, or otherwise weaken anonymity in ways
  the client cannot detect. Only connect to a server you or someone
  you trust runs. The project is small and easy to self-host (see
  `docker/server/Dockerfile`).
- **Sybil flooding.** A single party can open many WebSocket sessions
  to the server at near-zero cost. They then know which broadcast
  slots they didn't author, narrowing the real conversation. This is
  why the web client deliberately does **not** show a "connected
  clients" indicator — any such number can be manufactured.
- **Endpoint compromise.** If your browser, device, or extensions are
  compromised, none of this helps.
- **Anonymity from the recipient.** Successful decryption reveals the
  sender's pubkey to whoever they're talking to (it's needed for
  authentication). This isn't a metadata leak to the network — your
  peer already knows it's you — but it isn't an anonymous remailer
  either.

For deeper anonymity (hide the fact that you're a BeatEm user at all),
run the server as a Tor hidden service.

## Motivation

BeatEm exists to defend
[Article 12 of the UN Universal Declaration of Human Rights](https://www.un.org/en/about-us/universal-declaration-of-human-rights):

> No one shall be subjected to arbitrary interference with his privacy,
> family, home or correspondence, nor to attacks upon his honour and
> reputation. Everyone has the right to the protection of the law
> against such interference or attacks.

Encrypted message content protects *correspondence*. Hiding the
communication graph protects *privacy* — who you talk to, when, and
how often, which often reveals more than the messages.

## Project layout

```
source/         beatem_server, beatem_client, ws.c, crypto.c, protocol.c
include/        public headers (config, crypto, protocol, ws)
web/            single-file PWA client + service worker + icons
build/          Linux makefile + build outputs
docker/server/  multi-stage Dockerfile for containerised deployment
tests/          unit + smoke tests
```

## License

GPL-3.0-or-later — see [LICENSE](LICENSE).
