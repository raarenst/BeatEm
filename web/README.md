# BeatEm web client

Static HTML + libsodium.js. Speaks the same WebSocket protocol as the
CLI client.

## Run

The server listens on plain `ws://` (TLS would be added via a reverse
proxy in production), so the page just needs to be loaded in a browser.
The simplest way is to serve `index.html` over HTTP from this directory:

```
cd web && python3 -m http.server 8080
```

Then open `http://localhost:8080/` in a browser.

Some browsers refuse `ws://` from a `file://` origin, which is why a
local HTTP server is recommended.

## Use

1. Click **Generate new keypair** (or paste hex keys from `beatem_keygen`).
2. Share your **public key** with your peer out-of-band.
3. Paste their public key into **Peer's public key**.
4. Click **Connect**.

After the protocol handshake the chat UI appears. Messages you send
become `me: …` lines; messages from your peer appear as `peer: …`.
The client emits one packet every `heartbeat_ms` regardless of whether
you've typed anything (real or random padding) — that's what hides
who-is-talking-to-whom from a network observer.
