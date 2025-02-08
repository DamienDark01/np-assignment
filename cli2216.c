// Bandara K M D I
// Client Program

// header files
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <errno.h>
#include <netdb.h>
#include <sys/uio.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/udp.h>
#include <sys/stat.h>

// predefined values
#define svrPort 2216
#define cliPort 7996
#define chunkSize 1024
#define maxFiles 150

// custom method to handle errors
void errorCode (int resultCode, const char *msg) {
    if (resultCode < 0) {
        printf("Error : %s\n", msg);
        exit(1);
    }
}

// custom function to get the file size
long getFileSize (const char *fileName) {
    // variables
    char filePath[512];
    long size;

    // set file path
    snprintf(filePath, sizeof(filePath), "shared_files/%s", fileName);
    
    // open the file
    FILE *file = fopen(filePath, "rb");
    if (file == NULL) {
        size = 0;
    } else {
        // move the file pointer to the end of the file and obtain the size
        fseek(file, 0, SEEK_END);
        size = ftell(file);

        // close the file
        fclose(file);
    }

    // return the size
    return size;
}

// custom function to download files
void downloadFile (int cliFd) {
    // variables
    FILE *file;
    double percentage = 0.0;
    long resumePosition = 0, size = 0;
    int result = 0, fileNum = 0, bytesReceived, totalBytes, isSuccess = 0;
    char buffer[chunkSize], message[512], fileList[4096], fileIndex[5], fileName[128], filePath[512], fileSize[64], resume[64];

    // set all arrays to 0
    bzero(&buffer, sizeof(buffer));
    bzero(&message, sizeof(message));
    bzero(&fileList, sizeof(fileList));
    bzero(&fileName, sizeof(fileName));
    bzero(&fileSize, sizeof(fileSize));
    bzero(&fileIndex, sizeof(fileIndex));
    bzero(&resume, sizeof(resume));
    bzero(&filePath, sizeof(filePath));

    // 14
    // receive file list from server
    result = read(cliFd, fileList, sizeof(fileList));
    errorCode(result, "7.main -> read()");

    // 15
    // print to display available files
    printf("Available files : \n");
    printf("%s\n", fileList);

    // 16
    // prompt to enter file name
    printf("Enter the file number to download : ");
    scanf("%d", &fileNum);
    // handle errors
    while (fileNum > maxFiles || fileNum <= 0) {
        printf("Wrong file number! Try again!\n");
        printf("Enter the file number to download : ");
        scanf("%d", &fileNum);
    }

    // 17
    // send fileNum to server to get the file size
    sprintf(fileIndex, "%d", fileNum);
    result = write(cliFd, fileIndex, sizeof(fileIndex));
    errorCode(result, "8.main -> send()");

    // 21
    // get file size
    result = recv(cliFd, fileSize, sizeof(fileSize), 0);
    errorCode(result, "9.main -> recv()");
    size = atol(fileSize);

    // 23
    // get the file name
    result = recv(cliFd, fileName, sizeof(fileName), 0);
    errorCode(result, "10.main -> recv()");
    
    // display file details
    printf("File name : %s\n", fileName);
    printf("File size : %ld bytes\n", size);
    
    // 24
    // check if the file is a partially downloaded file
    resumePosition = getFileSize(fileName);
    totalBytes = resumePosition;

    // 25
    // send resume position to server
    snprintf(resume, sizeof(resume), "%ld", resumePosition);
    result = write(cliFd, resume, sizeof(resume));
    errorCode(result, "11.main -> write()");

    // 29
    // set full file path
    snprintf(filePath, sizeof(filePath), "shared_files/%s", fileName);

    // 30
    // open file to append or to write
    if (resumePosition > 0) {
        file = fopen(filePath, "ab");
    } else {
        file = fopen(filePath, "wb");
    }

    // 31
    // handle file error and download
    if (file == NULL) {
        errorCode(-1, "fopen() -> download error");
    } else {
        // download file
        printf("\nDownloading %s ...\n", fileName);
        while ((bytesReceived = recv(cliFd, buffer, chunkSize, 0)) > 0) {
            fwrite(buffer, 1, bytesReceived, file);
            totalBytes += bytesReceived;

            // calculate percentage
            percentage = ((double)totalBytes / (double)size) * 100.00;
            printf("\rProgress: %.2f%%", percentage);
            fflush(stdout);

            // break the loop if totalBytes equals or exceeds the file size
            if (totalBytes >= size) {
                break;
            }
        }

        // print download completion
        printf(" \n\nDownload complete! File saved as %s\n", filePath);
    }

    // close the file
    fclose(file);

    // 32
    // send if file download is a success
    if (percentage >= 100.0) {
        isSuccess = 1;
    } else{
        isSuccess = 0;
    }
    bzero(resume, sizeof(resume));
    snprintf(resume, sizeof(resume), "%d", isSuccess);
    result = write(cliFd, resume, sizeof(resume));
    errorCode(result, "write()");
}

// custom function to upload files
void uploadFile (int cliFd) {
    // variables
    int rslt = 0, bytes = 0;
    FILE *file;
    char buffer[chunkSize], filePath[512], fileName[128];

    // set all bytes to 0
    bzero(buffer, sizeof(buffer));
    bzero(filePath, sizeof(filePath));

    // 35
    // obtain the file name and path
    printf("Enter the file name : ");
    scanf("%s", fileName);
    printf("Please enter the full file path : ");
    scanf("%s", filePath);

    // 36
    // open the file and handle errors
    file = fopen(filePath, "rb");
    while (file == NULL) {
        printf("Enter a valid file path!\n");

        printf("Please enter the full file path : ");
        scanf("%s", filePath);

        // open the file
        file = fopen(filePath, "rb");
    }

    // 37 
    // send file name to server
    rslt = write(cliFd, fileName, sizeof(fileName));
    errorCode(rslt, "write()");

    // 41
    //read and send in specified chunks
    while ((bytes = fread(buffer, 1, chunkSize, file)) > 0) {
        rslt = write(cliFd, buffer, chunkSize);
        errorCode(rslt, "write()");
    }
}

// main method
// argv[1] = Client IP address
// argv[2] = Server IP address
int main (int argc, char *argv[])
{
    // create required directories
    mkdir("shared_files", 0777);

    // socket file descriptors
    int cliFd;

    // variables
    char task[10];
    int result = 0; 
    
    // set task array to 0
    bzero(&task, sizeof(task));

    // server socket address structure
    struct sockaddr_in server, client;

    // sizes of socket address structures
    socklen_t svrSize = sizeof(server);
    socklen_t cliSize = sizeof(client);

    // argument error handling
    if (argc < 3) {
        errorCode(-1, "1.Please enter Client and Server IP addresses!");
    }

    //create empty socket
    // IPv4
    // TCP
    cliFd = socket(AF_INET, SOCK_STREAM, 0);
    errorCode(cliFd, "2.main -> socket()");

    // set all bits of client socket address structure to 0
    bzero(&client, cliSize);

    // set values to client socket address structure
    // IPv4
    // IP address = predefined
    // Port number = 7996
    client.sin_family = AF_INET;
    client.sin_addr.s_addr = inet_addr(argv[1]);
    client.sin_port = htons(cliPort);

    // bind client to 7996 port
    result = bind(cliFd, (struct sockaddr *)&client, cliSize);
    errorCode(result, "3.main -> bind()");

    // set all bits of server socket address structure to 0
    bzero(&server, svrSize);

    // set values to server socket address structure
    // IPv4
    // IP address = argv[1]
    // Port number = 2216
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(argv[2]);
    server.sin_port = htons(svrPort);

    // show connection details
    printf("Connecting to server at %s:%d ...\n", argv[2], svrPort);

    // 5
    // connect()
    result = connect(cliFd, (struct sockaddr *)&server, svrSize);
    errorCode(result, "4.main -> connect()");

    // 8
    // display if connection is successful
    printf("Connected to the server.\n");

    // prompt to download/upload (task)
    printf("\nEnter \n - 'download' to download from server\n - 'upload' to upload to server\nPrompt : ");
    scanf("%s", task);
    printf("\n");
    // handle error
    while ((strcmp(task, "download") != 0) && (strcmp(task, "upload") != 0)) {
        printf("Incorrect task! Please try again!\n");
        printf("\nEnter \n - 'download' to download from server\n - 'upload' to upload to server\nPrompt : ");
        scanf("%s", task);
        printf("\n");
        task[strcspn(task, "\n")] = 0;
    }

    // 9
    // send task to server
    result = write(cliFd, task, sizeof(task));
    errorCode(result, "6.main -> send()");

    // call required functions based on input
    if (strcmp("download", task) == 0) {
        downloadFile(cliFd);
    } else if (strcmp("upload", task) == 0) {
        uploadFile(cliFd);
    }

    // close file descriptor
    close(cliFd);

    // end of client program
    return 0;
}
