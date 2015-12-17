# DuckChat
A chat program created for a CIS 422 class at the University of Oregon. Upgraded to support multiple servers and users across those servers.

USAGE:

Use a 'make' command to build the project.

Invoke a server on a <host> and <port> with ./server <host> <port>. Any host port pairs after the first pair indicate other servers to whom
this server is connected to.  i.e. ./server <host> <port> <host1> <port1> <host2> <port2> connects the server on <host>:<port> to the servers
on <host1>:<port1> and <host2>:<port2>. This program works and runs in both the single server and multiserver mode.

Invoke a client with ./client <host> <port> <username>, with host and port being the server that you want to connect to. Every client starts
on the default 'Common' channel.

Send messages by typing and pressing enter. Special commands are demarcated with a slash before the command:
/join <channel>: joins the existing channel or creates it if it doesn't exist.
/leave <channel>: leaves the channel and destroys it if there are no longer an users.
/switch <channel>: switch to a different channel that you are already subscribed to in order to speak on that channel.
/list: lists all available channels to join (INCOMPLETE FOR MULTISERVER)
/who <channel>: shows names of who is on the channel indicated (INCOMPLETE FOR MULTISERVER)
/exit: log off
