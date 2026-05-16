# BeatEm how-to

## Server
```
> docker pull REDACTED/bruno_server:latest
> docker run -dp 27015:27015 REDACTED/bruno_server
```

## Clients

Each client needs an X25519 keypair (32 bytes each, hex-encoded). Generate
keypairs out-of-band and share the public keys with your peer.

```
> beatem_client.exe <my_secret_hex> <my_public_hex> <peer_public_hex> [server_ip]
```

The two ends are launched with reciprocal keys so each can decrypt the other.

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
