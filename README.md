# server-client
Linux C server-client file transfer

The server is capable of accepting requests from multiple users but only sends one file at a time.

There is no limit of file size the server-client should be capable of transfering.

The syntax:

./sock_server.o <port>
./sock_client.o <ip> <port> <filename>

The client will save the received file using the following format: received_<filename>.


