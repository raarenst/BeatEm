# BeatEm — Users' Guide

A walk-through for getting two or more people chatting on BeatEm,
from "I just cloned the repo" to "we're talking over the internet."

The protocol is end-to-end encrypted (libsodium `crypto_box`) **and**
traffic-analysis resistant: every client emits a fixed-size packet on
a fixed schedule (real message or random padding — indistinguishable
to anyone watching the wire), and the server broadcasts the union of
all client packets to every connected client. That means a passive
observer can't tell *who is talking to whom*, only that someone is
using the service.

---

## 1. Prerequisites

You need a Linux box with the build toolchain and `libsodium`:

```sh
sudo apt install build-essential libsodium-dev xxd
```

Then build everything:

```sh
cd build_linux
make all          # builds beatem_server and beatem_client
make test         # optional: 60 unit + 2 smoke tests
```

You should now have two binaries in `build_linux/`:

```
beatem_server   # the broadcast hub
beatem_client   # the terminal chat client
```

Plus an installable web client (PWA) served by `beatem_server`
itself — no separate web server needed.

---

## 2. Start the server

In one terminal:

```sh
cd build_linux
./beatem_server
```

You should see:

```
BeatEm server listening on port 27015 (heartbeat=2000 ms, packet=128 B, max_clients=16)
```

The defaults are fine for most uses. To tweak them you can use CLI
flags, a config file, or both (CLI overrides the file):

```sh
./beatem_server --help                       # show all flags
./beatem_server --port 9000                  # listen on a different port
./beatem_server --max-clients 32 --packet-size 256 --heartbeat-ms 1500
./beatem_server --config beatem_server.conf  # read from a file
./beatem_server --config beatem_server.conf --port 9000  # file + override
```

Config-file format (see `beatem_server.conf.example` in the repo
root): one `key = value` per line, `#` for comments. Keys match the
long-option names exactly:

```
heartbeat-ms = 2000
packet-size  = 1024
max-clients  = 100
port         = 27015
```

Clients pick up whatever the server is configured for via the
handshake — they never need to be told the numbers separately, and
they never need to be rebuilt when you change server flags.

Leave this terminal open. The server prints connection events and
errors here.

---

## 3. Make the server reachable

How clients reach the server depends on where they are.

### 3a. Same machine (testing)

Nothing extra. The CLI client defaults to `127.0.0.1`, and the web
client at `http://localhost:27015/` connects to the same host.

### 3b. Same LAN

Find the server host's LAN IP (`ip a`, usually `192.168.x.x` or
`10.x.x.x`). Other devices on the network reach the server at
`<that-ip>:27015`.

### 3c. Public internet — quickest path, ngrok

ngrok gives you a public `https://...` URL that tunnels back to your
local `beatem_server`, with TLS handled at ngrok's edge. Great for
demos and small groups.

**One-time setup:**

1. Sign up at <https://ngrok.com> and copy your authtoken.
2. Install ngrok:
   ```sh
   sudo snap install ngrok
   # or download the binary from ngrok.com/download
   ```
3. Tell ngrok who you are:
   ```sh
   ngrok config add-authtoken <your-token>
   ```

**Each session:**

In a second terminal, while `beatem_server` is running:

```sh
ngrok http 27015
```

ngrok prints something like:

```
Forwarding   https://abc123.ngrok-free.dev -> http://localhost:27015
```

Share that `https://abc123.ngrok-free.dev` URL with the people you
want to chat with. They open it in their browser; the web client
auto-derives the WebSocket URL from the page origin (so `https://...`
→ `wss://...`) — no manual server entry needed.

> ⚠ Free-tier limits: the subdomain changes every restart, sessions
> end after ~2 hours, ~1 GB/month bandwidth. Fine for trying it out;
> for a permanent setup, see 3d.

### 3d. Docker

A multi-stage `Dockerfile` at `docker/server/Dockerfile` builds the
server from source inside the image — no host build step needed.
From the repo root:

```sh
docker build -f docker/server/Dockerfile -t beatem-server .
docker run --rm -p 27015:27015 beatem-server                          # defaults
docker run --rm -p 27015:27015 \
  -v "$PWD/beatem_server.conf:/etc/beatem/server.conf:ro" \
  beatem-server --config /etc/beatem/server.conf                      # with config
```

For multi-arch (amd64 + arm64):

```sh
docker buildx build --platform linux/amd64,linux/arm64 \
  -f docker/server/Dockerfile -t beatem-server .
```

### 3e. Public internet — VPS + reverse proxy

For a permanent deployment, run BeatEm behind Caddy or nginx on a
small VPS. See the deploy section at the bottom of `CLAUDE.md` for
the Caddyfile shape; it's roughly five lines.

---

## 4. Connect with the CLI client

The CLI client needs three things:

| Argument          | Meaning                                                |
|-------------------|--------------------------------------------------------|
| `my_secret_hex`   | Your **private** X25519 key — 64 hex chars, keep secret |
| `my_public_hex`   | Your **public** key — 64 hex chars, share with peers   |
| `peer_public_hex` | One peer's public key                                  |

The CLI is single-peer per invocation — for multiple peers, use the
web client (see §5).

### 4a. Generate a keypair

A test helper is built alongside the binaries:

```sh
cd build_linux
./beatem_keygen
```

It prints two keypairs (so you can hand one to a friend, keep one for
yourself):

```
KEYPAIR_0_SK=c0a9f179...
KEYPAIR_0_PK=16576410...
KEYPAIR_1_SK=5c7ce740...
KEYPAIR_1_PK=8f42d082...
```

Tell your peer your **public key** (`PK`). Keep your **secret key**
(`SK`) to yourself.

### 4b. Run the client

```sh
./beatem_client <my_sk> <my_pk> <peer_pk>          # localhost
./beatem_client <my_sk> <my_pk> <peer_pk> 192.168.1.42       # LAN
./beatem_client <my_sk> <my_pk> <peer_pk> abc123.ngrok-free.dev:443   # ngrok
```

(Use `host:port` if the server is on a non-default port.)

The client prints the negotiated cadence and a `>>` prompt. Type any
line and press Enter to send. Incoming messages from your peer appear
as:

```
         ---(hello!)---
>>
```

Special commands:

- `&genkeys` — generate a new keypair on the fly. **You'll need to
  re-share the new public key with your peer**, or they won't decrypt
  your messages anymore (and vice versa).
- `Ctrl-D` — quit.

---

## 5. Connect with the web / PWA client

The web client is **multi-peer**: one connection, one chat per peer,
all listening simultaneously.

### 5a. Open the page

Browse to whichever URL fits your setup:

| Setup              | URL                                |
|--------------------|------------------------------------|
| Same machine       | `http://localhost:27015/`          |
| Same LAN           | `http://<server-ip>:27015/`        |
| ngrok              | `https://abc123.ngrok-free.dev/`   |
| Production (VPS)   | `https://your-domain/`             |

There is no setup screen — the page lands you directly in the chat.
On first visit, a keypair is generated for you and saved in the
browser's `localStorage`; on return visits the same identity is
loaded. The WebSocket also connects automatically and auto-reconnects
after 3 s if the server drops.

### 5b. Share your public key

The topbar shows your **fingerprint** (first 8 hex chars of your
public key). The buttons next to it:

- **Copy** — copies your full 64-char public key to the clipboard.
- **QR** — opens a modal showing your public key as a QR code.
- **Regen** — generates a brand-new keypair (asks for confirmation;
  invalidates your old key — peers will need the new one).

Send your public key to a peer over an out-of-band channel (Signal,
email, in person, paper). For phone↔laptop in the same room, the
laptop's QR + the phone's **Scan** button (next to the peer-key input)
is the fastest path.

> ⚠ Your **secret** key lives in `localStorage` in plain form. That's
> fine for a single-user privacy app but means any browser extension
> with page access can read it, and anyone with hands-on access to
> your unlocked browser can too. To wipe everything, see §5g.

### 5c. Add peers

The peer-list sidebar (left on desktop, top on mobile) is where peer
public keys live. To add someone:

1. Paste their public key into **Pubkey** (or hit **Scan** to read a
   QR code from their device).
2. Give them a label (e.g. "Alice") in the label field.
3. Click **Add**.

Their entry appears with their 8-char **fingerprint** underneath. The
fingerprint is the same idea as Signal's safety numbers: read it over
the phone to confirm you have the right key (no man-in-the-middle).

Repeat for as many peers as you want. All of them are listened to
simultaneously — incoming messages route into whichever peer's chat
their key decrypts.

**Per-peer actions:**
- *Copy pk* — copy their public key back to your clipboard.
- *Remove* — drop them from your list.

On desktop these appear on hover; on touch devices they're always
visible.

Peer labels and pubkeys persist across page reloads. To clear them
along with your identity, see §5g.

### 5d. Chat

Click a peer in the sidebar to focus that conversation. The chat
area shows the history; the input box at the bottom sends to the
selected peer. Switch peers any time — each thread is independent,
unread counts appear in parens next to peers when you're focused
elsewhere.

### 5e. Install as a phone app (PWA)

On a phone:

1. Open the page in mobile Safari (iOS) or Chrome (Android).
2. Tap the browser menu → **Add to Home Screen** (iOS) or
   **Install app** (Android).
3. The BeatEm icon appears on your home screen. Tapping it opens
   fullscreen, no browser chrome.

> ⚠ Mobile OSes suspend backgrounded apps. While the app is
> on-screen and unlocked, real-time chat works. As soon as you lock
> the screen or switch apps, **cover traffic stops and you stop
> receiving** — by design, because the alternatives (push
> notifications, persistent foreground services) would leak the
> exact metadata the protocol exists to hide. Think of BeatEm on
> mobile as *"open the app when I want to chat"* rather than
> always-on messenger.

### 5f. The "W" warning panel

A small amber **W** button sits next to the heading. Tap it to expand
a panel showing:

- **Server you are trusting** — the host you're connected to and the
  WebSocket scheme (`ws://` or `wss://`).
- **Negotiated parameters** — current cadence, slot size, max-clients,
  per-message budget, and up/down byte rates.
- **What the app protects** — end-to-end encryption + constant-cadence
  cover traffic.
- **What the app does NOT protect against** — untrusted servers,
  Sybil flooding (an adversary can connect any number of fake
  clients), endpoint compromise.

Read it once when you set things up; it explains the threat model
plainly.

### 5g. Wipe local data

Inside the W panel, at the bottom, is a red **Wipe local data**
button. It deletes your secret key, public key, and peer list from
`localStorage`, then reloads the page (which immediately generates
a fresh keypair). Use it on a shared device, after you suspect key
material has leaked, or just to start over.

To wipe by hand: in DevTools → Application → Local Storage → delete
`beatem.sk`, `beatem.pk`, `beatem.peers`. Chat history is in memory
only and goes away on any reload.

---

## 6. Verifying you have the right peer key

The point of asking your peer for their public key out-of-band is
to make sure no man-in-the-middle swapped it for their own. Two ways
to verify:

- **Fingerprint over voice.** Read the first 8 hex chars of their
  pubkey to each other on a phone call you trust. Match? Real.
- **Pubkey on a trusted channel.** If you got the pubkey via Signal,
  in person, or printed on a business card, you already trust it.

If a peer's chat suddenly stops working (decryption fails on every
broadcast), either they regenerated their keypair (CLI `&genkeys` /
web **Regen** button) and didn't tell you, or something else is
wrong with the key. Ask them on a different channel.

---

## 7. Common gotchas

| Symptom                                          | Cause / fix                                                                 |
|--------------------------------------------------|------------------------------------------------------------------------------|
| `WebSocket connection failed`                    | `beatem_server` isn't running, or wrong port. Check `ss -ltn \| grep 27015`. |
| Topbar says "Disconnected. Reconnecting…"        | Same as above — the WebSocket can't reach the server. The page auto-retries every 3 s. |
| Page won't update after a code change            | Hard-refresh (Cmd/Ctrl+Shift+R). The service worker caches assets aggressively for offline use. |
| Browser shows `chrome-error://...`               | Wrong URL in the address bar (typed `27015` when the page is on a different port, etc.). |
| `Could not read handshake: -1` in the CLI        | Server is reachable but isn't the latest binary, or proxy ate the upgrade. Rebuild and retry. |
| Messages from peer don't arrive                  | Counter desync after a reboot; either keep the recipient running or wait a few seconds for the wall-clock-derived counter to catch up. |
| Link from Messenger / Instagram doesn't work     | In-app browsers often block WebSockets, persistent cookies, and PWA installs. Open the link in the system browser (Chrome / Safari) instead. |

---

## 8. What the protocol *doesn't* hide

Even when everything works:

- A passive observer can tell that you're connected to the server
  (IP-to-IP), and that BeatEm-shaped traffic is flowing. Tor onion
  service deployment hides both — see CLAUDE.md.
- The server operator sees your IP. Use Tor if that matters.
- Your secret key, public key, and peer list all live in
  `localStorage`. Anything that can run JS in the browser session
  (XSS, page-level extensions) can read them. Use **Wipe local data**
  (W panel) to clear them.
- A Sybil flood: a single party can connect many fake clients at near-
  zero cost. They then know which broadcast slots they didn't author,
  narrowing the real conversation. This is why the UI does not display
  a "connected client count" indicator — any such number can be
  manufactured.

The protocol is built for *traffic-analysis resistance*, not full
anonymity. For the latter, run the server as a Tor hidden service.
