# UdpChat - server and example of client for online chat


#Build

gcc -Wall -o server server.c

gcc -Wall -o client -pthread client.c


#Example

./server 1500

./client [your_host_name] 1500 alexandr

./client [your_host_name] 1500 mihail
