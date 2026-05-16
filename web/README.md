# BeatEm web client

Static HTML + libsodium.js. Speaks the same WebSocket protocol as the
CLI client.

## Run

`beatem_server` embeds this `index.html` at build time and serves it on
the same TCP port as the WebSocket. So you only need one process:

```
cd build_linux && ./beatem_server
# then open http://localhost:27015/ in a browser
```

No separate HTTP server is needed. (Re-running `make all` re-embeds
the latest `index.html`.)

For a public-internet deployment, point a reverse proxy (Caddy / nginx)
or a tunnel (ngrok, Cloudflare Tunnel) at port 27015 — it serves both
the page and the WebSocket. The page's JS automatically derives the
WebSocket scheme (`wss://` when the page was loaded over `https://`,
`ws://` over plain `http://`) and host (same origin), so you don't
need to edit anything to deploy.

## Use

1. Click **Generate new keypair** (or paste hex keys from `beatem_keygen`).
2. Click **Copy** (or **Copy pubkey** in the top bar after connecting)
   and share your public key with your peers out-of-band.
3. Click **Connect**.
4. In the chat screen, paste each peer's public key into the "Peer
   public key" field, give them a label (e.g. "Alice"), and click
   **Add**. You can add as many as you want; all peers are listened
   to simultaneously over the single WebSocket connection.
5. Click a peer in the left list to focus their thread. Messages you
   send are encrypted to that peer; incoming messages route into the
   thread of whichever peer's key successfully decrypts them. Unread
   counts appear next to non-active peers.

The client emits one packet every `heartbeat_ms` regardless of whether
you've typed anything (real or random padding) — that's what hides
who-is-talking-to-whom from a network observer.

Peer labels and public keys are saved in `localStorage`, so they
survive page reloads. Your **secret key is not saved** — paste or
regenerate it on each reload. (Browser-stored secret keys are exposed
to any XSS, extension, or shared-device access.)

Each peer entry shows a short **fingerprint** (first 8 hex chars of
their public key). Read these to each other over voice to verify you
have the right key — this is the same idea as Signal's "safety
numbers" or SSH's host-key fingerprint.
