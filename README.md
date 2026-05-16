# BeatEm

A privacy-preserving chat protocol whose goal is not just *what you
say* but *who you talk to*: every client emits fixed-size packets on a
fixed schedule (real or random padding), and the server broadcasts
the union to everyone, so a network observer can't tell who's talking.

End-to-end encryption is libsodium `crypto_box` (X25519 + XSalsa20 +
Poly1305) with monotonic replay-counter protection. Cover packets are
indistinguishable from real ciphertext on the wire.

## Three clients, one protocol

- **Linux CLI** — `beatem_client` (`source/beatem_client.c`)
- **Web / PWA** — `web/index.html` (vanilla JS + libsodium.js, served
  by `beatem_server` itself on the same TCP port as the WebSocket;
  installable to a phone home screen)
- The Windows CLI is currently broken pending a winsock port.

## Get started

See **[USERS_GUIDE.md](USERS_GUIDE.md)** for the full walk-through:
prerequisites, starting the server, exposing it via ngrok / LAN /
public internet, connecting with the CLI, the web, the PWA, and
adding peers.

For developers, **[CLAUDE.md](CLAUDE.md)** has the build instructions,
architecture overview, and conventions.

# BeatEm background
BeatEm is a chat system with the intention of beeing private. While many chats services can provide encrypted messages, it is still possible to map networks and who is speaking with who. Even onion routing and other attemtps can with sufficient resources be traced.

With BeatEm it is much harder, maybe impossible, both to read contents and to find out who is talking with who. It is done with a simple heartbeat protocol. Each chat client sends packages with a fixed size with regular intervals to a server. The packages containing correspondence are encrypted with the receivers public key and authenticated with the sender's secret key. If there is no text to send (no one is typing for the moment) the client still sends packages but with random content. The central server puts all client packages together and sends them to all clients. Each client tries to decrypt every slot in the broadcast; only the slot intended for that client (and authored by its expected peer) will authenticate, and only that one is printed. Cover (random) packets fail authentication and are silently dropped.

It is simple but indeed wasteful where most communication does not include any information. This is the price for privacy. The overhead is still small compared to streaming sound for example.

BeatEm is made to secure the United Nations Universal Declaration of Human Rights article 12.

```
Article 12
No one shall be subjected to arbitrary interference with his privacy, 
family, home or correspondence, nor to attacks upon his honour and 
reputation. Everyone has the right to the protection of the law against 
such interference or attacks.
```
