import socket
import sys
import threading
from threading import *
 
HOST = 'localhost'   # Symbolic name meaning all available interfaces
PORT = 8080 # Arbitrary non-privileged port
 
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
print('Socket created')
 
#Bind socket to local host and port
#try:
s.bind((HOST, PORT))
##except socket.error , msg:
##    print 'Bind failed. Error Code : ' + str(msg[0]) + ' Message ' + msg[1]
##    sys.exit()
     
print( 'Socket bind complete')
 
#Start listening on socket
s.listen(10)
print( 'Socket now listening')


def forward_to_ssr(request):
    HOST = 'localhost'   # Symbolic name meaning all available interfaces
    PORT = 8081 # Arbitrary non-privileged port
     
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((HOST, PORT))
    #conn, addr = s.accept()
    print( 'Connected with ' + addr[0] + ':' + str(addr[1]))
    print('forwarding'+str(request))
    #conn.send(request)
    s.sendall(request)
    response = Null
    while True:
        data = s.recv(16383) #conn.recv(16383)
        if data:
            response = data
        if not data:
            break
    return response
 
#Function for handling connections. This will be used to create threads
def clientthread(conn):
    #Sending message to connected client
    #conn.send('Welcome to the server. Type something and hit enter\n') #send only takes string
     
    #infinite loop so that function do not terminate and thread do not end.
    reply = ''
    request = None
    while True:
         
        #Receiving from client
        data = conn.recv(1024)
        if data:
            request = data
        if not data: 
            break
    reply = forward_to_ssr(request)
    conn.sendall(reply)
    #came out of loop
    conn.close()
 
#now keep talking with the client
while 1:
    #wait to accept a connection - blocking call
    conn, addr = s.accept()
    print('Connected with ' + addr[0] + ':' + str(addr[1]))
     
    #start new thread takes 1st argument as a function name to be run, second is the tuple of arguments to the function.
    t = threading.Thread(target=clientthread,args=(conn,))
    t.start()
 
s.close()
