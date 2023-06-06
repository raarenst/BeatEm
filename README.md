# BeatEM how-to

## Server
```
> docker pull raarenst3/bruno_server:latest
> docker run -dp 27015:27015 raarenst3/bruno_server
```
## Clients
```
> bruno_client.exe 8419 13219 32111 1919 5141 [127.0.0.1]
> bruno_client.exe 4607 1919 5141 13219 32111 [127.0.0.1]
```

# BeatEm background
BeatEm is a chat system with the intention of beeing private. While many chats services can provide encrypted messages, it is still possible to map networks and who is speaking with who. Even onion routing and other attemtps can with sufficient resources be traced.

With BeatEm it is much harder, maybe impossible, both to read contents and to find out who is talking with who. It is done with a simple heartbeat protocol. Each chat client sends packages with a fixed size with regular intevals to a server. The packages containing correspondence are encrypted with the receivers public key and have that key in the header of the message. If there is no text to send (noone is typing for the moment) the client still sends packages but with random content. The central server puts all client packages together and sends them to all clients. Each client decrypts all messages from the server and looks for its own public key in the header. If it finds the key, the message is for this client.

It is simple but indeed wasteful where most communication does not include any information. This is the price for privacy. The overhead is still small compared to streaming sound for example.

BeatEm is made to secure the United Nations Universal Declaration of Human Rights article 12.

```
Article 12
No one shall be subjected to arbitrary interference with his privacy, 
family, home or correspondence, nor to attacks upon his honour and 
reputation. Everyone has the right to the protection of the law against 
such interference or attacks.
```
