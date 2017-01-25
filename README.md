# Proxy-Server
Designed a Proxy Webserver that communicated through TCP socket with the Client on Linux platform and could handle multiple clients in C. Caching feature with timeout was implemented for speedy retrieval of webpages.

Programming Assignmment 4

Aim: To build a simple web proxy server that is capable of accepting HTTP 1.0 requests from clients, 
     pass them to HTTP Server and handle returning traffic from the HTTP server back to clients.

Implementation:

The proxy server was written in c Code. It takes 2 arguments, first one is the port number the proxy 
listens to and second one is the timeout for caching.

Two sockets were created:
Socket 1: Communication between Client (Web Browser) and Proxy Server
Socket 2: Communication between Proxy Server and HTTP Sever

The GET request from the Client is recieved on the proxy server. It is then parsed and the host to 
connected to is identified. A new HTTP request is constructed and sent to HTTP Server. The proxy will
send back the reply from the HTTP server back to the HTTP CLient using the orignal socket connecton 
with the HTTP Client,

Caching:

In order to implement caching with a timeout value, I first calculated the MD5 sum of the entre URL.
Then, I created a '.html' file containing the contents of the webpage. The time of of last access of 
this file is stored. whenever the proxy server receives a GET request, it would check if the cached
copy is present and timeout has not expired then it drectly sends the cached copy to the HTTP Client.
If the timer has expired, it fetches the requested URL from the HTTP server and caches the file. 

Video Link: https://www.youtube.com/watch?v=kZqxn4LPT3o

