// Bandara K M D I
// Server program

// header files
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include <netdb.h>
#include <sys/uio.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/udp.h>
#include <dirent.h>
#include <signal.h>
#include <sys/wait.h>

// predefined values
#define portNum 2216
#define chunkSize 1024
#define maxClients 16
#define maxFiles 150

// custom function to handle errors
void errorCode (int resultCode, const char *msg) {
    if (resultCode < 0) {
        printf("%s error\n", msg);
        exit(1);
    }
}

// custom function to generate logs
void generateLogs (const char *cliIp, int cliPort, const char *fileName, const char *task, int isSuccess) {
    // open log file to append
    FILE *logFile = fopen("log_srv7996.txt", "a");

    if (logFile == NULL) {
        errorCode(-1, "cannot open log file");
    } else {
        // get current time stamp
        time_t seconds = time(NULL);
        char *nowTime = ctime(&seconds);

        // check if successful or not
        char *status;
        if (isSuccess == 1) {
            status = "Successful";
        } else {
            status = "Failed";
        }

        // print to log file
        fprintf(logFile, "Time:%s - Client IP:%s Port:%d - File:%s - Task:%s - Status:%s\n\n", nowTime, cliIp, cliPort, fileName, task, status);
    }

    // close the file
    fclose(logFile);
}

// custom function to handle clients
void handleClient (int cliFd, struct sockaddr_in client) {
    // variables
    size_t len;
    int count = 1, rslt = 0, fileNum = 1, isSuccess = 0, bytes = 0, bytesReceived;
    char buffer[chunkSize], fileIndex[5], task[10], fileList[4096], filePath[512], resume[64], fileName[128], fileSize[64];
    DIR *directory;
    struct dirent *entry;
    struct stat fileInfo;
    off_t size;
    long resumePosition;
    double percentage = 0.0;

    // set all arrays to NULL
    bzero(&buffer, sizeof(buffer));
    bzero(&task, sizeof(task));
    bzero(&fileList, sizeof(fileList));
    bzero(&filePath, sizeof(filePath));
    bzero(&fileIndex, sizeof(fileIndex));
    bzero(&fileName, sizeof(fileName));

    // 7
    // print client details
    printf("Client connection from %s:%d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));

    // 10
    // get task from client and copy it to task
    rslt = read(cliFd, task, sizeof(task));
    errorCode(rslt, "2.handleClient -> recv()");

    // upload to client
    if (strcmp(task, "download") == 0) {
        // 11
        // handle specific directory
        directory = opendir("shared_files");
        if (directory == NULL) {
            errorCode(-1, "3.handleClient -> Cannot open 'shared_files' directory");
        }

        // 12
        // loop over all files in the directory
        while ((entry = readdir(directory)) != NULL) {
            // set all bytes of file path array and contruct full path to file
            bzero(&filePath, sizeof(filePath));
            snprintf(filePath, sizeof(filePath), "shared_files/%s", entry->d_name);

            // retrieve metadata and check if it is a file and not a directory
            if (stat(filePath, &fileInfo) == 0 && S_ISREG(fileInfo.st_mode)) {
                // ignore hidden files
                if (entry->d_name[0] != '.') {
                    //  append the file name to file list if it is a file
                    len = strlen(fileList);
                    sprintf(fileList + len, "%d", fileNum);
                    strcat(fileList, ". ");
                    strcat(fileList, entry->d_name);
                    strcat(fileList, "\n");
                    fileNum++;
                }
            }
        }
        // close directory
        closedir(directory);

        // 13
        // send file list to client
        rslt = send(cliFd, fileList, strlen(fileList), 0);
        errorCode(rslt, "4.handleClient -> send()");

        // 18
        // receive client requested file name
        rslt = read(cliFd, fileIndex, sizeof(fileIndex));
        errorCode(rslt, "5.handleClient -> recv()");
        fileNum = atoi(fileIndex);

        // 19
        // handle specific directory
        directory = opendir("shared_files");
        if (directory == NULL) {
            errorCode(-1, "3.handleClient -> Cannot open 'shared_files' directory");
        }
        // go over the files and locate the file corresponding to the given number
        while ((entry = readdir(directory)) != NULL) {
            // set all bytes of file path array and contruct full path to file
            bzero(&filePath, sizeof(filePath));
            snprintf(filePath, sizeof(filePath), "shared_files/%s", entry->d_name);

            // retrieve metadata and check if it is a file and not a directory
            if (stat(filePath, &fileInfo) == 0 && S_ISREG(fileInfo.st_mode)) {
                // ignore hidden files
                if (entry->d_name[0] != '.') {
                    // compare index with count
                    if (count == fileNum) {
                        // obtain file name
                        strcpy(fileName, entry->d_name);

                        // obtain file size
                        size = fileInfo.st_size;
                        sprintf(fileSize, "%ld", size);

                        // break the loop
                        break;                        
                    }
                    // increase count to match the file count
                    count++;
                }
            }
        }
        // close directory
        closedir(directory);

        // 20 
        // send file size to client
        rslt = write(cliFd, fileSize, sizeof(fileSize));
        errorCode(rslt, "send()");

        // 22
        // send file name to client
        rslt = write(cliFd, fileName, sizeof(fileName));
        errorCode(rslt, "send()");

        // 26
        // get resume position for the file
        rslt = read(cliFd, resume,sizeof(resume));
        errorCode(rslt, "read()");
        resumePosition = atol(resume);

        // 27
        // set full file path
        snprintf(filePath, sizeof(filePath), "shared_files/%s", fileName);

        // 28
        // open the file to upload to client
        FILE *file = fopen(filePath, "rb");
        if (file == NULL) {
            errorCode(-1, "file not found");
        } else {
            // resume from stopped position
            fseek(file, resumePosition, SEEK_SET);

            //read and send in specified chunks
            while ((bytes = fread(buffer, 1, chunkSize, file)) > 0) {
                rslt = write(cliFd, buffer, chunkSize);
                errorCode(rslt, "write()");
            }
        }
        // close file
        fclose(file);

        // 33
        // get if download was successful
        bzero(resume, sizeof(resume));
        rslt = recv(cliFd, resume, sizeof(resume), 0);
        errorCode(rslt, "8.handleClient -> recv()");
        isSuccess = atoi(resume);

        // 34
        // call generateLogs() function
        generateLogs(inet_ntoa(client.sin_addr), ntohs(client.sin_port), fileName, task, isSuccess);
    } 
    // download to server
    else if (strcmp(task, "upload") == 0) {
        // 38
        // indicate downloading from client
        printf("Waiting for client to upload a file...");

        // 39
        // get file name
        rslt = read(cliFd, fileName, sizeof(fileName));
        errorCode(rslt, "read()");

        // 40
        // set full file path
        snprintf(filePath, sizeof(filePath), "shared_files/%s", fileName);

        // 42
        // open file to write
        FILE *file = fopen(filePath, "wb");
        // handle file error and download
        if (file == NULL) {
            errorCode(-1, "fopen() -> download error");
        } else {
            // indicate downloading of file
            printf("\n\nDownloading %s from %s:%d...", fileName, inet_ntoa(client.sin_addr), ntohs(client.sin_port));

            // download file
            while ((bytesReceived = recv(cliFd, buffer, chunkSize, 0)) > 0) {
                // set variable values and 
                rslt = 0;
                rslt = fwrite(buffer, 1, bytesReceived, file);
                

                // break the loop if totalBytes equals or exceeds the file size
                if (bytesReceived > rslt) {
                    isSuccess = 1;
                    break;
                } else {
                    isSuccess = 0;
                }
            }

            // print download completion
            printf("Download complete! File saved as %s\n\n", filePath);

            // 43
            // call generateLogs() function
            generateLogs(inet_ntoa(client.sin_addr), ntohs(client.sin_port), fileName, task, isSuccess);
        }
    }
}

// main method
// argv[1] = Server IPv4 address
int main (int argc, char *argv[]) 
{
    // Ignore SIGCHLD to avoid zombie processes
    signal(SIGCHLD, SIG_IGN);

    // create required directories and ignore if they already exist
    mkdir("shared_files", 0777);
    
    // socket file descriptors
    int serverfd, cliFd;

    // variables
    int rslt = 0;    

    // server socket address structure
    struct sockaddr_in server, client;

    // sizes of socket address structures
    socklen_t svrSize = sizeof(server);
    socklen_t cliSize = sizeof(client);

    // argument error handling
    if (argc < 2) {
        errorCode(-1, "1.Please enter Server IP address");
    }

    // 1
    // create empty socket 
    // IPv4
    // TCP
    serverfd = socket(AF_INET, SOCK_STREAM, 0);
    errorCode(serverfd, "2.main -> socket()");

    // set all bits of socket address structures to 0
    bzero(&server, svrSize);
    bzero(&client, cliSize);

    // set values to server socket address structure 
    // IPv4
    // IP address = argv[1]
    // Port number = 2216
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(argv[1]);
    server.sin_port = htons(portNum);

    // 2
    // bind()
    rslt = bind(serverfd, (struct sockaddr *)&server, svrSize);
    errorCode(rslt, "3.main -> bind()");

    // 3
    // print server details
    printf("Server started on\n IP: %s\n Port: %d\n", argv[1], portNum);
    printf("Waiting for client connections ...\n\n");

    // 4
    // listen()
    rslt = listen(serverfd, maxClients);
    errorCode(rslt, "4.main -> listen()");

    // infinite loop
    for (;;) {
        // 6
        // accept()
        cliFd = accept(serverfd, (struct sockaddr *)&client, &cliSize);
        errorCode(cliFd, "5.main -> accept()");

        if (fork() == 0) {
            // close listening file descriptor
            close(serverfd);
            
            // call handleClient() function
            handleClient(cliFd, client);

            // exit child process after handling client socket
            exit(0);
        }

        // close client file descriptor
        close(cliFd);
    }

    // close listening file descriptor
    close(serverfd);

    // end of server program
    return 0;
}
