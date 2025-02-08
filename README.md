# np-assignment
This demonstrates a basic understanding of socket programming in C by implementing a simplified version of a torrent file exchange scenario. The client and server programs will simulate the process of uploading and downloading files, mimicking the behavior of a torrent system.

File name: ReadMe.txt </br>
Author: Damith Induranga Bandara </br>
Module: Network Programming

This file contains</br>
- steps to compile and run the Server and Client programs.
- inputs that can be given to the programs.
- limitations of the programs.

Server file name: srv7996.c</br>
Client file name: cli2216.c



Steps to compile the programs</br>
-  Navigate to the location where the program/s are saved.
- Enter the given commands in to the terminal:
    - Server program
        gcc srv7996.c -o svr; chmod +x svr
    - Client program
        gcc cli2216.c -o cli; chmod +x cli
- Note that you can enter your preferred name after -o, that is, for svr/cli.
- Permissions required to execute the compiled files are given with the above commands.



Steps to run the programs</br>
- Find the IPv4 address for both server and client/s by entering the given command in to the terminal:
        ip a | grep inet
- This is due to the automatic IP address assignment by DHCP.
- Navigate to the location where the program/s are saved.
- Enter the given commands to execute the programs
    - Server program</br>
        ./svr server-ipv4-address
    - Client program</br>
        ./cli client-ipv4-address server-ipv4-address</br>
(svr/cli: complied file name & server-ipv4-address: IPv4 address of the server & client-ipv4-address: IPv4 address of the client)



Inputs for Server program</br>
- The server program will indicate 
    - which clients are connected.
    - when a file is uploaded by the client.
    - where the uploaded file is saved.

Inputs for Client program</br>
- The user can enter
    - whether to upload or download files.
    - which file to download from the available files.
    - which file to upload from files in the client's files.
- The client program will indicate
    - when connected to the server.
    - files available  in the server.
    - download progress and where the file was saved to.



Note that all the files/folders will be created in the same directory as of the server/client programs.</br>
    - complied executable files
    - shared_files directory: holds the files that can be shared (upload/download)
    - log file in server



Limitations</br>
- Only 16 clients can be handled simultaneously by the server.
- To test file download resuming, very large files need to be used.
- This is due to PCIe 4th generation SSDs that is being used in the test PC.
- Resuming interrupted download works only from Server to Client file transfers (downloads).
- It doesn't work from Client to Server since file size is not shared from Client to Server (uploads).
