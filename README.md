## http server written in C

This is a simple implementation of a http server written in C. It is not a full implementation of the HTTP/1.1 protocol, but it is enough to serve get requests and post data.
I have tried to make this a multi-threaded server for handling multiple requests at the same time. This server can parse the incoming http request and send the appropriate response.

### How to run the server?

1. clone the repository:
  
   ```git clone https://github.com/crl5h/http-server.git```
   
2. run the following command in the terminal:
   
   ```make run```
   
3. post data to the server:
   
   ```curl -X POST http://localhost:4221/files/test.txt -d "testing post req!"``` or run ```bash test.sh post```
   
4. get data from the server:

   ```curl -i -X GET http://localhost:4221/files/test.txt``` or run ```bash test.sh get```
   
5. checking concurrent requests:
   ```make test```
   (dont forget to enable sleep in the server code while testing. sleep(1) mimics an CPU intensive task) 

routes:
   1. GET /files/*
   2. POST /files/*