# UdpIntervalSender
Tool for testing of UDP-receive-functions. (Only Windows with MinGW at the moment)

It sends an identifier (0xABCDEF12), followed by an incrementing message counter, followed by the amount of bytes following.
You get the actual interval displayed as an average of the last 5 messages.

Syntax:
UdpIntervalSender.exe [dest_ip dest_port interval packetsize]
