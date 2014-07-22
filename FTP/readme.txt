"""
---------------------------------------------------------------------------
ECEN 602 : HW2 : TFTP Server implementation

Run the Server code by passing arguments: <serverIP> <serverPort>

The TFTP server will be waiting for the clients to make file requests

To run the client code, Connect to the server via TFTP client by passing <hostname> <portno> Choose the mode (octet/ascii). 

Use "get <filenameonserver> <newfile>" to get any file on the server to be stored as newfile.

---------------------------------------------------------------------------
What the code does:

* Checks if the requested file is available on the server side. Else sends an error message to the client: "File not found".
* Checks if the requested file size is less than 32MB. Else sends error message "File size exceeding the limit".
* Multiple clients are handled paralelly using "fork". Hence two clients can request the same file.
* To check for the packet loss condition we wait for acknowledgement from the client side before transmitting the next packet. Timeout feature is implemented to handle the cases where the acknowledge packet might be lost.
* Ultimately we can transfer any file less than 32MB the server has access to.
---------------------------------------------------------------------------
Project by:
Bolun Huang and Sriharsha Madala
---------------------------------------------------------------------------
"""
