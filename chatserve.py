#!/usr/bin/python

# File name: chatserve.py
# Author: Christopher Kirchner
# Class: CS362_SEII
# Date: 10/30/2016
# Description: chat server that hosts chat client communication using TCP connection


import struct
from socket import *
import argparse
import sys
import ctypes
import SocketServer
import threading


MAXDATASIZE = 512
MAXHANDLESIZE = 11

# Name: send_num
# Description: send 4-byte unsigned int through socket

def send_num(num, sock):
    return sock.send(struct.pack('!I', num))

# Name: get_num
# Description: receive 4-byte unsigned int from socket

def get_num(sock):
    #https://docs.python.org/2/library/struct.html
    return struct.unpack('!I', sock.recv(4))[0]

# Name: send_msg
# Description: sends message through socket with msg size first

def send_msg(msg, sock):
    send_num(len(msg), sock)
    sock.sendall(msg)

#http://stackoverflow.com/questions/6715944/non-blocking-socket-in-python

# Name: get_msg
# Description: receives message from socket with msg size first

def get_msg(sock):
    msg_size = get_num(sock)
    msg = ''
    # get message from socket in chunks if needed
    while msg_size > 0:
        data = sock.recv(msg_size)
        msg_size -= len(data)
        msg += data
    return msg

# STARTS UP SERVER #

# Name: get_listening_socket
# Description: setup and return bound and listening server socket

def get_listening_socket(port):
    sock = socket(AF_INET, SOCK_STREAM)
    # reuse address to obviate pesky reuse msg
    sock.setsockopt(SOL_SOCKET, SO_REUSEADDR, 1)
    sock.bind(('', port))
    sock.listen(5)
    return sock

#Citation - https://pymotw.com/2/threading/

# Name: ChatServer
# Description: thread class for threaded chat server

class ChatServer(threading.Thread):

    def __init__(self, sock, addr, lock):
        threading.Thread.__init__(self)
        self.sock = sock
        self.addr = addr
        self.server_handle = self.getName()
        self.output = lock
        self.client_handle = self.setup()

    # setup connection with client
    # exchange handles
    def setup(self):
        with self.output:
            print "Connection from {}".format(self.addr)
        client_handle = get_msg(self.sock)
        send_msg("{}".format(self.server_handle), self.sock)
        return client_handle

    # run chat communication with client
    def run(self):
        end = False
        while not end:
            # get message from client
            msg = get_msg(self.sock)
            # exit if client quits
            if msg == "\\quit":
                with self.output:
                    print "{} left the chat".format(self.client_handle)
                end = True
            else:
                # prepend client handle to message
                print "{}> {}".format(self.client_handle, msg)
                msg = raw_input("{}> ".format(self.server_handle))
                # exit if server thread quits
                if msg == "\\quit":
                    send_msg("\\quit", self.sock)
                    end = True
                    # utilize lock for owning stdout
                    with self.output:
                        print "Thread closed connection"
                else:
                    # send message to client
                    send_msg("{}".format(msg), self.sock)
        self.sock.close()

def main(argv):
    #Citation - https://docs.python.org/2/library/argparse.html
    # parser = argparse.ArgumentParser(description="Setup chat client")
    # parser.add_argument('-p', action='store', dest='port', type=int,
    #                     help='port number')
    # args = parser.parse_args()

    if len(argv) != 2:
        print "usage: chatserve.py port"
        exit(1)
    server_socket = get_listening_socket(int(argv[1]))
    threads = []
    # Used lock to control std output during competing thread executions
    # Citation - http://stackoverflow.com/questions/18348991/python-threading-stdin-stdout
    output = threading.Lock()
    while 1:
        with output:
            print "Server is ready to receive"
        sock, addr = server_socket.accept()
        # Using threaded class for each chat server to client communication
        # Citation - http://stackoverflow.com/questions/17453212/multi-threaded-tcp-server-in-python
        t = ChatServer(sock, addr, output)
        t.start()
        threads.append(t)

    # wait for all threads to die
    for t in threads:
        t.join()

if __name__ == "__main__":
    main(sys.argv)
