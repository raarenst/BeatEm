# BeatEm
BeatEm is a chat system with the intention of beeing private. While many chats services can provide encrypted messages, it is still possible to map networks and who is speaking with who. Even onion routing and other attemtps can with sufficient resources trace who is talking with who.

With BeatEm it is much harder, maybe impossible, both to read contents of what is communicated and to disclose who is talking with who. It is done with a simple heartbeat protocol. Each chat client sends packages with a fixed size with regular intevals to a server. The packages containing correspondence are encrypted with the receivers public key and have the receivers public key first in the message. If there is no text to send the client still sends a package but with random content. The central server puts all client packages together and sends them to all clients. Each client decrypts all messages from the server and looks for its public key in the header. If it finds the key, the message is for this client.

It is simple but indeed wasteful with most communication does not include any information. This is the price for security.

BeatEm is made to secure the United Nations Universal Declaration of Human Rights article 12.

```
##Article 12
No one shall be subjected to arbitrary interference with his privacy, 
family, home or correspondence, nor to attacks upon his honour and 
reputation. Everyone has the right to the protection of the law against 
such interference or attacks.
```
