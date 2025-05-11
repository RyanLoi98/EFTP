#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
/*
 * Name: Ryan Loi
 * Date (dd/mm/yyyy): 08/03/2023
 * Lecture: L02
 * Tutorial: T02
 *
 * CPSC 441 Assignment 3
 *
 * Server side of the Easy File Transfer Protocol (EFTP)
 *
 * The only form of help (very basic questions) obtained was from the TA of Tutorial T02 (Sadaf Erfanmanesh)
 */



// creating some global variables that will be used to for the EFTP Message packet structures

// AUTH structure ending indices, and size
int authOpIndex = 1;
int authUsrIndex = 33;
int authNull1Index = 34;
int authPassIndex = 66;
int authNull2Index = 67;
int authSize = 68;

// RRQ structure ending indices, and size
int rrqOpIndex = 1;
int rrqSessIndex = 6;
int rrqFileIndex = 261;
int rrqNull = 262;
int rrqSize = 263;

// WRQ structure ending indices, and size
int wrqOpIndex = 1;
int wrqSessIndex = 6;
int wrqFileIndex = 261;
int wrqNullIndex = 262;
int wrqSize = 263;

// Data structure ending indices, and size
int dataOpIndex = 1;
int dataSessIndex = 6;
int dataBlockIndex = 11;
int dataSegIndex = 14;
int dataDataIndex = 1038;
int dataSize = 1039;

// ACK structure ending indices, and size
int ackOpIndex = 1;
int ackSessIndex = 6;
int ackBlockIndex = 11;
int ackSegIndex = 14;
int ackSize = 15;

// Error structure ending indices, and size
int errOpIndex = 1;
int errStrIndex = 513;
int errNullIndex = 514;
int errSize = 515;

// max receiving size
int maxRecv = 1040;



/**
 * Function to generate the EFTP ACK packet structure
 * @param session: parameter to hold the server generated session number
 * @param block: parameter to hold the block number
 * @param segment: parameter to hold the segment number
 * @param ackStructReturn: param holding a pointer in the main function where the final ACK EFTP packet structure will
 *                         be copied to
 */
void generateACK(const char* session, const char* block, const char* segment, char *ackStructReturn[]){
    // character array for to hold the ack EFTP packet structure, size is the size of an ack packet
    char ackStructure[ackSize];

    // counters for the loops which will be used to access the session, block, and segment strings
    int sessionCount = 0;
    int blockCount = 0;
    int segmentCount = 0;

    // add the op code
    ackStructure[0] = '0';
    ackStructure[ackOpIndex] = '5';

    // loop through the empty ack packet, start looping at 2 because we need to start after the op code
    for(int i = 2; i < ackSize; i++){

        // if i is within the index of the packet structure meant for the 2 byte session number (5 digit number max) then
        // fill in the session number
        if(i <= ackSessIndex){
            // if the session counter is less than the length of the session string then access the character at the index
            // indicated by the session counter and enter it into the ack structure
            if(sessionCount < strlen(session)){
                ackStructure[i] = session[sessionCount];
                // increment the counter
                sessionCount++;

                // otherwise we have finished entering in the session number but there is still space in the ack structure
                // so fill it with unit separator characters
            }else {
                ackStructure[i] = '\31';
            }
        }

        // if i is within the index of the packet structure meant for the 2 byte block number (5 digit number max) then
        // fill in the block number
        if((i > ackSessIndex) && (i <= ackBlockIndex)){
            // if the block counter is less than the length block string then access the character at the index
            // indicated by the block counter and enter it into the ack structure
            if(blockCount < strlen(block)){
                ackStructure[i] = block[blockCount];
                // increment the block counter
                blockCount++;

                // otherwise we have finished entering in the block number but there is still space in the ack structure
                // so fill it with unit separator characters
            }else {
                ackStructure[i] = '\31';
            }
        }

        // if i is within the index of the packet structure meant for the 1 byte segment number (3 digit number max) then
        // fill in the segment number
        if((i <= ackSegIndex) && (i > ackBlockIndex)){
            // if the segment counter is less than the length segment string then access the character at the index
            // indicated by the segment counter and enter it into the ack structure
            if(segmentCount < strlen(segment)){
                ackStructure[i] = segment[segmentCount];
                // increment the segment counter
                segmentCount++;

                // otherwise we have finished entering in the segment number but there is still space in the ack structure
                // so fill it with unit separator characters
            }else {
                ackStructure[i] = '\31';
            }
        }
    }

    // copy the ack structure into the allocated memory pointer in the main function
    strncpy((char *) ackStructReturn, ackStructure, ackSize);
}



/**
 * Function to generate the EFTP error packet structure
 * @param errorMessage: error message
 * @param errStructReturn: an EFTP error message packet structure containing the error message
 */
void generateError(const char* errorMessage, char *errStructReturn[]){
    // character array for to hold the error EFTP packet structure, size is the size of an error packet
    char errStructure[errSize];

    // counters for the loops which will be used to access the error message string
    int errCount = 0;

    // add the op code
    errStructure[0] = '0';
    errStructure[errOpIndex] = '6';

    // loop through the empty err packet, start looping at 2 because we need to start after the op code
    for(int i = 2; i < errSize; i++){

        // if i is within the index of the packet structure meant for the 512 byte error message string (512 characters
        // max) then fill in the error message
        if(i <= errStrIndex){
            // if the session counter is less than the length of the error string then access the character at the index
            // indicated by the session counter and enter it into the err structure
            if(errCount < strlen(errorMessage)){
                errStructure[i] = errorMessage[errCount];
                // increment the counter
                errCount++;

                // otherwise we have finished entering in the error message but there is still space in the err structure
                // so fill it with unit separator characters
            }else{
                errStructure[i] = '\31';
            }
        }

    }
    // make the last byte a null character
    errStructure[errNullIndex] = '\0';

    // copy the err structure into the allocated memory pointer in the main function
    strncpy((char *) errStructReturn, errStructure, errSize);
}



/**
 * Function to check if the number of command line arguments are correct (4 are expected - including the program name).
 * If an incorrect number of command line arguments are given the program will exit and an error message will be printed.
 *
 * @param argc: the number of command line arguments the program is given
 */
void argCountCheck(int argc){
    // checking command line arguments
    // if argc == 1 no command line arguments are given so print an error message and exit with error code 1
    if (argc == 1){
        printf("Error: No command line arguments given. The server must be given (in this exact order):\na valid username:password "
               "(in this exact format and colons \":\" cannot be used in the username or password)\nfor a client, a port number to listen to (in the range of 2000 - 65535),"
               " and a valid path to the working directory for the server\n");
        printf("Server shutting down...\n");
        exit(1);


        // otherwise if more than 3 command line arguments are given (argc > 4) print an error message that too many
        // command line arguments are given and exit with error code 1
    } else if (argc > 4){
        printf("Error: too many command line arguments are given, enter (in this exact order): a valid username:password\n"
               "(in this exact format and colons \":\" cannot be used in the username or password)\nfor a client, a port number to listen to (in the range of 2000 - 65535),"
               " and a valid path to the working directory for the server\n");
        printf("Server shutting down...\n");
        exit(1);


        // otherwise if less than 3 command line arguments are given (argc < 4) then print an error message that not
        // enough command line arguments are given and exit with error code 1
    } else if (argc < 4){
        printf("Error: not enough command line arguments are given, enter (in this exact order):\na valid username:password "
               "(in this exact format and colons \":\" cannot be used in the username or password)\nfor a client, a port number to listen to (in the range of 2000 - 65535),"
               " and a valid path to the working directory for the server\n");
        printf("Server shutting down...\n");
        exit(1);
    }
}




/**
 * A function that generates a random port number between: 2000 - 65535.
 * @return A random port number between: 2000 - 65535
 */
int randPort(){
    // return the random number, from 2000 - 65535 including the end values
    return ((rand() % (65535-2000 + 1)) + 2000);
}



/**
 * Function to validate the port number a user entered for the server. A valid port number is an integer from 2000-65535.
 * An invalid port number causes the server to print an error message and the program to exit.
 *
 * @param portArg: pointer containing the port number given to portCheck
 * @return: a validated port number, otherwise the program will exit
 */
int portCheck (char *portArg[]){
    // variable to hold the port number as an integer
    int port;

    // get the length of the port number
    int portLen = strlen(*portArg);

    // convert the port number from a string to an integer
    port = atoi(*portArg);

    // atoi returns a 0 if the input was a non integer input, so check to see if the input was invalid
    // if it was invalid, throw an error and exit with error code 1
    if(port == 0){
        printf("Error: the port number cannot have characters or symbols, it must be an integer from 2000 - 65535\n");
        printf("Server shutting down...\n");
        exit(1);
    }

    // convert the port number back into a string, so we can compare lengths with the original input string. This way
    // we can determine if the user entered any additional characters after the port number, which would be removed
    // by the atoi function resulting in the integer getting shorter (size is + 1 to include room for \0)
    char strPort [portLen+1];
    sprintf(strPort, "%d", port);

    // if the atoi converted port number was shorter than the original input string then throw an error that characters are not allowed
    // and exit with error code 1
    if(portLen != strlen(strPort)) {
        printf("Error: the port number cannot have characters or symbols, it must be an integer from 2000 - 65535\n");
        printf("Server shutting down...\n");
        exit(1);
    }

    // now check to see if the port was in the valid range from 2000 - 65535, if not throw an error and exit with error code 1
    if (port > 65535 || port < 2000){
        printf("Error: port number must be an integer number from 2000 - 65535\n");
        printf("Server shutting down...\n");
        exit(1);
    }

    // if we reach this point, then the port number is valid so return it
    return port;
}



/**
 * Function that checks if a received session number is equal to the actual session number
 * @param incomingData: a received session number from a data packet received from the client
 * @param sessActual: the actual session number that the client should have and was generated by the server
 * @return: 0 if the actual session number doesn't match the received session number, 1 if it does match.
 */
int checkSess(const char* incomingData, int sessActual){
    // session number counter
    int sessCounter = 0;
    // loop through the session number
    for (int i = 2; i <= 6; i++) {
        // if the i-th character in the session number isn't our line separator increment the sessionNumLen
        if (incomingData[i] != '\31') {
            sessCounter++;
        }
    }

    // get the session number as a string
    char sessStr[6];
    memset(sessStr, 0, sizeof (sessStr));
    strncpy(sessStr, &incomingData[2], sessCounter);

    // convert the sessNum into an integer
    int sessNum = atoi(sessStr);

    // if sessNum is equal to the sessActual return 1, 0 otherwise
    if(sessNum == sessActual){
        return 1;
    }
    return 0;
}



/**
 *  Function to generate the EFTP data packet structure - specifically for binary data. This is tailored made
 *  for the binary data, there will be no extra space.
 * @param block: parameter to hold the block number
 * @param segment: parameter to hold the segment number
 * @param data: parameter to hold data
 * @param size: parameter to hold size of the data
 * @param dataStructReturn: param holding a pointer in the main function where the final data EFTP packet structure will
 *                         be copied to
 */
void generateBinaryData(const char* block, const char* segment, const char* data, int size, char *dataStructReturn[]){
    // character array for to hold the data EFTP packet structure, dataSize is 15 (bytes for the op code, session #, block #,
    // and segment #) + size of the data packet
    int binSize = 15 + size;
    char dataStructure[binSize];

    // counters for the loops which will be used to access the block, segment, and data strings
    int sessionCount = 0;
    int blockCount = 0;
    int segmentCount = 0;
    int dataCount = 0;

    // add the op code
    dataStructure[0] = '0';
    dataStructure[dataOpIndex] = '4';

    // variable to hold size as a string
    char szeStr[5];
    memset(szeStr, 0, sizeof (szeStr));

    // convert the size into a string
    sprintf(szeStr, "%d", size);


    // loop through the empty data packet, start looping at 2 because we need to start after the op code
    for(int i = 2; i < binSize; i++){

        // fill size into spot for session
        if(i <= dataSessIndex){
            if(sessionCount < 5){
                if( szeStr[sessionCount] != '\0'){
                    dataStructure[i] = szeStr[sessionCount];
                }else{
                    dataStructure[i] = '\31';
                }
                sessionCount++;
                // fill remainder with line separator
            }else{
                dataStructure[i] = '\31';
            }
        }

        // if i is within the index of the packet structure meant for the 2 byte block number (5 digit number max) then
        // fill in the block number
        if((i > dataSessIndex) && (i <= dataBlockIndex)){
            // if the block counter is less than the length block string then access the character at the index
            // indicated by the block counter and enter it into the data structure
            if(blockCount < strlen(block)){
                dataStructure[i] = block[blockCount];
                // increment the block counter
                blockCount++;

                // otherwise we have finished entering in the block number but there is still space in the data structure
                // so fill it with unit separator characters
            }else {
                dataStructure[i] = '\31';
            }
        }

        // if i is within the index of the packet structure meant for the 1 byte segment number (3 digit number max) then
        // fill in the segment number
        if((i <= dataSegIndex) && (i > dataBlockIndex)){
            // if the segment counter is less than the length segment string then access the character at the index
            // indicated by the segment counter and enter it into the data structure
            if(segmentCount < strlen(segment)){
                dataStructure[i] = segment[segmentCount];
                // increment the segment counter
                segmentCount++;

                // otherwise we have finished entering in the segment number but there is still space in the data structure
                // so fill it with unit separator characters
            }else {
                dataStructure[i] = '\31';
            }
        }

        // if i is within the packet structure meant for the data block (1024 chars max) then
        // fill in the data
        if((i < binSize) && (i > dataSegIndex)){
            // if the data counter is less than the length of the data string then access the character at the index
            // indicated by the data counter and enter it into the data structure
            if(dataCount < size){
                dataStructure[i] = data[dataCount];
                // increment the segment counter
                dataCount++;
            }
        }
    }

    // copy the data structure into the allocated memory pointer in the main function
    memcpy(dataStructReturn, dataStructure, binSize);
}



/**
 *  Function to generate the EFTP data packet structure
 * @param session: parameter to hold the server generated session number
 * @param block: parameter to hold the block number
 * @param segment: parameter to hold the segment number
 * @param data: parameter to hold data
 * @param dataStructReturn: param holding a pointer in the main function where the final data EFTP packet structure will
 *                         be copied to
 */
void generateData(const char* session, const char* block, const char* segment, const char* data, char *dataStructReturn[]){
    // character array for to hold the data EFTP packet structure, size is the size of a data packet
    char dataStructure[dataSize];

    // counters for the loops which will be used to access the session, block, segment, and data strings
    int sessionCount = 0;
    int blockCount = 0;
    int segmentCount = 0;
    int dataCount = 0;

    // add the op code
    dataStructure[0] = '0';
    dataStructure[dataOpIndex] = '4';

    // loop through the empty data packet, start looping at 2 because we need to start after the op code
    for(int i = 2; i < dataSize; i++){

        // if i is within the index of the packet structure meant for the 2 byte session number (5 digit number max) then
        // fill in the session number
        if(i <= dataSessIndex){
            // if the session counter is less than the length of the session string then access the character at the index
            // indicated by the session counter and enter it into the data structure
            if(sessionCount < strlen(session)){
                dataStructure[i] = session[sessionCount];
                // increment the counter
                sessionCount++;

                // otherwise we have finished entering in the session number but there is still space in the data structure
                // so fill it with unit separator characters
            }else{
                dataStructure[i] = '\31';
            }
        }

        // if i is within the index of the packet structure meant for the 2 byte block number (5 digit number max) then
        // fill in the block number
        if((i > dataSessIndex) && (i <= dataBlockIndex)){
            // if the block counter is less than the length block string then access the character at the index
            // indicated by the block counter and enter it into the data structure
            if(blockCount < strlen(block)){
                dataStructure[i] = block[blockCount];
                // increment the block counter
                blockCount++;

                // otherwise we have finished entering in the block number but there is still space in the data structure
                // so fill it with unit separator characters
            }else {
                dataStructure[i] = '\31';
            }
        }

        // if i is within the index of the packet structure meant for the 1 byte segment number (3 digit number max) then
        // fill in the segment number
        if((i <= dataSegIndex) && (i > dataBlockIndex)){
            // if the segment counter is less than the length segment string then access the character at the index
            // indicated by the segment counter and enter it into the data structure
            if(segmentCount < strlen(segment)){
                dataStructure[i] = segment[segmentCount];
                // increment the segment counter
                segmentCount++;

                // otherwise we have finished entering in the segment number but there is still space in the data structure
                // so fill it with unit separator characters
            }else {
                dataStructure[i] = '\31';
            }
        }

        // if i is within the index of the packet structure meant for the 1024 byte segment number (1024 chars max) then
        // fill in the data
        if((i <= dataDataIndex) && (i > dataSegIndex)){
            // if the data counter is less than the length of the data string then access the character at the index
            // indicated by the data counter and enter it into the data structure
            if(dataCount < strlen(data)){
                dataStructure[i] = data[dataCount];
                // increment the segment counter
                dataCount++;

                // otherwise we have finished entering in the segment number but there is still space in the data structure
                // so fill it with unit separator characters
            }else {
                dataStructure[i] = '\31';
            }
        }
    }

    // copy the data structure into the allocated memory pointer in the main function
    strncpy(dataStructReturn, dataStructure, dataSize);
}



/**
 * Function that checks to see if a working directory path is valid, if not the program will print an error and exit with
 * error code 1
 *
 * @param workingDir: a parameter containing a working directory path
 */
void checkDir(const char *workingDir){

    // use the stat function to verify if the working directory exists

    // create a stat structure
    struct stat path;

    // initialize the working directory path attributes (treated as a file) to the variable path
    stat(workingDir, &path);

    // use S_ISDIR and st_mode to check if it isn't a valid path, if so print an error message and exit with error code 1
    if(!S_ISDIR(path.st_mode)){
        printf("\nError: the argument for the working directory is invalid\n"
               "The server must be given (in this exact order): a valid username:password\n"
               "(in this exact format and colons \":\" cannot be used in the username or password)\nfor a client, a port number to listen to (in the range of 2000 - 65535),"
               " and a valid path to the working\ndirectory for the server\n");
        printf("Server shutting down...\n");
        exit(1);
    }
}



/**
 * Function that checks if the command line argument for the username:password of a client is correct (i.e. containing
 * only 1 colon separating the username from the password). If the format is incorrect then an error message will
 * be printed and the program will exit with error code 1.
 *
 * @param userPWD: pointer to the username:password argument
 */
void checkUserPWD (const char *userPWD){

    // get the length of the username:password argument
    int length = strlen(userPWD);

    // check to see if colon is first or last thing in the argument for the username:password argument. If it is
    // then the format is incorrect and the user is missing a user name or password or they entered too many colons
    if ((userPWD[0] == ':') || userPWD[length-1] == ':'){
        printf("\nError: the argument for the client's username:password is invalid\n"
               "The server must be given (in this exact order): a valid username:password\n"
               "(in this exact format and colons \":\" cannot be used in the username or password)\nfor a client, a port number to listen to (in the range of 2000 - 65535),"
               " and a valid path to the working\ndirectory for the server\n");
        printf("Server shutting down...\n");
        exit(1);
    }

    // counter to keep track of how many colons the username:password argument has
    int counter = 0;

    // loop through the username:password argument and check to see if it has more than 1 :, if so print an error
    // and exit the program
    for(int i = 0; i < length; i++){
        // increment counter if i=th character of username:password argument == ":"
        if (userPWD[i] == ':'){
            counter++;
        }


        // if counter > 1 print error message and exit because the string has more than 1 :
        if (counter > 1){
            printf("\nError: the argument for the client's username:password is invalid\n"
                   "The server must be given (in this exact order): a valid username:password\n"
                   "(in this exact format and colons \":\" cannot be used in the username or password)\nfor a client, a port number to listen to (in the range of 2000 - 65535),"
                   " and a valid path to the working\ndirectory for the server\n");
            printf("Server shutting down...\n");
            exit(1);
        }
    }

    // after the loop check if counter is 0, in that case there was no colon in the username:password argument so
    // print an error message and exit with error code 1
    if (counter == 0){
        printf("\nError: the argument for the client's username:password is invalid\n"
               "The server must be given (in this exact order): a valid username:password\n"
               "(in this exact format and colons \":\" cannot be used in the username or password)\nfor a client, a port number to listen to (in the range of 2000 - 65535),"
               " and a valid path to the working\ndirectory for the server\n");
        printf("Server shutting down...\n");
        exit(1);
    }
}



/**
 * A function that generates a random session number between: 1 - 65535 (including the end points)
 * @return A random session number between: 1 - 65535 (including the end points)
 */
int randSessionNum() {
    // return the random number, from 1 - 65535 including the end values
    return ((rand() % 65535) + 1);
}



/**
 * Function that runs the server side of the program utilizing UDP transport layer protocol, and Easy File Transfer
 * protocol
 * @param argc: parameter that holds the number of command line arguments given to the program
 * @param argv: parameter that holds the command line arguments given to the program
 * @return: returns 0 if the program executed successfully, returns 1 if there was an error that caused the server to halt
 */
int main(int argc, char *argv[]) {
    // seeding random with time
    srand(time(NULL));

    // check if the number of command line arguments is correct, if it is then the program can move on without exiting
    argCountCheck(argc);

    // get session number
    int SessionNum = randSessionNum();



    // create some variables to hold values later on in program execution

    // commandline argument for listening port number
    int listenPort;

    // check the port number (index 2 of argv), then assign the result to port, if the port number is invalid then
    // portCheck will terminate the program with an error message
    listenPort = portCheck(&argv[2]);


    // variable to hold the commandline argument for working directory of the server, size is the length of the string representing the path
    // to the working directory, which is the argument at index 3 of argv.
    char workingDir [strlen(argv[3]) + 1];
    // cleaning buffers of the variable to hold incoming data
    memset(workingDir, 0, sizeof(workingDir));

    // check working directory (index 3 of argv), if valid execution can continue, otherwise checkDir will terminate the
    // program with an error message
    checkDir(argv[3]);
    // assign working directory to its variable
    strcpy(workingDir, argv[3]);

    // change the working directory, if the process fails print an error message and exit with error code 1.
    if(chdir(workingDir) == -1){
        printf("Error: Server unable to switch to the specified working directory\n");
        printf("Server shutting down...\n");
        exit(1);
    }

    // check username:password (index1 of argv), if valid execution can continue, otherwise checkUserPWD will terminate
    // the program with an error message
    checkUserPWD(argv[1]);


    // get the username and password
    // use strtok to split the first command line argument via the colon :

    // creating the pointer for strtok
    char * pointer = strtok(argv[1], ":");
    // variable to hold the commandline argument for a valid username, size is the length of the string representing the username
    char username [strlen(pointer) + 1];
    // cleaning buffers of the variable to hold incoming data
    memset(username, 0, sizeof(username));
    // first get the user name
    strcpy(username, pointer);

    // check if the user name is > 32 characters long (limit of the EFTP message packet structure) if so print error
    // and exit with error code 1
    if(strlen(username) > 32){
        printf("Error: username is too long, username must be at most 32 characters.\n");
        printf("Server shutting down...\n");
        exit(1);
    }

    // advance the pointer to get the password
    pointer = strtok(NULL, ":");
    // variable to hold the commandline argument for a valid password, size is the length of the string representing the password
    char password [strlen(pointer) + 1];
    // cleaning buffers of the variable to hold incoming data
    memset(password, 0, sizeof(password));
    // get the password
    strcpy(password, pointer);

    // check if the password is > 32 characters long (limit of the EFTP message packet structure) if so print error
    // and exit with error code 1
    if(strlen(password) > 32){
        printf("Error: password is too long, password must be at most 32 characters.\n");
        printf("Server shutting down...\n");
        exit(1);
    }


    // Establishing the UDP and EFTP protocols - server side

    // creation of socket, addresses, and binding

    // UDP Socket creation
    int serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    // check status of socket creation, if == -1 socket creation failed so print error message and end program with
    // error code 1
    if(serverSocket == -1){
        printf("Error: server was unable to create a UDP socket...\n");
        printf("Server shutting down...\n");
        exit(1);
    }


    // creating variables to hold the server and client addresses
    struct sockaddr_in serverAddress, clientAddress;

    // resetting memory buffers of the client and server addresses
    memset((char*) &serverAddress, 0, sizeof(serverAddress));
    memset((char*) &clientAddress, 0, sizeof (clientAddress));

    // getting size of client address for later
    int sizeOfClientAddress = sizeof(clientAddress);

    // continuing address setup by adding ip address and port

    // specifies the address family (AF_INET is IPv4 protocol)
    serverAddress.sin_family = AF_INET;
    // specifies port number to be the one the user entered via command line argument, htson convert from 16 bit host-byte-order to network-byte-order
    serverAddress.sin_port = htons(listenPort);
    //specifies ip address to be 127.0.0.1 (which is the local host IP). The inet_addr converts the ip from
    //"numbers-and-dots notation in CP into binary data in network byte order".
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");



    // create a variable to capture error statuses from various operations
    int errorStatus;


    // binding the socket to the port and ip address
    errorStatus = bind(serverSocket, (struct sockaddr*) &serverAddress, sizeof(serverAddress));

    // check if the binding was unsuccessful (errorStatus == -1), if so print an error message and exit with error code 1
    if(errorStatus == -1){
        printf("Error: server was unable to bind UDP socket to an ip address and port...\n");
        printf("Server shutting down...\n");
        exit(1);
    }







// loop server forever
    while (1) {
        printf("\nServer waiting for a client...\n");
        // initialize serversocket2 to have the port number equal to the session number
        int serverSocket2 = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

        // check status of socket creation, if == -1 socket creation failed so print error message and end program with
        // error code 1
        if(serverSocket2 == -1){
            printf("Error: server was unable to create a UDP socket...\n");
            printf("Server shutting down...\n");
            exit(1);
        }


        // creating variables to hold the server and client addresses
        struct sockaddr_in serverAddress2;

        // resetting memory buffers of the client and server addresses

        memset((char*) &serverAddress2, 0, sizeof(serverAddress2));

        // specifies the address family (AF_INET is IPv4 protocol)
        serverAddress2.sin_family = AF_INET;
        // creating the random port for the "z-port"
        int randomZport = randPort();

        // specifies port number to be random port number, htson convert from 16 bit host-byte-order to network-byte-order
        serverAddress2.sin_port = htons(randomZport);
        //specifies ip address to be 127.0.0.1 (which is the local host IP). The inet_addr converts the ip from
        //"numbers-and-dots notation in CP into binary data in network byte order".
        serverAddress2.sin_addr.s_addr = inet_addr("127.0.0.1");


        // binding the socket to the port and ip address
        errorStatus = bind(serverSocket2, (struct sockaddr*) &serverAddress2, sizeof(serverAddress2));

        // check if the binding was unsuccessful (errorStatus == -1), if so print an error message and exit with error code 1
        if(errorStatus == -1){
            printf("\nError: server was unable to bind UDP socket2 to an ip address and port...\n");
            printf("Server shutting down...\n");

            // cannot send a message to the client who is now listening to the new socket port number, so just exit as the
            // client will automatically time out

            exit(1);
        }



        // variable to hold the client's session number
        char SessionNumStr[6];
        // cleaning buffers of the variable to hold incoming data
        memset(SessionNumStr, 0, sizeof(SessionNumStr));

        // Authorization step

        // create a variable to hold the AUTH EFT packet from the client
        char authPCK[maxRecv];

        // cleaning buffers of the variable to hold incoming data
        memset(authPCK, 0, sizeof(authPCK));

        // receiving auth packet data from the client
        errorStatus = recvfrom(serverSocket, authPCK, sizeof(authPCK), 0, (struct sockaddr *) &clientAddress,
                               &sizeOfClientAddress);


        // check if the data received from the client contained the error op code, if so print the error message and exit.
        if(authPCK[authOpIndex] == '6'){
            // counter for error message length
            int errorMsgLen = 0;

            // loop through the error EFPT packet to see how big the message is, start at the first index of the error
            // message
            for(int i = (errOpIndex+1); i <= errStrIndex; i++){
                // if the i-th character in the error message isn't our line separator increment the errorMsgLen
                if (authPCK[i] != '\31'){
                    errorMsgLen++;
                }
            }
            // create a variable for the error message (+1 so we can append a '\0' later on)
            char errMSG[errorMsgLen + 1];
            // cleaning buffers of the variable to hold incoming data
            memset(errMSG, 0, sizeof(errMSG));

            // copy the error message only, from the first index (op code index + 1)
            strncpy(errMSG, &authPCK[(errOpIndex + 1)], errorMsgLen);


            // print the error message, use continue on the loop so the server can connect to a different client
            printf("\n%s\n", errMSG);
            printf("Server aborting connection to the client...\n\n");
            continue;

            // if the op code was 1 we have recieved the auth message, check to see if the reception was ok
        }else if(authPCK[authOpIndex] == '1') {

            // if there were no errors with reception print a message, process the session number and break out of the loop
            if (errorStatus > 0) {
                printf("Server has received an Authorization packet from the client!\n\nBeginning authentication process...\n\n");

                // process the AUTH EFTP packet

                // get user name

                // counter for username length
                int userLen = 0;

                // loop through the AUTH EFTP packet to see how big the username string is, start at the first index
                // of the user name
                for(int i = (authOpIndex+1); i <= authUsrIndex; i++){
                    // if the i-th character in the username isn't our line separator increment the userLen
                    if (authPCK[i] != '\31'){
                        userLen++;
                    }
                }

                // create a variable for the username (+1 so we can append a null value later)
                char usernameC[userLen + 1];
                // cleaning buffers of the variable to hold incoming data
                memset(usernameC, 0, sizeof(usernameC));

                // copy the username only, from the first index (op code index + 1)
                strncpy(usernameC, &authPCK[(authOpIndex + 1)], userLen);


                // get password

                // counter for password length
                int passLen = 0;

                // loop through the AUTH EFTP packet to see how big the password string is, start at the first index
                // of the password
                for(int i = (authNull1Index+1); i <= authPassIndex; i++){
                    // if the i-th character in the password isn't our line separator increment passLen
                    if (authPCK[i] != '\31'){
                        passLen++;
                    }
                }

                // create a variable for the password (+1 so we can append a null value later)
                char passwordC[passLen + 1];
                // cleaning buffers of the variable to hold incoming data
                memset(passwordC, 0, sizeof(passwordC));

                // copy the username only, from the first index (first null character index + 1)
                strncpy(passwordC, &authPCK[(authNull1Index + 1)], passLen);



                // check if the username and password match, if so generate a random session number
                // then create an ack and send it to the client
                if((strcmp(username, usernameC) == 0) && (strcmp(password, passwordC) == 0)){
                    // print message that the username and password matches
                    printf("Authentication success! Username and Password from the client matches server record!\n");

                    // convert session number into a string
                    sprintf(SessionNumStr, "%d", SessionNum);

                    // print session number
                    printf("Generated session number for this client-server communication is: %d\n", SessionNum);

                    // print out the random port the server is transitioning to
                    printf("\nServer transitioning communication to a new port (port number: %d) for future communication with client\n", randomZport);

                    // sending authorization ack to the client
                    char authACK[ackSize+1];
                    // cleaning buffers of the variable to hold incoming data
                    memset(authACK, 0, sizeof(authACK));
                    generateACK(SessionNumStr, "00000", "000", authACK);

                    // send using new server socket
                    errorStatus = sendto(serverSocket2, authACK, strlen(authACK), 0, (struct sockaddr *) &clientAddress,
                                         sizeOfClientAddress);

                    // check to see if sending was successful, if errorStatus < 0, so print error message and
                    // use continue on the loop so the server can connect to a different client
                    if (errorStatus < 0) {
                        printf("\nError: there was an error sending data to the client\n");
                        printf("Server aborting connection to current client...\n\n");
                        continue;
                    }

                    // otherwise user name and password doesn't match so print error message and send error to the client
                    // then use continue on the loop so the server can communicate with other clients
                } else{
                    printf("\nAuthorization Error: Username and Password do not match server records\n");
                    printf("Server aborting connection to current client...\n\n");
                    char errMSG[] = {"Authorization Error: Username and Password do not match server records\n"};
                    char errPCK[errSize+1];
                    // cleaning buffers of the variable to hold incoming data
                    memset(errPCK, 0, sizeof(errPCK));
                    generateError(errMSG, errPCK);

                    // send using original server socket
                    errorStatus = sendto(serverSocket, errPCK, strlen(errPCK), 0, (struct sockaddr *) &clientAddress,
                                         sizeOfClientAddress);

                    continue;
                }
            }
        }



        // Request Phase

        // variable to control if the server is in RRQ or WRQ mode 0 = WRQ, 1 = RRQ
        int mode;

        // print message that the server is receiving a request from the client
        printf("\nServer receiving request type from the client\n");

        // setting socket to time out if nothing is there is inactivity for 20 seconds (gives the client up to 20 seconds to send data)
        struct timeval timeout20sec = {20, 0};
        setsockopt(serverSocket2, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout20sec, sizeof(struct timeval));


        // create a variable to hold the request message
        char request[maxRecv];
        // clearing memory of variable
        memset(request, 0, sizeof(request));

        // receiving request packet data from the client
        errorStatus = recvfrom(serverSocket2, request, sizeof(request), 0, (struct sockaddr *) &clientAddress,
                               &sizeOfClientAddress);


        // if error status < 0 there is a problem receiving from the socket so print an error message, and
        // then send an error to the client before using continue to connect to another client
        if (errorStatus <= 0) {
            printf("\nError: unable to receive request data from the client\n");
            printf("Server aborting connection to the client...\n");

            // create error message
            char errMSG[] = {
                    "Error: the server was unable to receive any request data from its socket and had to abort the connection\n"};
            char errPCK[errSize + 1];
            // cleaning buffers of the variable to hold incoming data
            memset(errPCK, 0, sizeof(errPCK));
            generateError(errMSG, errPCK);

            // send error message to the server
            sendto(serverSocket2, errPCK, strlen(errPCK), 0, (struct sockaddr *) &clientAddress,
                   sizeOfClientAddress);

            continue;
        }


        // check the session number from the client's message matches records, if not disconnect from the client.
        if(checkSess(request, SessionNum) == 0){
            printf("\nError: client's session number doesn't match records...\n");
            printf("Server disconnecting from current client...\n");

            // send error to the client
            // create error message
            char errMSG[] = {
                    "Error: the server was unable to verify the client's data transmission due to an incorrect session number being sent\n"};
            char errPCK[errSize + 1];
            // cleaning buffers of the variable to hold incoming data
            memset(errPCK, 0, sizeof(errPCK));
            generateError(errMSG, errPCK);

            // send error message to the server
            sendto(serverSocket2, errPCK, strlen(errPCK), 0, (struct sockaddr *) &clientAddress,
                   sizeOfClientAddress);

            continue;
        }


        // check packet op code

        // if op code is 6 that is the sever error, so print it out and use continue on the loop so the server can connect
        // with a new client
        if(request[rrqOpIndex] == '6'){
            // counter for error message length
            int errorMsgLen = 0;

            // loop through the error EFPT packet to see how big the message is, start at the first index of the error
            // message
            for(int i = (errOpIndex+1); i <= errStrIndex; i++){
                // if the i-th character in the error message isn't our line separator increment the errorMsgLen
                if (authPCK[i] != '\31'){
                    errorMsgLen++;
                }
            }
            // create a variable for the error message (+1 so we can append a '\0' later on)
            char errMSG[errorMsgLen + 1];
            // cleaning buffers of the variable to hold incoming data
            memset(errMSG, 0, sizeof(errMSG));

            // copy the error message only, from the first index (op code index + 1)
            strncpy(errMSG, &authPCK[(errOpIndex + 1)], errorMsgLen);


            // print the error message, use continue on the loop so the server can connect to a different client
            printf("\n%s\n", errMSG);
            printf("Server aborting connection to the client...\n\n");
            continue;


            // if the op code is 2 set mode to RRQ (1)
        }else if(request[rrqOpIndex] == '2'){
            mode = 1;
            // print message saying the client request
            printf("Client has requested a file download (RRQ)\n");

            // if the op code is 3 set the mode to WRQ (0)
        }else if(request[wrqOpIndex] == '3'){
            // print message saying the client request
            printf("Client has requested a file upload (WRQ)\n");
            mode = 0;


            // otherwise op code is incorrect so print an error and use continue to connect to a new client
            // but also send an error to the client.
        }else{

            printf("\nError: client request had an incorrect op code\n");
            printf("Server aborting connection to the client...\n");

            // create error message
            char errMSG[] = {
                    "Error: client request had an incorrect op code. Server aborting connection...n\n"};
            char errPCK[errSize + 1];
            // cleaning buffers of the variable to hold incoming data
            memset(errPCK, 0, sizeof(errPCK));
            generateError(errMSG, errPCK);

            // send error message to the client
            sendto(serverSocket2, errPCK, strlen(errPCK), 0, (struct sockaddr *) &clientAddress,
                   sizeOfClientAddress);

            continue;

        }

        // get file name

        // create a variable for the file name
        char fileName [256];
        // cleaning up variable buffer
        memset(fileName, 0, sizeof(fileName));


        // create a counter to loop through the file name
        // counter for error message length
        int fileNameLen = 0;

        // loop through the request EFPT packet to see how big the filename is, start at the first index of the error
        // filename
        for(int i = (rrqSessIndex+1); i <= rrqFileIndex; i++){
            // if the i-th character in the error message isn't our line separator increment the errorMsgLen
            if (request[i] != '\31'){
                fileNameLen++;
            }
        }

        // copy the file name into the variable
        strncpy(fileName, &request[rrqSessIndex + 1], fileNameLen);




        // check mode

        // if mode == 1 it is in RRQ mode
        if (mode == 1) {
            // variable to trigger replacement of null characters (only for binary files) (0 = false, 1 = true and enabled)
            int replace = 0;

            // Print saying the server is uploading the file
            printf("Server beginning data transmission for the file called: %s\n", fileName);

            // create a variable to hold the opening mode for FOPEN
            char openMode[3];
            // cleaning buffers of the variable to hold incoming data
            memset(openMode, 0, sizeof(openMode));


            // index the last 3 characters in a file name to see if it has the txt extension, if so it is a text file
            if ((fileName[strlen(fileName) - 1] == 't') && ((fileName[strlen(fileName) - 2] == 'x')) &&
                (fileName[strlen(fileName) - 3] == 't')) {
                // make the opening mode r if the text file is txt, r because we will be reading from the file in RRQ mode
                strcpy(openMode, "r");

                // print that data is text
                printf("Data type of file: Text \n\n");

                // otherwise make the opening mode rb for binary files rb because we will be reading to the file in RRQ mode
            } else {
                strcpy(openMode, "rb");
                // print that data is binary
                printf("Data type of file: binary \n\n");
                // enable the replacement algorithm
                replace = 1;
            }

            // file pointer and file creation
            // creating a pointer to open an output file (file name provided by the client) to write data from the server to
            FILE *filePtr = fopen(fileName, openMode);

            // check to see if the outFile pointer is null, if it is that means fopen was unable to create the file
            // so print an error message and use continue so the server can connect to new clients
            // also send error to the server
            if (filePtr == NULL) {
                printf("Error: server does not contain a file named: \"%s\" to read data from\n",
                       fileName);
                printf("Server aborting connection with client...\n");

                // create error message
                char errMSG[] = {"Error: the server did not contain the requested file"};
                char errPCK[errSize + 1];
                // cleaning buffers of the variable to hold incoming data
                memset(errPCK, 0, sizeof(errPCK));
                generateError(errMSG, errPCK);

                // send error message to the client
                sendto(serverSocket2, errPCK, strlen(errPCK), 0, (struct sockaddr *) &clientAddress,
                       sizeOfClientAddress);

                continue;
            }

            // start reading from the file and sending data

            // create a variable for the block
            char block[8192];
            // cleaning buffers of the variable to hold incoming data
            memset(block, 0, sizeof(block));

            // variable to hold index position of a block when we are accessing data
            int blockIndex = 0;

            // create variable for the block counter
            int blockCount = 1;

            // accumulator variable to see if the file size is a multiple of 1KB, because it accumulates all data ever
            // read (i.e. accumulates block index's)
            int accumulator = 0;

            // convert block number to a string
            char blockCountStr [3];


            // loop until end of file
            while((blockIndex = fread(block, 1, 8192, filePtr)) != 0){

                // cleaning buffers of the variable to hold incoming data
                memset(blockCountStr, 0, sizeof(blockCountStr));
                sprintf(blockCountStr, "%d", blockCount);


                // variables to hold the 8 segments of data
                char segment1[1024], segment2[1024], segment3[1024], segment4[1024], segment5[1024],
                        segment6[1024], segment7[1024], segment8[1024];

                // cleaning buffers of the variable to hold incoming data
                memset(segment1, 0, sizeof(segment1));
                memset(segment2, 0, sizeof(segment2));
                memset(segment3, 0, sizeof(segment3));
                memset(segment4, 0, sizeof(segment4));
                memset(segment5, 0, sizeof(segment5));
                memset(segment6, 0, sizeof(segment6));
                memset(segment7, 0, sizeof(segment7));
                memset(segment8, 0, sizeof(segment8));


                // add actual data read to the accumulator
                accumulator += blockIndex;

                // variables to act like flags to indicate if a segment has its data yet, 0 = no data, 1 = has data
                int seg1Flag = 0, seg2Flag = 0, seg3Flag = 0, seg4Flag = 0, seg5Flag = 0, seg6Flag = 0, seg7Flag = 0, seg8Flag = 0;

                // variables to act like flags to indicate if a segment needs to be retransmitted, initially all set to 1 for yes
                int seg1Retrans = 1, seg2Retrans = 1, seg3Retrans = 1, seg4Retrans = 1, seg5Retrans = 1, seg6Retrans = 1, seg7Retrans = 1, seg8Retrans = 1;

                // for binary use only, variables to store how large the data packs will be (initialized to 0)
                int binSZE1 = 0; int binSZE2 = 0; int binSZE3 = 0; int binSZE4 = 0;
                int binSZE5 = 0; int binSZE6 = 0; int binSZE7 = 0; int binSZE8 = 0;


                // if block count is greater than a thresh hold (intervals of 1024 bytes) that means a segment has data
                // so set its flag to 1

                if(blockIndex > 0){
                    seg1Flag = 1;

                    // set the bin sizes only if in binary mode
                    if(replace){
                        if (blockIndex >= 1024){
                            binSZE1 = 1024 + 15;
                        } else{
                            binSZE1 = blockIndex + 15;
                        }
                    }
                }

                if(blockIndex >= 1024){
                    seg2Flag = 1;

                    // set the bin sizes only if in binary mode
                    if(replace){
                        if (blockIndex >= 2048){
                            binSZE2 = 1024 + 15;
                        } else{
                            binSZE2 = ((blockIndex - 1024) + 15);
                        }
                    }
                }

                if(blockIndex >= 2048){
                    seg3Flag = 1;

                    // set the bin sizes only if in binary mode
                    if(replace){
                        if (blockIndex >= 3072){
                            binSZE3 = 1024 + 15;
                        } else{
                            binSZE3 = ((blockIndex - 2048) + 15);
                        }
                    }
                }

                if(blockIndex >= 3072){
                    seg4Flag = 1;

                    // set the bin sizes only if in binary mode
                    if(replace){
                        if (blockIndex >= 4096){
                            binSZE4 = 1024 + 15;
                        } else{
                            binSZE4 = ((blockIndex - 3072) + 15);
                        }
                    }
                }

                if(blockIndex >= 4096){
                    seg5Flag = 1;

                    // set the bin sizes only if in binary mode
                    if(replace){
                        if (blockIndex >= 5120){
                            binSZE5 = 1024 + 15;
                        } else{
                            binSZE5 = ((blockIndex - 4096) + 15);
                        }
                    }
                }


                if(blockIndex >= 5120){
                    seg6Flag = 1;

                    // set the bin sizes only if in binary mode
                    if(replace){
                        if (blockIndex >= 6144){
                            binSZE6 = 1024 + 15;
                        } else{
                            binSZE6 = ((blockIndex - 5120) + 15);
                        }
                    }
                }

                if(blockIndex >= 6144){
                    seg7Flag = 1;

                    // set the bin sizes only if in binary mode
                    if(replace){
                        if (blockIndex >= 7168){
                            binSZE7 = 1024 + 15;
                        } else{
                            binSZE7 = ((blockIndex - 6144) + 15);
                        }
                    }
                }

                if(blockIndex >= 7168){
                    seg8Flag = 1;

                    // set the bin sizes only if in binary mode
                    if(replace){
                        if (blockIndex == 8192){
                            binSZE8 = 1024 + 15;
                        } else{
                            binSZE8 = ((blockIndex - 7168) + 15);
                        }
                    }
                }

                // make variables to hold all of the data packets
                char dataPCK1[dataSize], dataPCK2[dataSize], dataPCK3[dataSize], dataPCK4[dataSize], dataPCK5[dataSize], dataPCK6[dataSize], dataPCK7[dataSize], dataPCK8[dataSize];
                // cleaning buffers of the variable to hold incoming data
                memset(dataPCK1, 0, sizeof(dataPCK1));  memset(dataPCK2, 0, sizeof(dataPCK2));
                memset(dataPCK3, 0, sizeof(dataPCK3));  memset(dataPCK4, 0, sizeof(dataPCK4));
                memset(dataPCK5, 0, sizeof(dataPCK5));  memset(dataPCK6, 0, sizeof(dataPCK6));
                memset(dataPCK7, 0, sizeof(dataPCK7));  memset(dataPCK8, 0, sizeof(dataPCK8));

                // making variables to hold all of the binary data pack sizes
                char binPCK1[binSZE1], binPCK2[binSZE2], binPCK3[binSZE3], binPCK4[binSZE4], binPCK5[binSZE5], binPCK6[binSZE6], binPCK7[binSZE7], binPCK8[binSZE8];
                // cleaning buffers of the variable to hold incoming data
                memset(binPCK1, 0, sizeof(binPCK1));  memset(binPCK2, 0, sizeof(binPCK2));
                memset(binPCK3, 0, sizeof(binPCK3));  memset(binPCK4, 0, sizeof(binPCK4));
                memset(binPCK5, 0, sizeof(binPCK5));  memset(binPCK6, 0, sizeof(binPCK6));
                memset(binPCK7, 0, sizeof(binPCK7));  memset(binPCK8, 0, sizeof(binPCK8));

                // Create the individual data packets for each segment if their segFlags indicate they have data
                if(seg1Flag){

                    // create data packs depending on if we are in binary mode or not
                    if (replace){
                        // variable to hold the segment data
                        char segdata[binSZE1 - 15];
                        // cleaning buffers of the variable to hold incoming data
                        memset(segdata, 0, sizeof(segdata));

                        // copying over the data
                        memcpy(segdata, &block[0], (binSZE1 - 15));

                        // generating the data packet
                        generateBinaryData(blockCountStr, "1", segdata, (binSZE1 - 15), binPCK1);

                    }else {

                        // variable to hold the segment data
                        char segdata[1024];

                        // cleaning buffers of the variable to hold incoming data
                        memset(segdata, 0, sizeof(segdata));

                        // if block index is >= 1024, we can copy 1024 bytes
                        if (blockIndex >= 1024) {
                            strncpy(segdata, &block[0], 1024);
                            // otherwise just copy the number of bytes the block index states
                        } else {
                            strncpy(segdata, &block[0], blockIndex);
                        }

                        // generate the data packet
                        generateData("00000", blockCountStr, "1", segdata, dataPCK1);
                        // decrement the block index by the amount bytes taken for segment 1
                        blockIndex = blockIndex - 1024;
                    }


                }


                // Create the individual data packets for each segment if their segFlags indicate they have data
                if(seg2Flag){

                    // create data packs depending on if we are in binary mode or not
                    if(replace){
                        // variable to hold the segment data
                        char segdata[binSZE2 - 15];
                        // cleaning buffers of the variable to hold incoming data
                        memset(segdata, 0, sizeof(segdata));

                        // copying over the data
                        memcpy(segdata, &block[1024], (binSZE2 - 15));

                        // generating the data packet
                        generateBinaryData(blockCountStr, "2" , segdata, (binSZE2 - 15), binPCK2);

                    }else {
                        // variable to hold the segment data
                        char segdata[1024];

                        // cleaning buffers of the variable to hold incoming data
                        memset(segdata, 0, sizeof(segdata));

                        // if block index is >= 1024, we can copy 1024 bytes
                        if (blockIndex >= 1024) {
                            strncpy(segdata, &block[1024], 1024);

                            // otherwise just copy the number of bytes the block index states
                        } else {
                            strncpy(segdata, &block[1024], blockIndex);

                        }
                        // generate the data packet
                        generateData("00000", blockCountStr, "2", segdata, dataPCK2);
                        // decrement the block index by the amount bytes taken for segment 2
                        blockIndex = blockIndex - 1024;
                    }

                }


                // Create the individual data packets for each segment if their segFlags indicate they have data
                if(seg3Flag){

                    // generate data packs depending if we are in binary mode or not
                    if(replace){
                        // variable to hold the segment data
                        char segdata[binSZE3 - 15];
                        // cleaning buffers of the variable to hold incoming data
                        memset(segdata, 0, sizeof(segdata));

                        // copying over the data
                        memcpy(segdata, &block[2048], (binSZE3 - 15));

                        // generating the data packet
                        generateBinaryData(blockCountStr, "3", segdata, (binSZE3 - 15), binPCK3);

                    }else {

                        // variable to hold the segment data
                        char segdata[1024];

                        // cleaning buffers of the variable to hold incoming data
                        memset(segdata, 0, sizeof(segdata));

                        // if block index is >= 1024, we can copy 1024 bytes
                        if (blockIndex >= 1024) {
                            strncpy(segdata, &block[2048], 1024);


                            // otherwise just copy the number of bytes the block index states
                        } else {
                            strncpy(segdata, &block[2048], blockIndex);
                        }
                        // generate the data packet
                        generateData("00000", blockCountStr, "3", segdata, dataPCK3);
                        // decrement the block index by the amount bytes taken for segment 3
                        blockIndex = blockIndex - 1024;
                    }
                }


                // Create the individual data packets for each segment if their segFlags indicate they have data
                if(seg4Flag){
                    // generate data packets depending on if we are in binary mode or not
                    if(replace){
                        // variable to hold the segment data
                        char segdata[binSZE4 - 15];
                        // cleaning buffers of the variable to hold incoming data
                        memset(segdata, 0, sizeof(segdata));

                        // copying over the data
                        memcpy(segdata, &block[3072], (binSZE4 - 15));

                        // generating the data packet
                        generateBinaryData(blockCountStr, "4", segdata, (binSZE4 - 15), binPCK4);

                    }else {
                        // variable to hold the segment data
                        char segdata[1024];

                        // cleaning buffers of the variable to hold incoming data
                        memset(segdata, 0, sizeof(segdata));

                        // if block index is >= 1024, we can copy 1024 bytes
                        if (blockIndex >= 1024) {
                            strncpy(segdata, &block[3072], 1024);

                            // otherwise just copy the number of bytes the block index states
                        } else {
                            strncpy(segdata, &block[3072], blockIndex);
                        }
                        // generate the data packet
                        generateData("00000", blockCountStr, "4", segdata, dataPCK4);
                        // decrement the block index by the amount bytes taken for segment 4
                        blockIndex = blockIndex - 1024;
                    }
                }


                // Create the individual data packets for each segment if their segFlags indicate they have data
                if(seg5Flag){
                    if(replace){
                        // variable to hold the segment data
                        char segdata[binSZE5 - 15];
                        // cleaning buffers of the variable to hold incoming data
                        memset(segdata, 0, sizeof(segdata));

                        // copying over the data
                        memcpy(segdata, &block[4096], (binSZE5 - 15));

                        // generating the data packet
                        generateBinaryData(blockCountStr,"5" , segdata, (binSZE5 - 15), binPCK5);
                    }else {
                        // variable to hold the segment data
                        char segdata[1024];

                        // cleaning buffers of the variable to hold incoming data
                        memset(segdata, 0, sizeof(segdata));

                        // if block index is >= 1024, we can copy 1024 bytes
                        if (blockIndex >= 1024) {
                            strncpy(segdata, &block[4096], 1024);

                            // otherwise just copy the number of bytes the block index states
                        } else {
                            strncpy(segdata, &block[4096], blockIndex);
                        }
                        // generate the data packet
                        generateData("00000", blockCountStr, "5", segdata, dataPCK5);
                        // decrement the block index by the amount bytes taken for segment 5
                        blockIndex = blockIndex - 1024;
                    }
                }


                // Create the individual data packets for each segment if their segFlags indicate they have data
                if(seg6Flag){
                    // create data packs depending on if we are in binary mode or not
                    if(replace){
                        // variable to hold the segment data
                        char segdata[binSZE6 - 15];
                        // cleaning buffers of the variable to hold incoming data
                        memset(segdata, 0, sizeof(segdata));

                        // copying over the data
                        memcpy(segdata, &block[5120], (binSZE6 - 15));

                        // generating the data packet
                        generateBinaryData(blockCountStr,"6", segdata, (binSZE6 - 15), binPCK6);
                    }else {
                        // variable to hold the segment data
                        char segdata[1024];

                        // cleaning buffers of the variable to hold incoming data
                        memset(segdata, 0, sizeof(segdata));

                        // if block index is >= 1024, we can copy 1024 bytes
                        if (blockIndex >= 1024) {
                            strncpy(segdata, &block[5120], 1024);
                            // otherwise just copy the number of bytes the block index states
                        } else {
                            strncpy(segdata, &block[5120], blockIndex);
                        }
                        // generate the data packet
                        generateData("00000", blockCountStr, "6", segdata, dataPCK6);
                        // decrement the block index by the amount bytes taken for segment 6
                        blockIndex = blockIndex - 1024;
                    }
                }


                // Create the individual data packets for each segment if their segFlags indicate they have data
                if(seg7Flag){
                    // generate data packs depending on if we are in binary mode or not
                    if(replace){
                        // variable to hold the segment data
                        char segdata[binSZE7 - 15];
                        // cleaning buffers of the variable to hold incoming data
                        memset(segdata, 0, sizeof(segdata));

                        // copying over the data
                        memcpy(segdata, &block[6144], (binSZE7 - 15));

                        // generating the data packet
                        generateBinaryData(blockCountStr, "7", segdata, (binSZE7 - 15), binPCK7);

                    }else {
                        // variable to hold the segment data
                        char segdata[1024];

                        // cleaning buffers of the variable to hold incoming data
                        memset(segdata, 0, sizeof(segdata));

                        // if block index is >= 1024, we can copy 1024 bytes
                        if (blockIndex >= 1024) {
                            strncpy(segdata, &block[6144], 1024);

                            // otherwise just copy the number of bytes the block index states
                        } else {
                            strncpy(segdata, &block[6144], blockIndex);
                        }
                        // generate the data packet
                        generateData("00000", blockCountStr, "7", segdata, dataPCK7);
                        // decrement the block index by the amount bytes taken for segment 7
                        blockIndex = blockIndex - 1024;
                    }
                }

                // Create the individual data packets for each segment if their segFlags indicate they have data
                if(seg8Flag){

                    // create data packs depending on if we are in binary mode or not
                    if(replace){
                        // variable to hold the segment data
                        char segdata[binSZE8 - 15];
                        // cleaning buffers of the variable to hold incoming data
                        memset(segdata, 0, sizeof(segdata));

                        // copying over the data
                        memcpy(segdata, &block[7168], (binSZE8 - 15));

                        // generating the data packet
                        generateBinaryData(blockCountStr,"8" , segdata, (binSZE8 - 15), binPCK8);

                    }else {

                        // variable to hold the segment data
                        char segdata[1024];

                        // cleaning buffers of the variable to hold incoming data
                        memset(segdata, 0, sizeof(segdata));

                        // if block index is >= 1024, we can copy 1024 bytes
                        if (blockIndex >= 1024) {
                            strncpy(segdata, &block[7168], 1024);

                            // otherwise just copy the number of bytes the block index states
                        } else {
                            strncpy(segdata, &block[7168], blockIndex);
                        }
                        // generate the data packet
                        generateData("00000", blockCountStr, "8", segdata, dataPCK8);
                        // decrement the block index by the amount bytes taken for segment 8
                        blockIndex = blockIndex - 1024;

                    }
                }

                // send data and wait for ack

                // setting socket to time out if nothing is there is inactivity for 1 seconds (this is about a 5 second delay, when all the receives are factored in)
                struct timeval timeout1sec = {1, 0};
                setsockopt(serverSocket2, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout1sec, sizeof(struct timeval));


                // loop 3 times max so we can check for acks
                for(int i = 0; i <= 3; i++){

                    // when i == 3 (and atleast 1 segflag == 1) we have checked for acks and resent data the required 3
                    // times but the client is unable to get the data so go to breakfree to connect to a new client
                    if((i == 3) && ((seg1Flag != 0) || (seg2Flag != 0) || (seg3Flag != 0) || (seg4Flag != 0) || (seg5Flag !=0) || (seg6Flag != 0) || (seg7Flag != 0) || (seg8Flag !=0))){
                        printf("\nError: Server unable to transfer data to the client, the client has likely disconnected.\n");
                        goto breakfree;
                    }

                    // print the attempt number only on the first try, or on resending data attempts
                    if(i == 0) {
                        // Display which attempt the server is on in regards to sending data to the client
                        printf("\nData transfer Attempt #%d for block #%d\n", i + 1, blockCount);

                    }else if ((i > 0) && ((seg1Flag != 0) || (seg2Flag != 0) || (seg3Flag != 0) || (seg4Flag != 0) || (seg5Flag !=0) || (seg6Flag != 0) || (seg7Flag != 0) || (seg8Flag !=0))) {
                        // Display which attempt the server is on in regards to sending data to the client
                        printf("\nData transfer Attempt #%d for block #%d\n", i + 1, blockCount);
                    }

                    // transmit segment data if flags allow (has data, not yet recieved ack)
                    if(seg1Flag && seg1Retrans){
                        // print the block number and segment being transmitted
                        printf("Server sending client: segment number %d of block #%d\n", 1, blockCount);

                        // send a binary data pack or text data pack depending on if we are in binary mode or not
                        if(replace){

                            errorStatus = sendto(serverSocket2, binPCK1, sizeof(binPCK1), 0,
                                                 (struct sockaddr *) &clientAddress,
                                                 sizeOfClientAddress);
                        }else {
                            // send datapck
                            errorStatus = sendto(serverSocket2, dataPCK1, strlen(dataPCK1), 0,
                                                 (struct sockaddr *) &clientAddress,
                                                 sizeOfClientAddress);
                        }
                        // check to see if sending was successful, if errorStatus < 0, so print error message and
                        // use goto break free on the loop so the server can connect to a different client
                        if (errorStatus < 0) {
                            printf("\nError: there was an error sending data to the client\n");
                            printf("Server aborting connection to current client...\n\n");
                            goto breakfree;
                        }
                    }


                    // transmit segment data if flags allow (has data, not yet recieved ack)
                    if(seg2Flag && seg2Retrans){
                        // send datapck
                        // print the block number and segment being transmitted
                        printf("Server sending client: segment number %d of block #%d\n", 2, blockCount);

                        // send binary or text data pack depending on if we are in binary mode or not
                        if(replace){
                            errorStatus = sendto(serverSocket2, binPCK2, sizeof(binPCK2), 0,
                                                 (struct sockaddr *) &clientAddress,
                                                 sizeOfClientAddress);
                        }else {
                            errorStatus = sendto(serverSocket2, dataPCK2, strlen(dataPCK2), 0,
                                                 (struct sockaddr *) &clientAddress,
                                                 sizeOfClientAddress);
                        }
                        // check to see if sending was successful, if errorStatus < 0, so print error message and
                        // use goto break free on the loop so the server can connect to a different client
                        if (errorStatus < 0) {
                            printf("\nError: there was an error sending data to the client\n");
                            printf("Server aborting connection to current client...\n\n");
                            goto breakfree;
                        }
                    }


                    // transmit segment data if flags allow (has data, not yet recieved ack)
                    if(seg3Flag && seg3Retrans){
                        // print the block number and segment being transmitted
                        printf("Server sending client: segment number %d of block #%d\n", 3, blockCount);

                        // send a text or binary data pack depending on if we are in binary mode or not
                        if(replace){
                            errorStatus = sendto(serverSocket2, binPCK3, sizeof(binPCK3), 0,
                                                 (struct sockaddr *) &clientAddress,
                                                 sizeOfClientAddress);
                        }else {
                            // send datapck
                            errorStatus = sendto(serverSocket2, dataPCK3, strlen(dataPCK3), 0,
                                                 (struct sockaddr *) &clientAddress,
                                                 sizeOfClientAddress);
                        }
                        // check to see if sending was successful, if errorStatus < 0, so print error message and
                        // use goto break free on the loop so the server can connect to a different client
                        if (errorStatus < 0) {
                            printf("\nError: there was an error sending data to the client\n");
                            printf("Server aborting connection to current client...\n\n");
                            goto breakfree;
                        }

                    }


                    // transmit segment data if flags allow (has data, not yet recieved ack)
                    if(seg4Flag && seg4Retrans){
                        // print the block number and segment being transmitted
                        printf("Server sending client: segment number %d of block #%d\n", 4, blockCount);

                        // send a binary data pack or text data pack depending on if we are in binary mode or not
                        if(replace){
                            // send datapck
                            errorStatus = sendto(serverSocket2, binPCK4, sizeof(binPCK4), 0,
                                                 (struct sockaddr *) &clientAddress,
                                                 sizeOfClientAddress);
                        }else {
                            // send datapck
                            errorStatus = sendto(serverSocket2, dataPCK4, strlen(dataPCK4), 0,
                                                 (struct sockaddr *) &clientAddress,
                                                 sizeOfClientAddress);
                        }
                        // check to see if sending was successful, if errorStatus < 0, so print error message and
                        // use goto break free on the loop so the server can connect to a different client
                        if (errorStatus < 0) {
                            printf("\nError: there was an error sending data to the client\n");
                            printf("Server aborting connection to current client...\n\n");
                            goto breakfree;
                        }

                    }


                    // transmit segment data if flags allow (has data, not yet recieved ack)
                    if(seg5Flag && seg5Retrans){
                        // print the block number and segment being transmitted
                        printf("Server sending client: segment number %d of block #%d\n", 5, blockCount);

                        // send a binary or text datapack depending on if we are in binary mode
                        if(replace){
                            // send datapck
                            errorStatus = sendto(serverSocket2, binPCK5, sizeof(binPCK5), 0,
                                                 (struct sockaddr *) &clientAddress,
                                                 sizeOfClientAddress);
                        }else {

                            // send datapck
                            errorStatus = sendto(serverSocket2, dataPCK5, strlen(dataPCK5), 0,
                                                 (struct sockaddr *) &clientAddress,
                                                 sizeOfClientAddress);
                        }
                        // check to see if sending was successful, if errorStatus < 0, so print error message and
                        // use goto break free on the loop so the server can connect to a different client
                        if (errorStatus < 0) {
                            printf("\nError: there was an error sending data to the client\n");
                            printf("Server aborting connection to current client...\n\n");
                            goto breakfree;
                        }

                    }


                    // transmit segment data if flags allow (has data, not yet recieved ack)
                    if(seg6Flag && seg6Retrans){
                        // print the block number and segment being transmitted
                        printf("Server sending client: segment number %d of block #%d\n", 6, blockCount);

                        // send a binary or text data pack depending on if we are in binary mode or not
                        if(replace){
                            // send datapck
                            errorStatus = sendto(serverSocket2, binPCK6, sizeof(binPCK6), 0,
                                                 (struct sockaddr *) &clientAddress,
                                                 sizeOfClientAddress);

                        }else {
                            // send datapck
                            errorStatus = sendto(serverSocket2, dataPCK6, strlen(dataPCK6), 0,
                                                 (struct sockaddr *) &clientAddress,
                                                 sizeOfClientAddress);
                        }
                        // check to see if sending was successful, if errorStatus < 0, so print error message and
                        // use goto break free on the loop so the server can connect to a different client
                        if (errorStatus < 0) {
                            printf("\nError: there was an error sending data to the client\n");
                            printf("Server aborting connection to current client...\n\n");
                            goto breakfree;
                        }

                    }


                    // transmit segment data if flags allow (has data, not yet recieved ack)
                    if(seg7Flag && seg7Retrans){
                        // print the block number and segment being transmitted
                        printf("Server sending client: segment number %d of block #%d\n", 7, blockCount);

                        // send a binary or text data pack depending on if we are in binary mode or not
                        if(replace){
                            // send datapck
                            errorStatus = sendto(serverSocket2, binPCK7, sizeof(binPCK7), 0,
                                                 (struct sockaddr *) &clientAddress,
                                                 sizeOfClientAddress);
                        }else {
                            // send datapck
                            errorStatus = sendto(serverSocket2, dataPCK7, strlen(dataPCK7), 0,
                                                 (struct sockaddr *) &clientAddress,
                                                 sizeOfClientAddress);
                        }
                        // check to see if sending was successful, if errorStatus < 0, so print error message and
                        // use goto break free on the loop so the server can connect to a different client
                        if (errorStatus < 0) {
                            printf("\nError: there was an error sending data to the client\n");
                            printf("Server aborting connection to current client...\n\n");
                            goto breakfree;
                        }

                    }

                    // transmit segment data if flags allow (has data, not yet recieved ack)
                    if(seg8Flag && seg8Retrans){
                        // print the block number and segment being transmitted
                        printf("Server sending client: segment number %d of block #%d\n", 8, blockCount);

                        // send a binary or text data pack depending on if we are in binary mode or not
                        if(replace){
                            // send datapck
                            errorStatus = sendto(serverSocket2, binPCK8, sizeof(binPCK8), 0,
                                                 (struct sockaddr *) &clientAddress,
                                                 sizeOfClientAddress);
                        }else {
                            // send datapck
                            errorStatus = sendto(serverSocket2, dataPCK8, strlen(dataPCK8), 0,
                                                 (struct sockaddr *) &clientAddress,
                                                 sizeOfClientAddress);
                        }
                        // check to see if sending was successful, if errorStatus < 0, so print error message and
                        // use goto break free on the loop so the server can connect to a different client
                        if (errorStatus < 0) {
                            printf("\nError: there was an error sending data to the client\n");
                            printf("Server aborting connection to current client...\n\n");
                            goto breakfree;
                        }

                    }


                    // wait for acks

                    // wait for ack if seg1 had data
                    if(seg1Flag){

                        // buffer to hold data ACK data from the server
                        char dataACK[maxRecv];

                        // cleaning buffers of the variable to hold incoming data
                        // resetting memory buffers of the client and server addresses
                        memset(dataACK, 0, sizeof(dataACK));

                        // recieve ack from client, will also reassign the server's port number to be the newly generated "port z" from the server
                        errorStatus = recvfrom(serverSocket2, dataACK, sizeof(dataACK), 0, (struct sockaddr *) &clientAddress,
                                               &sizeOfClientAddress);


                        // check if the data received from the client contained the error op code, if so print the error message and exit.
                        if(dataACK[ackOpIndex] == '6'){
                            // counter for error message length
                            int errorMsgLen = 0;

                            // loop through the error EFPT packet to see how big the message is, start at the first index of the error
                            // message
                            for(int i = (errOpIndex+1); i <= errStrIndex; i++){
                                // if the i-th character in the error message isn't our line separator increment the errorMsgLen
                                if (dataACK[i] != '\31'){
                                    errorMsgLen++;
                                }
                            }
                            // create a variable for the error message (+1 so we can append a '\0' later on)
                            char errMSG[errorMsgLen + 1];
                            // cleaning buffers of the variable to hold incoming data
                            memset(errMSG, 0, sizeof(errMSG));

                            // copy the error message only, from the first index (op code index + 1)
                            strncpy(errMSG, &dataACK[(errOpIndex + 1)], errorMsgLen);


                            // print the error message, close socket, and use break on the loop with go to (so a new client can be connected to)
                            printf("\n%s\n", errMSG);
                            printf("Server aborting connection with the client...\n");
                            close(serverSocket2);
                            goto breakfree;

                            // if the op code was 5 we have recieved the ack message, check to see if the reception was ok
                        }else if(dataACK[ackOpIndex] == '5') {

                            // if there were no errors with reception, process the session number and break out of the loop
                            if (errorStatus > 0) {

                                // check the session number from the client's message matches records, if not disconnect from the client.
                                if(checkSess(request, SessionNum) == 0){
                                    printf("\nError: client's session number doesn't match records...\n");
                                    printf("Server disconnecting from current client...\n");

                                    // send error to the client
                                    // create error message
                                    char errMSG[] = {
                                            "Error: the server was unable to verify the client's data transmission due to an incorrect session number being sent\n"};
                                    char errPCK[errSize + 1];
                                    // cleaning buffers of the variable to hold incoming data
                                    memset(errPCK, 0, sizeof(errPCK));
                                    generateError(errMSG, errPCK);

                                    // send error message to the server
                                    sendto(serverSocket2, errPCK, strlen(errPCK), 0, (struct sockaddr *) &clientAddress,
                                           sizeOfClientAddress);

                                    goto breakfree;
                                }

                                // process the data EFTP packet

                                // counter for block number length
                                int blockNum = 0;

                                // loop through the ack EFPT packet to see how big the session number is, start at the first index of the session
                                // number
                                for(int i = (ackSessIndex+1); i <= ackBlockIndex; i++){
                                    // if the i-th character in the session number isn't our line separator increment the sessionNumLen
                                    if (dataACK[i] != '\31'){
                                        blockNum++;
                                    }
                                }

                                char tempBlock[5];
                                // wipe buffer of variable
                                memset(tempBlock, 0, sizeof(tempBlock));

                                // copy the block number only, from the first index (op code index + 1)
                                strncpy(tempBlock, &dataACK[(ackSessIndex + 1)], blockNum);

                                // disable retransfer flag if correct
                                if((atoi(tempBlock)) == blockCount) {

                                    // now check segment number
                                    if(dataACK[dataBlockIndex+1] == '1'){
                                        // set retrans to off because ack received
                                        seg1Retrans = 0;
                                        // disable the flag
                                        seg1Flag = 0;

                                    }
                                }
                            }
                        }

                    }


                    // Create the individual data packets for each segment if their segFlags indicate they have data
                    if(seg2Flag){
                        // buffer to hold data ACK data from the server
                        char dataACK[maxRecv];

                        // cleaning buffers of the variable to hold incoming data
                        // resetting memory buffers of the client and server addresses
                        memset(dataACK, 0, sizeof(dataACK));

                        // recieve ack from client, will also reassign the server's port number to be the newly generated "port z" from the server
                        errorStatus = recvfrom(serverSocket2, dataACK, sizeof(dataACK), 0, (struct sockaddr *) &clientAddress,
                                               &sizeOfClientAddress);


                        // check if the data received from the client contained the error op code, if so print the error message and exit.
                        if(dataACK[ackOpIndex] == '6'){
                            // counter for error message length
                            int errorMsgLen = 0;

                            // loop through the error EFPT packet to see how big the message is, start at the first index of the error
                            // message
                            for(int i = (errOpIndex+1); i <= errStrIndex; i++){
                                // if the i-th character in the error message isn't our line separator increment the errorMsgLen
                                if (dataACK[i] != '\31'){
                                    errorMsgLen++;
                                }
                            }
                            // create a variable for the error message (+1 so we can append a '\0' later on)
                            char errMSG[errorMsgLen + 1];
                            // cleaning buffers of the variable to hold incoming data
                            memset(errMSG, 0, sizeof(errMSG));

                            // copy the error message only, from the first index (op code index + 1)
                            strncpy(errMSG, &dataACK[(errOpIndex + 1)], errorMsgLen);


                            // print the error message, close socket, and use break on the loop with go to (so a new client can be connected to)
                            printf("\n%s\n", errMSG);
                            printf("Server aborting connection with the client...\n");
                            close(serverSocket2);
                            goto breakfree;

                            // if the op code was 5 we have recieved the ack message, check to see if the reception was ok
                        }else if(dataACK[ackOpIndex] == '5') {

                            // if there were no errors with reception, process the session number and break out of the loop
                            if (errorStatus > 0) {

                                // check the session number from the client's message matches records, if not disconnect from the client.
                                if(checkSess(request, SessionNum) == 0){
                                    printf("\nError: client's session number doesn't match records...\n");
                                    printf("Server disconnecting from current client...\n");

                                    // send error to the client
                                    // create error message
                                    char errMSG[] = {
                                            "Error: the server was unable to verify the client's data transmission due to an incorrect session number being sent\n"};
                                    char errPCK[errSize + 1];
                                    // cleaning buffers of the variable to hold incoming data
                                    memset(errPCK, 0, sizeof(errPCK));
                                    generateError(errMSG, errPCK);

                                    // send error message to the server
                                    sendto(serverSocket2, errPCK, strlen(errPCK), 0, (struct sockaddr *) &clientAddress,
                                           sizeOfClientAddress);

                                    goto breakfree;
                                }

                                // process the data EFTP packet

                                // counter for block number length
                                int blockNum = 0;

                                // loop through the ack EFPT packet to see how big the session number is, start at the first index of the session
                                // number
                                for(int i = (ackSessIndex+1); i <= ackBlockIndex; i++){
                                    // if the i-th character in the session number isn't our line separator increment the sessionNumLen
                                    if (dataACK[i] != '\31'){
                                        blockNum++;
                                    }
                                }

                                char tempBlock[5];
                                // wipe buffer of variable
                                memset(tempBlock, 0, sizeof(tempBlock));

                                // copy the block number only, from the first index (op code index + 1)
                                strncpy(tempBlock, &dataACK[(ackSessIndex + 1)], blockNum);

                                // disable retransfer flag if correct
                                if((atoi(tempBlock)) == blockCount) {

                                    // now check segment number
                                    if(dataACK[dataBlockIndex+1] == '2'){
                                        // set retrans to off because ack received
                                        seg2Retrans = 0;
                                        // disable the flag
                                        seg2Flag = 0;

                                    }
                                }
                            }
                        }

                    }


                    // Create the individual data packets for each segment if their segFlags indicate they have data
                    if(seg3Flag){
                        // buffer to hold data ACK data from the server
                        char dataACK[maxRecv];

                        // cleaning buffers of the variable to hold incoming data
                        // resetting memory buffers of the client and server addresses
                        memset(dataACK, 0, sizeof(dataACK));

                        // recieve ack from client, will also reassign the server's port number to be the newly generated "port z" from the server
                        errorStatus = recvfrom(serverSocket2, dataACK, sizeof(dataACK), 0, (struct sockaddr *) &clientAddress,
                                               &sizeOfClientAddress);


                        // check if the data received from the client contained the error op code, if so print the error message and exit.
                        if(dataACK[ackOpIndex] == '6'){
                            // counter for error message length
                            int errorMsgLen = 0;

                            // loop through the error EFPT packet to see how big the message is, start at the first index of the error
                            // message
                            for(int i = (errOpIndex+1); i <= errStrIndex; i++){
                                // if the i-th character in the error message isn't our line separator increment the errorMsgLen
                                if (dataACK[i] != '\31'){
                                    errorMsgLen++;
                                }
                            }
                            // create a variable for the error message (+1 so we can append a '\0' later on)
                            char errMSG[errorMsgLen + 1];
                            // cleaning buffers of the variable to hold incoming data
                            memset(errMSG, 0, sizeof(errMSG));

                            // copy the error message only, from the first index (op code index + 1)
                            strncpy(errMSG, &dataACK[(errOpIndex + 1)], errorMsgLen);


                            // print the error message, close socket, and use break on the loop with go to (so a new client can be connected to)
                            printf("\n%s\n", errMSG);
                            printf("Server aborting connection with the client...\n");
                            close(serverSocket2);
                            goto breakfree;

                            // if the op code was 5 we have recieved the ack message, check to see if the reception was ok
                        }else if(dataACK[ackOpIndex] == '5') {

                            // if there were no errors with reception, process the session number and break out of the loop
                            if (errorStatus > 0) {

                                // check the session number from the client's message matches records, if not disconnect from the client.
                                if(checkSess(request, SessionNum) == 0){
                                    printf("\nError: client's session number doesn't match records...\n");
                                    printf("Server disconnecting from current client...\n");

                                    // send error to the client
                                    // create error message
                                    char errMSG[] = {
                                            "Error: the server was unable to verify the client's data transmission due to an incorrect session number being sent\n"};
                                    char errPCK[errSize + 1];
                                    // cleaning buffers of the variable to hold incoming data
                                    memset(errPCK, 0, sizeof(errPCK));
                                    generateError(errMSG, errPCK);

                                    // send error message to the server
                                    sendto(serverSocket2, errPCK, strlen(errPCK), 0, (struct sockaddr *) &clientAddress,
                                           sizeOfClientAddress);

                                    goto breakfree;
                                }


                                // process the data EFTP packet

                                // counter for block number length
                                int blockNum = 0;

                                // loop through the ack EFPT packet to see how big the session number is, start at the first index of the session
                                // number
                                for(int i = (ackSessIndex+1); i <= ackBlockIndex; i++){
                                    // if the i-th character in the session number isn't our line separator increment the sessionNumLen
                                    if (dataACK[i] != '\31'){
                                        blockNum++;
                                    }
                                }

                                char tempBlock[5];
                                // wipe buffer of variable
                                memset(tempBlock, 0, sizeof(tempBlock));

                                // copy the block number only, from the first index (op code index + 1)
                                strncpy(tempBlock, &dataACK[(ackSessIndex + 1)], blockNum);

                                // disable retransfer flag if correct
                                if((atoi(tempBlock)) == blockCount) {

                                    // now check segment number
                                    if(dataACK[dataBlockIndex+1] == '3'){
                                        // set retrans to off because ack received
                                        seg3Retrans = 0;
                                        // disable the flag
                                        seg3Flag = 0;

                                    }
                                }
                            }
                        }

                    }


                    // Create the individual data packets for each segment if their segFlags indicate they have data
                    if(seg4Flag){
                        // buffer to hold data ACK data from the server
                        char dataACK[maxRecv];

                        // cleaning buffers of the variable to hold incoming data
                        // resetting memory buffers of the client and server addresses
                        memset(dataACK, 0, sizeof(dataACK));

                        // recieve ack from client, will also reassign the server's port number to be the newly generated "port z" from the server
                        errorStatus = recvfrom(serverSocket2, dataACK, sizeof(dataACK), 0, (struct sockaddr *) &clientAddress,
                                               &sizeOfClientAddress);


                        // check if the data received from the client contained the error op code, if so print the error message and exit.
                        if(dataACK[ackOpIndex] == '6'){
                            // counter for error message length
                            int errorMsgLen = 0;

                            // loop through the error EFPT packet to see how big the message is, start at the first index of the error
                            // message
                            for(int i = (errOpIndex+1); i <= errStrIndex; i++){
                                // if the i-th character in the error message isn't our line separator increment the errorMsgLen
                                if (dataACK[i] != '\31'){
                                    errorMsgLen++;
                                }
                            }
                            // create a variable for the error message (+1 so we can append a '\0' later on)
                            char errMSG[errorMsgLen + 1];
                            // cleaning buffers of the variable to hold incoming data
                            memset(errMSG, 0, sizeof(errMSG));

                            // copy the error message only, from the first index (op code index + 1)
                            strncpy(errMSG, &dataACK[(errOpIndex + 1)], errorMsgLen);


                            // print the error message, close socket, and use break on the loop with go to (so a new client can be connected to)
                            printf("\n%s\n", errMSG);
                            printf("Server aborting connection with the client...\n");
                            close(serverSocket2);
                            goto breakfree;

                            // if the op code was 5 we have recieved the ack message, check to see if the reception was ok
                        }else if(dataACK[ackOpIndex] == '5') {

                            // if there were no errors with reception, process the session number and break out of the loop
                            if (errorStatus > 0) {

                                // process the data EFTP packet

                                // counter for block number length
                                int blockNum = 0;

                                // loop through the ack EFPT packet to see how big the session number is, start at the first index of the session
                                // number
                                for(int i = (ackSessIndex+1); i <= ackBlockIndex; i++){
                                    // if the i-th character in the session number isn't our line separator increment the sessionNumLen
                                    if (dataACK[i] != '\31'){
                                        blockNum++;
                                    }
                                }

                                char tempBlock[5];
                                // wipe buffer of variable
                                memset(tempBlock, 0, sizeof(tempBlock));

                                // copy the block number only, from the first index (op code index + 1)
                                strncpy(tempBlock, &dataACK[(ackSessIndex + 1)], blockNum);

                                // disable retransfer flag if correct
                                if((atoi(tempBlock)) == blockCount) {

                                    // now check segment number
                                    if(dataACK[dataBlockIndex+1] == '4'){
                                        // set retrans to off because ack received
                                        seg4Retrans = 0;
                                        // disable the flag
                                        seg4Flag = 0;

                                    }
                                }
                            }
                        }

                    }


                    // Create the individual data packets for each segment if their segFlags indicate they have data
                    if(seg5Flag){
                        // buffer to hold data ACK data from the server
                        char dataACK[maxRecv];

                        // cleaning buffers of the variable to hold incoming data
                        // resetting memory buffers of the client and server addresses
                        memset(dataACK, 0, sizeof(dataACK));

                        // recieve ack from client, will also reassign the server's port number to be the newly generated "port z" from the server
                        errorStatus = recvfrom(serverSocket2, dataACK, sizeof(dataACK), 0, (struct sockaddr *) &clientAddress,
                                               &sizeOfClientAddress);


                        // check if the data received from the client contained the error op code, if so print the error message and exit.
                        if(dataACK[ackOpIndex] == '6'){
                            // counter for error message length
                            int errorMsgLen = 0;

                            // loop through the error EFPT packet to see how big the message is, start at the first index of the error
                            // message
                            for(int i = (errOpIndex+1); i <= errStrIndex; i++){
                                // if the i-th character in the error message isn't our line separator increment the errorMsgLen
                                if (dataACK[i] != '\31'){
                                    errorMsgLen++;
                                }
                            }
                            // create a variable for the error message (+1 so we can append a '\0' later on)
                            char errMSG[errorMsgLen + 1];
                            // cleaning buffers of the variable to hold incoming data
                            memset(errMSG, 0, sizeof(errMSG));

                            // copy the error message only, from the first index (op code index + 1)
                            strncpy(errMSG, &dataACK[(errOpIndex + 1)], errorMsgLen);


                            // print the error message, close socket, and use break on the loop with go to (so a new client can be connected to)
                            printf("\n%s\n", errMSG);
                            printf("Server aborting connection with the client...\n");
                            close(serverSocket2);
                            goto breakfree;

                            // if the op code was 5 we have recieved the ack message, check to see if the reception was ok
                        }else if(dataACK[ackOpIndex] == '5') {

                            // if there were no errors with reception, process the session number and break out of the loop
                            if (errorStatus > 0) {


                                // check the session number from the client's message matches records, if not disconnect from the client.
                                if(checkSess(request, SessionNum) == 0){
                                    printf("\nError: client's session number doesn't match records...\n");
                                    printf("Server disconnecting from current client...\n");

                                    // send error to the client
                                    // create error message
                                    char errMSG[] = {
                                            "Error: the server was unable to verify the client's data transmission due to an incorrect session number being sent\n"};
                                    char errPCK[errSize + 1];
                                    // cleaning buffers of the variable to hold incoming data
                                    memset(errPCK, 0, sizeof(errPCK));
                                    generateError(errMSG, errPCK);

                                    // send error message to the server
                                    sendto(serverSocket2, errPCK, strlen(errPCK), 0, (struct sockaddr *) &clientAddress,
                                           sizeOfClientAddress);

                                    goto breakfree;
                                }

                                // process the data EFTP packet

                                // counter for block number length
                                int blockNum = 0;

                                // loop through the ack EFPT packet to see how big the session number is, start at the first index of the session
                                // number
                                for(int i = (ackSessIndex+1); i <= ackBlockIndex; i++){
                                    // if the i-th character in the session number isn't our line separator increment the sessionNumLen
                                    if (dataACK[i] != '\31'){
                                        blockNum++;
                                    }
                                }

                                char tempBlock[5];
                                // wipe buffer of variable
                                memset(tempBlock, 0, sizeof(tempBlock));

                                // copy the block number only, from the first index (op code index + 1)
                                strncpy(tempBlock, &dataACK[(ackSessIndex + 1)], blockNum);

                                // disable retransfer flag if correct
                                if((atoi(tempBlock)) == blockCount) {

                                    // now check segment number
                                    if(dataACK[dataBlockIndex+1] == '5'){
                                        // set retrans to off because ack received
                                        seg5Retrans = 0;
                                        // disable the flag
                                        seg5Flag = 0;

                                    }
                                }
                            }
                        }
                    }


                    // Create the individual data packets for each segment if their segFlags indicate they have data
                    if(seg6Flag){

                        // buffer to hold data ACK data from the server
                        char dataACK[maxRecv];

                        // cleaning buffers of the variable to hold incoming data
                        // resetting memory buffers of the client and server addresses
                        memset(dataACK, 0, sizeof(dataACK));

                        // recieve ack from client, will also reassign the server's port number to be the newly generated "port z" from the server
                        errorStatus = recvfrom(serverSocket2, dataACK, sizeof(dataACK), 0, (struct sockaddr *) &clientAddress,
                                               &sizeOfClientAddress);


                        // check if the data received from the client contained the error op code, if so print the error message and exit.
                        if(dataACK[ackOpIndex] == '6'){
                            // counter for error message length
                            int errorMsgLen = 0;

                            // loop through the error EFPT packet to see how big the message is, start at the first index of the error
                            // message
                            for(int i = (errOpIndex+1); i <= errStrIndex; i++){
                                // if the i-th character in the error message isn't our line separator increment the errorMsgLen
                                if (dataACK[i] != '\31'){
                                    errorMsgLen++;
                                }
                            }
                            // create a variable for the error message (+1 so we can append a '\0' later on)
                            char errMSG[errorMsgLen + 1];
                            // cleaning buffers of the variable to hold incoming data
                            memset(errMSG, 0, sizeof(errMSG));

                            // copy the error message only, from the first index (op code index + 1)
                            strncpy(errMSG, &dataACK[(errOpIndex + 1)], errorMsgLen);


                            // print the error message, close socket, and use break on the loop with go to (so a new client can be connected to)
                            printf("\n%s\n", errMSG);
                            printf("Server aborting connection with the client...\n");
                            close(serverSocket2);
                            goto breakfree;

                            // if the op code was 5 we have recieved the ack message, check to see if the reception was ok
                        }else if(dataACK[ackOpIndex] == '5') {

                            // if there were no errors with reception, process the session number and break out of the loop
                            if (errorStatus > 0) {

                                // check the session number from the client's message matches records, if not disconnect from the client.
                                if(checkSess(request, SessionNum) == 0){
                                    printf("\nError: client's session number doesn't match records...\n");
                                    printf("Server disconnecting from current client...\n");

                                    // send error to the client
                                    // create error message
                                    char errMSG[] = {
                                            "Error: the server was unable to verify the client's data transmission due to an incorrect session number being sent\n"};
                                    char errPCK[errSize + 1];
                                    // cleaning buffers of the variable to hold incoming data
                                    memset(errPCK, 0, sizeof(errPCK));
                                    generateError(errMSG, errPCK);

                                    // send error message to the server
                                    sendto(serverSocket2, errPCK, strlen(errPCK), 0, (struct sockaddr *) &clientAddress,
                                           sizeOfClientAddress);

                                    goto breakfree;
                                }

                                // process the data EFTP packet

                                // counter for block number length
                                int blockNum = 0;

                                // loop through the ack EFPT packet to see how big the session number is, start at the first index of the session
                                // number
                                for(int i = (ackSessIndex+1); i <= ackBlockIndex; i++){
                                    // if the i-th character in the session number isn't our line separator increment the sessionNumLen
                                    if (dataACK[i] != '\31'){
                                        blockNum++;
                                    }
                                }

                                char tempBlock[5];
                                // wipe buffer of variable
                                memset(tempBlock, 0, sizeof(tempBlock));

                                // copy the block number only, from the first index (op code index + 1)
                                strncpy(tempBlock, &dataACK[(ackSessIndex + 1)], blockNum);

                                // disable retransfer flag if correct
                                if((atoi(tempBlock)) == blockCount) {

                                    // now check segment number
                                    if(dataACK[dataBlockIndex+1] == '6'){
                                        // set retrans to off because ack received
                                        seg6Retrans = 0;
                                        // disable the flag
                                        seg6Flag = 0;

                                    }
                                }
                            }
                        }

                    }


                    // Create the individual data packets for each segment if their segFlags indicate they have data
                    if(seg7Flag){

                        // buffer to hold data ACK data from the server
                        char dataACK[maxRecv];

                        // cleaning buffers of the variable to hold incoming data
                        // resetting memory buffers of the client and server addresses
                        memset(dataACK, 0, sizeof(dataACK));

                        // recieve ack from client, will also reassign the server's port number to be the newly generated "port z" from the server
                        errorStatus = recvfrom(serverSocket2, dataACK, sizeof(dataACK), 0, (struct sockaddr *) &clientAddress,
                                               &sizeOfClientAddress);


                        // check if the data received from the client contained the error op code, if so print the error message and exit.
                        if(dataACK[ackOpIndex] == '6'){
                            // counter for error message length
                            int errorMsgLen = 0;

                            // loop through the error EFPT packet to see how big the message is, start at the first index of the error
                            // message
                            for(int i = (errOpIndex+1); i <= errStrIndex; i++){
                                // if the i-th character in the error message isn't our line separator increment the errorMsgLen
                                if (dataACK[i] != '\31'){
                                    errorMsgLen++;
                                }
                            }
                            // create a variable for the error message (+1 so we can append a '\0' later on)
                            char errMSG[errorMsgLen + 1];
                            // cleaning buffers of the variable to hold incoming data
                            memset(errMSG, 0, sizeof(errMSG));

                            // copy the error message only, from the first index (op code index + 1)
                            strncpy(errMSG, &dataACK[(errOpIndex + 1)], errorMsgLen);


                            // print the error message, close socket, and use break on the loop with go to (so a new client can be connected to)
                            printf("\n%s\n", errMSG);
                            printf("Server aborting connection with the client...\n");
                            close(serverSocket2);
                            goto breakfree;

                            // if the op code was 5 we have recieved the ack message, check to see if the reception was ok
                        }else if(dataACK[ackOpIndex] == '5') {

                            // if there were no errors with reception, process the session number and break out of the loop
                            if (errorStatus > 0) {

                                // check the session number from the client's message matches records, if not disconnect from the client.
                                if(checkSess(request, SessionNum) == 0){
                                    printf("\nError: client's session number doesn't match records...\n");
                                    printf("Server disconnecting from current client...\n");

                                    // send error to the client
                                    // create error message
                                    char errMSG[] = {
                                            "Error: the server was unable to verify the client's data transmission due to an incorrect session number being sent\n"};
                                    char errPCK[errSize + 1];
                                    // cleaning buffers of the variable to hold incoming data
                                    memset(errPCK, 0, sizeof(errPCK));
                                    generateError(errMSG, errPCK);

                                    // send error message to the server
                                    sendto(serverSocket2, errPCK, strlen(errPCK), 0, (struct sockaddr *) &clientAddress,
                                           sizeOfClientAddress);

                                    goto breakfree;
                                }

                                // process the data EFTP packet

                                // counter for block number length
                                int blockNum = 0;

                                // loop through the ack EFPT packet to see how big the session number is, start at the first index of the session
                                // number
                                for(int i = (ackSessIndex+1); i <= ackBlockIndex; i++){
                                    // if the i-th character in the session number isn't our line separator increment the sessionNumLen
                                    if (dataACK[i] != '\31'){
                                        blockNum++;
                                    }
                                }

                                char tempBlock[5];
                                // wipe buffer of variable
                                memset(tempBlock, 0, sizeof(tempBlock));

                                // copy the block number only, from the first index (op code index + 1)
                                strncpy(tempBlock, &dataACK[(ackSessIndex + 1)], blockNum);

                                // disable retransfer flag if correct
                                if((atoi(tempBlock)) == blockCount) {

                                    // now check segment number
                                    if(dataACK[dataBlockIndex+1] == '7'){
                                        // set retrans to off because ack received
                                        seg7Retrans = 0;
                                        // disable the flag
                                        seg7Flag = 0;

                                    }
                                }
                            }
                        }

                    }

                    // Create the individual data packets for each segment if their segFlags indicate they have data
                    if(seg8Flag){

                        // buffer to hold data ACK data from the server
                        char dataACK[maxRecv];

                        // cleaning buffers of the variable to hold incoming data
                        // resetting memory buffers of the client and server addresses
                        memset(dataACK, 0, sizeof(dataACK));

                        // recieve ack from client, will also reassign the server's port number to be the newly generated "port z" from the server
                        errorStatus = recvfrom(serverSocket2, dataACK, sizeof(dataACK), 0, (struct sockaddr *) &clientAddress,
                                               &sizeOfClientAddress);


                        // check if the data received from the client contained the error op code, if so print the error message and exit.
                        if(dataACK[ackOpIndex] == '6'){
                            // counter for error message length
                            int errorMsgLen = 0;

                            // loop through the error EFPT packet to see how big the message is, start at the first index of the error
                            // message
                            for(int i = (errOpIndex+1); i <= errStrIndex; i++){
                                // if the i-th character in the error message isn't our line separator increment the errorMsgLen
                                if (dataACK[i] != '\31'){
                                    errorMsgLen++;
                                }
                            }
                            // create a variable for the error message (+1 so we can append a '\0' later on)
                            char errMSG[errorMsgLen + 1];
                            // cleaning buffers of the variable to hold incoming data
                            memset(errMSG, 0, sizeof(errMSG));

                            // copy the error message only, from the first index (op code index + 1)
                            strncpy(errMSG, &dataACK[(errOpIndex + 1)], errorMsgLen);


                            // print the error message, close socket, and use break on the loop with go to (so a new client can be connected to)
                            printf("\n%s\n", errMSG);
                            printf("Server aborting connection with the client...\n");
                            close(serverSocket2);
                            goto breakfree;

                            // if the op code was 5 we have recieved the ack message, check to see if the reception was ok
                        }else if(dataACK[ackOpIndex] == '5') {

                            // if there were no errors with reception, process the session number and break out of the loop
                            if (errorStatus > 0) {

                                // check the session number from the client's message matches records, if not disconnect from the client.
                                if(checkSess(request, SessionNum) == 0){
                                    printf("\nError: client's session number doesn't match records...\n");
                                    printf("Server disconnecting from current client...\n");

                                    // send error to the client
                                    // create error message
                                    char errMSG[] = {
                                            "Error: the server was unable to verify the client's data transmission due to an incorrect session number being sent\n"};
                                    char errPCK[errSize + 1];
                                    // cleaning buffers of the variable to hold incoming data
                                    memset(errPCK, 0, sizeof(errPCK));
                                    generateError(errMSG, errPCK);

                                    // send error message to the server
                                    sendto(serverSocket2, errPCK, strlen(errPCK), 0, (struct sockaddr *) &clientAddress,
                                           sizeOfClientAddress);

                                    goto breakfree;
                                }

                                // process the data EFTP packet

                                // counter for block number length
                                int blockNum = 0;

                                // loop through the ack EFPT packet to see how big the session number is, start at the first index of the session
                                // number
                                for(int i = (ackSessIndex+1); i <= ackBlockIndex; i++){
                                    // if the i-th character in the session number isn't our line separator increment the sessionNumLen
                                    if (dataACK[i] != '\31'){
                                        blockNum++;
                                    }
                                }

                                char tempBlock[5];
                                // wipe buffer of variable
                                memset(tempBlock, 0, sizeof(tempBlock));

                                // copy the block number only, from the first index (op code index + 1)
                                strncpy(tempBlock, &dataACK[(ackSessIndex + 1)], blockNum);

                                // disable retransfer flag if correct
                                if((atoi(tempBlock)) == blockCount) {

                                    // now check segment number
                                    if(dataACK[dataBlockIndex+1] == '8'){
                                        // set retrans to off because ack received
                                        seg8Retrans = 0;
                                        // disable the flag
                                        seg8Flag = 0;

                                    }
                                }
                            }
                        }

                    }

                }



                // cleaning buffers of the variable to hold incoming data
                memset(block, 0, sizeof(block));
                blockCount++;
            }

            // if the amount of data sent (accumulator) is a multiple of 1KB we will send an empty data packet
            if((accumulator % 1024) == 0){
                // determine which segment number to use

                // 1:  1024 / 1024 = 1 --> send 2
                // 2:  x  /  1024 = 2 --> send 3

                //  ...

                // 8: x / 1024 = 8 --> send 1

                // variable to hold the segment number to be sent
                int segToSend;

                if((accumulator / 1024) == 1){
                    segToSend = 2;

                }else if((accumulator / 1024) == 2){
                    segToSend = 3;

                }else if((accumulator / 1024) == 3){
                    segToSend = 4;

                }else if ((accumulator / 1024) == 4){
                    segToSend = 5;

                }else if((accumulator / 1024) == 5){
                    segToSend = 6;

                }else if((accumulator / 1024) == 6){
                    segToSend = 7;

                }else if((accumulator / 1024) == 7){
                    segToSend = 8;

                }else{
                    segToSend = 1;
                }


                // convert the segment back into a string
                char segToSendStr [1];
                sprintf(segToSendStr, "%d", segToSend);




                char emptyData[dataSize];
                // clearing buffers of empty data
                memset(emptyData, 0, sizeof(emptyData));
                // create an empty data packet
                generateData("00000", blockCountStr, segToSendStr, "", emptyData);

                // send datapck
                errorStatus = sendto(serverSocket2, emptyData, strlen(emptyData), 0, (struct sockaddr *) &clientAddress,
                                     sizeOfClientAddress);

                // check to see if sending was successful, if errorStatus < 0, so print error message and
                // use goto break free on the loop so the server can connect to a different client
                if (errorStatus < 0) {
                    printf("\nError: there was an error sending data to the client\n");
                    printf("Server aborting connection to current client...\n\n");
                    goto breakfree;
                }
            }


            // close file
            fclose(filePtr);









            // ------- WRQ mode ------ //


            // otherwise mode == 0 and it is in WRQ mode
        }else{

            // sending ack

            // create variable to hold the ack structure
            char ackStruct[ackSize + 1];
            // cleaning buffers of the variable to hold incoming data
            memset(ackStruct, 0, sizeof(ackStruct));
            // get the ack packet
            generateACK("00000", "1", "000", ackStruct);
            // send the auth packet to the server
            errorStatus = sendto(serverSocket2, ackStruct, strlen(ackStruct), 0,
                                 (struct sockaddr *) &clientAddress,
                                 sizeOfClientAddress);

            // check if there was an error sending to the server, if so errorStatus < 0, then print an error message
            // and exit with error code 1
            if (errorStatus < 0) {
                printf("\nError unable to send authorization data to the client!\n");
                printf("Server aborting connection to client...\n");
                goto WRQBreak;
            }


            // Print saying the server is downloading the file
            printf("Server beginning data download for the file called: %s\n", fileName);

            // create a variable to hold the opening mode for FOPEN
            char openMode[3];
            // cleaning buffers of the variable to hold incoming data
            memset(openMode, 0, sizeof(openMode));

            // flag to control replacement of '\0' characters in binary transfers only (0 = false, 1 = true and enabled)
            int replace = 0;

            // index the last 3 characters in a file name to see if it has the txt extension, if so it is a text file
            if ((fileName[strlen(fileName) - 1] == 't') && ((fileName[strlen(fileName) - 2] == 'x')) &&
                (fileName[strlen(fileName) - 3] == 't')) {
                // make the opening mode w if the text file is binary, w because we will be writing to the file in RRQ mode
                strcpy(openMode, "w");

                // otherwise make the opening mode wb for binary files wb because we will be writing to the file in RRQ mode
            } else {
                strcpy(openMode, "wb");
                replace = 1;
            }

            // file pointer and file creation
            // creating a pointer to open an output file (same file name as provided by command line) to write data from the server to
            FILE *outFile = fopen(fileName, openMode);

            // check to see if the outFile pointer is null, if it is that means fopen was unable to create the file
            // so print an error message and exit the program with error code 1
            // also send error to the server
            if (outFile == NULL) {
                printf("Error: Server could not create an output file called \"%s\" to save data from the client to\n",
                       fileName);
                printf("Server aborting connection with client...\n");

                // create error message
                char errMSG[] = {"Error: the server was unable to create a storage file, and connection was terminated\n"};
                char errPCK[errSize + 1];
                // cleaning buffers of the variable to hold incoming data
                memset(errPCK, 0, sizeof(errPCK));
                generateError(errMSG, errPCK);

                // send error message to the server
                sendto(serverSocket2, errPCK, strlen(errPCK), 0, (struct sockaddr *) &clientAddress,
                       sizeOfClientAddress);
                goto WRQBreak;
            }


            //  Data transmission phase

            // print message that the data transfer phase is beginning
            printf("Beginning data transfer...\n");

            // variable to control the loop, will be changed to 0 when the end of the file has been received
            int control = 1;
            // loop to get all of the blocks of data
            while (control) {
                // check for segment < full size (can be empty) which means end of file

                // variables to hold the 8 segments of data
                char segment1[1024], segment2[1024], segment3[1024], segment4[1024], segment5[1024],
                        segment6[1024], segment7[1024], segment8[1024];

                // cleaning buffers of the variable to hold incoming data
                memset(segment1, 0, sizeof(segment1));
                memset(segment2, 0, sizeof(segment2));
                memset(segment3, 0, sizeof(segment3));
                memset(segment4, 0, sizeof(segment4));
                memset(segment5, 0, sizeof(segment5));
                memset(segment6, 0, sizeof(segment6));
                memset(segment7, 0, sizeof(segment7));
                memset(segment8, 0, sizeof(segment8));

                // variable to contain the segment number of the last segment with data
                int lastSeg = 0;
                // variable to contain the index of the last data position in the last segment
                int lastSegDataIndex;

                // setting socket to time out if nothing is there is inactivity for 20 seconds (this way the client has time to resend segments 3x)
                struct timeval timeout5sec = {5, 0};
                setsockopt(serverSocket2, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout5sec, sizeof(struct timeval));

                // variable to hold block number
                char blockNum[6];

                // cleaning buffers of the variable to hold incoming data
                memset(blockNum, 0, sizeof(blockNum));

                // initialize blockNum to null
                strcpy(blockNum, "null");

                // variables to act like flags to indicate if a segment has its data yet, 1 = no data, 0 = has data
                int seg1Flag = 1, seg2Flag = 1, seg3Flag = 1, seg4Flag = 1, seg5Flag = 1, seg6Flag = 1, seg7Flag = 1, seg8Flag = 1;

                // variables to hold the index of where the data section ends on these segments
                int seg1DataIndex = 0, seg2DataIndex = 0, seg3DataIndex = 0, seg4DataIndex = 0, seg5DataIndex = 0, seg6DataIndex = 0, seg7DataIndex = 0, seg8DataIndex = 0;

                //loop to get all of the segments of data
                while (seg1Flag || seg2Flag || seg3Flag || seg4Flag || seg5Flag || seg6Flag || seg7Flag || seg8Flag) {

                    // prepare to break the loop if we have a segment flag and all of its prior segments and their flags not being 0
                    if (lastSeg != 0) {
                        if(lastSeg == 1){
                            break;

                        }else if(lastSeg == 2){
                            if(seg1Flag != 1){
                                break;
                            }

                        }else if (lastSeg == 3){

                            if((seg1Flag != 1) && (seg2Flag != 1)){
                                break;
                            }

                        }else if (lastSeg == 4){
                            if((seg1Flag != 1) && (seg2Flag != 1) && (seg3Flag !=1)){
                                break;
                            }

                        }else if (lastSeg == 5){
                            if((seg1Flag != 1) && (seg2Flag != 1) && (seg3Flag !=1) && (seg4Flag !=1)){
                                break;
                            }

                        }else if (lastSeg == 6){
                            if((seg1Flag != 1) && (seg2Flag != 1) && (seg3Flag !=1) && (seg4Flag !=1) && (seg5Flag !=1)){
                                break;
                            }

                        }else if (lastSeg == 7){
                            if((seg1Flag != 1) && (seg2Flag != 1) && (seg3Flag !=1) && (seg4Flag !=1) && (seg5Flag !=1) & (seg6Flag !=1)){
                                break;
                            }

                        }else if (lastSeg == 8){
                            if((seg1Flag != 1) && (seg2Flag != 1) && (seg3Flag !=1) && (seg4Flag !=1) && (seg5Flag !=1) & (seg6Flag !=1) && (seg7Flag) != 1){
                                break;
                            }
                        }
                    }
                    // variable to hold the actual size of the incoming data
                    int dataSZE = 0;

                    // variable to hold incoming data
                    char incomingData[maxRecv];
                    // cleaning buffers of the variable to hold incoming data
                    memset(incomingData, 0, sizeof(incomingData));


                    // recieve data segment from server, it will automatically use the new socket with port z from the server
                    errorStatus = recvfrom(serverSocket2, incomingData, sizeof(incomingData), 0,
                                           (struct sockaddr *) &clientAddress,&sizeOfClientAddress);


                    // if execution reaches here there is a problem receiving from the socket so print an error message, and
                    // then send an error to the server before exiting with error code 1
                    if (errorStatus <= 0) {
                        printf("\nError: unable to recieve data from the client\n");
                        printf("Server aborting connection with client...\n");

                        // create error message
                        char errMSG[] = {
                                "Error: the server was unable to receive any data from its socket and had to abort the connection\n"};
                        char errPCK[errSize + 1];
                        // cleaning buffers of the variable to hold incoming data
                        memset(errPCK, 0, sizeof(errPCK));
                        generateError(errMSG, errPCK);

                        // send error message to the server
                        sendto(serverSocket2, errPCK, strlen(errPCK), 0, (struct sockaddr *) &clientAddress,
                               sizeOfClientAddress);

                        goto WRQBreak;
                    }

                    // if op code is 6 print the error from the server
                    if (incomingData[dataOpIndex] == '6'){

                        // counter for error message length
                        int errorMsgLen = 0;

                        // loop through the error EFPT packet to see how big the message is, start at the first index of the error
                        // message
                        for(int i = (errOpIndex+1); i <= errStrIndex; i++){
                            // if the i-th character in the error message isn't our line separator increment the errorMsgLen
                            if (incomingData[i] != '\31'){
                                errorMsgLen++;
                            }
                        }
                        // create a variable for the error message (+1 so we can append a '\0' later on)
                        char errMSG[errorMsgLen + 1];
                        // cleaning buffers of the variable to hold incoming data
                        memset(errMSG, 0, sizeof(errMSG));

                        // copy the error message only, from the first index (op code index + 1)
                        strncpy(errMSG, &incomingData[(errOpIndex + 1)], errorMsgLen);


                        // print the error message, close socket, and exit the program with error code 1
                        printf("\n%s\n", errMSG);
                        printf("Server aborting connection with the Client...\n");
                        close(serverSocket2);
                        goto WRQBreak;

                    }

                    // if op code is 4 we have
                    if (incomingData[dataOpIndex] == '4') {

                        // get block number

                        // block number coubter
                        int blockCounter = 0;
                        // loop through the block number
                        for (int i = (dataSessIndex + 1); i <= dataBlockIndex; i++) {
                            // if the i-th character in the session number isn't our line separator increment the sessionNumLen
                            if (incomingData[i] != '\31') {
                                blockCounter++;
                            }
                        }

                        // block number variable for internal use only
                        char blockNum2[6];
                        // clearing the buffer
                        memset(blockNum2, 0, sizeof(blockNum2));

                        // copy the block number only, from the first index (session # index + 1)
                        strncpy(blockNum2, &incomingData[(dataSessIndex + 1)], blockCounter);

                        // see if external block number is equal to null, if so it is the first iteration so just assign this block number to it
                        if (strcmp(blockNum, "null") == 0) {
                            // cleaning buffers of the variable to hold incoming data
                            memset(blockNum, 0, sizeof(blockNum));
                            // copy the block number only, from the first index (session # index + 1) to the external block number
                            strncpy(blockNum, &incomingData[(dataSessIndex + 1)], blockCounter);

                            // otherwise check if the block number currently sent is different print an error and send an error message to the server
                        } else {
                            if (strcmp(blockNum, blockNum2) != 0) {
                                printf("\nError: The client has sent corrupted data (block out of order).\n");
                                printf("Server aborting connection to the Client...\n");

                                // create error message
                                char errMSG[] = {
                                        "Error: the client sent a block out of order\n"};
                                char errPCK[errSize + 1];
                                // cleaning buffers of the variable to hold incoming data
                                memset(errPCK, 0, sizeof(errPCK));
                                generateError(errMSG, errPCK);

                                // send error message to the server
                                sendto(serverSocket2, errPCK, strlen(errPCK), 0, (struct sockaddr *) &clientAddress,
                                       sizeOfClientAddress);

                                goto WRQBreak;
                            }
                        }



                        // if in binary mode get the data size which is stored in the session number
                        if(replace) {
                            // session number counter
                            int sessCounter = 0;
                            // loop through the session number
                            for (int i = 2; i <= dataSessIndex; i++) {
                                // if the i-th character in the session number isn't our line separator increment the sessionNumLen
                                if (incomingData[i] != '\31') {
                                    sessCounter++;
                                }
                            }
                            // temporary variable to hold the data size string
                            char tempDataSZE[5];
                            // clearing buffer for incoming data.
                            memset(tempDataSZE, 0, sizeof(tempDataSZE));
                            // get the data size stored in the session number
                            strncpy(tempDataSZE, &incomingData[2], sessCounter);
                            // conver the data size to an integer
                            dataSZE = atoi(tempDataSZE);
                        }



                        // process the first segment
                        if (incomingData[dataBlockIndex + 1] == '1') {
                            // disable segment 1 flag
                            seg1Flag = 0;

                            // counter for how long the data segment is
                            int dataCounter = 0;

                            // perform different things depending on if we are in binary mode or not
                            if(replace){
                                seg1DataIndex = dataSZE;
                                dataCounter = dataSZE;
                                memcpy(segment1, &incomingData[(dataSegIndex + 1)], dataCounter);

                            }else{
                                // loop through the data segment of the data packet
                                for (int i = (dataSegIndex + 1); i <= dataDataIndex; i++) {
                                    // if the i-th character in the session number isn't our line separator increment the sessionNumLen
                                    if (incomingData[i] != '\31') {
                                        dataCounter++;
                                    }
                                }
                                // set data index
                                seg1DataIndex = dataCounter;

                                // copy the data only, from the first index (segment # index + 1)
                                strncpy(segment1, &incomingData[(dataSegIndex + 1)], dataCounter);

                            }


                            // check to see if data counter is less than 1024, if so it is the last segment
                            if (dataCounter < 1024) {
                                // disable the out most loop control
                                control = 0;
                                // make the last segment variable equal to this segment
                                lastSeg = 1;
                                // make the last segment data  index equal to the data counter
                                lastSegDataIndex = dataCounter;
                            }

                            // send ack for this current segment
                            // print attempt number of loop
                            printf("Sending block #%d, segment #%d acknowledgement to the client\n", (atoi(blockNum)), 1);

                            // create variable to hold the ack structure
                            char ackStruct[authSize + 1];
                            // cleaning buffers of the variable to hold incoming data
                            memset(ackStruct, 0, sizeof(ackStruct));
                            // get the ack packet
                            generateACK("00000", blockNum, "1", ackStruct);
                            // send the auth packet to the server
                            errorStatus = sendto(serverSocket2, ackStruct, strlen(ackStruct), 0,
                                                 (struct sockaddr *) &clientAddress,
                                                 sizeOfClientAddress);

                            // check if there was an error sending to the server, if so errorStatus < 0, then print an error message
                            // and exit with error code 1
                            if (errorStatus < 0) {
                                printf("\nError unable to send data acknowledgement data to the client!\n");
                                printf("Server aborting connection to the Client...\n");
                                goto WRQBreak;
                            }


                            // process the second segment
                        } else if (incomingData[dataBlockIndex + 1] == '2') {
                            // disable segment 2 flag
                            seg2Flag = 0;

                            // counter for how long the data segment is
                            int dataCounter = 0;

                            // perform different things depending on if we are in binary mode or not
                            if(replace){
                                seg2DataIndex = dataSZE;
                                dataCounter = dataSZE;
                                // copy the data only, from the first index (segment # index + 1)
                                memcpy(segment2, &incomingData[(dataSegIndex + 1)], dataCounter);

                            }else {

                                // loop through the data segment of the data packet
                                for (int i = (dataSegIndex + 1); i <= dataDataIndex; i++) {
                                    // if the i-th character in the session number isn't our line separator increment the sessionNumLen
                                    if (incomingData[i] != '\31') {
                                        dataCounter++;
                                    }
                                }
                                // set data index
                                seg2DataIndex = dataCounter;

                                // copy the data only, from the first index (segment # index + 1)
                                strncpy(segment2, &incomingData[(dataSegIndex + 1)], dataCounter);
                            }

                            // check to see if data counter is less than 1024, if so it is the last segment
                            if (dataCounter < 1024) {
                                // disable the out most loop control
                                control = 0;
                                // make the last segment variable equal to this segment
                                lastSeg = 2;
                                // make the last segment data  index equal to the data counter
                                lastSegDataIndex = dataCounter;
                            }

                            // send ack for this current segment
                            // print attempt number of loop
                            printf("Sending block #%d, segment #%d acknowledgement to the client\n", (atoi(blockNum)),2);

                            // create variable to hold the ack structure
                            char ackStruct[authSize + 1];
                            // cleaning buffers of the variable to hold incoming data
                            memset(ackStruct, 0, sizeof(ackStruct));
                            // get the ack packet
                            generateACK("00000", blockNum, "2", ackStruct);
                            // send the auth packet to the server
                            errorStatus = sendto(serverSocket2, ackStruct, strlen(ackStruct), 0,
                                                 (struct sockaddr *) &clientAddress,
                                                 sizeOfClientAddress);

                            // check if there was an error sending to the server, if so errorStatus < 0, then print an error message
                            // and exit with error code 1
                            if (errorStatus < 0) {
                                printf("\nError unable to send data acknowledgement data to the client!\n");
                                printf("Server aborting connection to the client...\n");
                                goto  WRQBreak;
                            }


                            // process the third segment
                        } else if (incomingData[dataBlockIndex + 1] == '3') {
                            // disable segment 3 flag
                            seg3Flag = 0;

                            // counter for how long the data segment is
                            int dataCounter = 0;

                            // perform different things depending on if we are in binary mode or not
                            if(replace){
                                seg3DataIndex = dataSZE;
                                dataCounter = dataSZE;
                                // copy the data only, from the first index (segment # index + 1)
                                memcpy(segment3, &incomingData[(dataSegIndex + 1)], dataCounter);

                            }else {

                                // loop through the data segment of the data packet
                                for (int i = (dataSegIndex + 1); i <= dataDataIndex; i++) {
                                    // if the i-th character in the session number isn't our line separator increment the sessionNumLen
                                    if (incomingData[i] != '\31') {
                                        dataCounter++;
                                    }
                                }
                                // set data index
                                seg3DataIndex = dataCounter;

                                // copy the data only, from the first index (segment # index + 1)
                                strncpy(segment3, &incomingData[(dataSegIndex + 1)], dataCounter);
                            }

                            // check to see if data counter is less than 1024, if so it is the last segment
                            if (dataCounter < 1024) {
                                // disable the out most loop control
                                control = 0;
                                // make the last segment variable equal to this segment
                                lastSeg = 3;
                                // make the last segment data  index equal to the data counter
                                lastSegDataIndex = dataCounter;
                            }

                            // send ack for this current segment
                            // print attempt number of loop
                            printf("Sending block #%d, segment #%d acknowledgement to the client\n", (atoi(blockNum)), 3);

                            // create variable to hold the ack structure
                            char ackStruct[authSize + 1];
                            // cleaning buffers of the variable to hold incoming data
                            memset(ackStruct, 0, sizeof(ackStruct));
                            // get the ack packet
                            generateACK("00000", blockNum, "3", ackStruct);
                            // send the auth packet to the server
                            errorStatus = sendto(serverSocket2, ackStruct, strlen(ackStruct), 0,
                                                 (struct sockaddr *) &clientAddress,
                                                 sizeOfClientAddress);

                            // check if there was an error sending to the server, if so errorStatus < 0, then print an error message
                            // and exit with error code 1
                            if (errorStatus < 0) {
                                printf("\nError unable to send data acknowledgement data to the client!\n");
                                printf("Server aborting connection to the client...\n");
                                goto  WRQBreak;
                            }


                            // process segment 4
                        } else if (incomingData[dataBlockIndex + 1] == '4') {
                            // disable segment 2 flag
                            seg4Flag = 0;

                            // counter for how long the data segment is
                            int dataCounter = 0;

                            // perform different things depending on if we are in binary mode or not
                            if(replace){
                                seg4DataIndex = dataSZE;
                                dataCounter = dataSZE;
                                // copy the data only, from the first index (segment # index + 1)
                                memcpy(segment4, &incomingData[(dataSegIndex + 1)], dataCounter);
                            }else {

                                // loop through the data segment of the data packet
                                for (int i = (dataSegIndex + 1); i <= dataDataIndex; i++) {
                                    // if the i-th character in the session number isn't our line separator increment the sessionNumLen
                                    if (incomingData[i] != '\31') {
                                        dataCounter++;
                                    }
                                }
                                // set data index
                                seg4DataIndex = dataCounter;

                                // copy the data only, from the first index (segment # index + 1)
                                strncpy(segment4, &incomingData[(dataSegIndex + 1)], dataCounter);
                            }

                            // check to see if data counter is less than 1024, if so it is the last segment
                            if (dataCounter < 1024) {
                                // disable the out most loop control
                                control = 0;
                                // make the last segment variable equal to this segment
                                lastSeg = 4;
                                // make the last segment data  index equal to the data counter
                                lastSegDataIndex = dataCounter;
                            }

                            // send ack for this current segment
                            // print attempt number of loop
                            printf("Sending block #%d, segment #%d acknowledgement to the client\n", (atoi(blockNum)), 4);

                            // create variable to hold the ack structure
                            char ackStruct[authSize + 1];
                            // cleaning buffers of the variable to hold incoming data
                            memset(ackStruct, 0, sizeof(ackStruct));
                            // get the ack packet
                            generateACK("00000", blockNum, "4", ackStruct);
                            // send the auth packet to the server
                            errorStatus = sendto(serverSocket2, ackStruct, strlen(ackStruct), 0,
                                                 (struct sockaddr *) &clientAddress,
                                                 sizeOfClientAddress);

                            // check if there was an error sending to the server, if so errorStatus < 0, then print an error message
                            // and exit with error code 1
                            if (errorStatus < 0) {
                                printf("\nError unable to send data acknowledgement data to the client!\n");
                                printf("Server aborting connection to the client...\n");
                                goto  WRQBreak;
                            }


                            // process segment 5
                        } else if (incomingData[dataBlockIndex + 1] == '5') {
                            // disable segment 5 flag
                            seg5Flag = 0;

                            // counter for how long the data segment is
                            int dataCounter = 0;

                            // perform different things depending on if we are in binary mode or not
                            if(replace){
                                seg5DataIndex = dataSZE;
                                dataCounter = dataSZE;
                                // copy the data only, from the first index (segment # index + 1)
                                memcpy(segment5, &incomingData[(dataSegIndex + 1)], dataCounter);
                            }else {

                                // loop through the data segment of the data packet
                                for (int i = (dataSegIndex + 1); i <= dataDataIndex; i++) {
                                    // if the i-th character in the session number isn't our line separator increment the sessionNumLen
                                    if (incomingData[i] != '\31') {
                                        dataCounter++;
                                    }
                                }
                                // set data index
                                seg5DataIndex = dataCounter;

                                // copy the data only, from the first index (segment # index + 1)
                                strncpy(segment5, &incomingData[(dataSegIndex + 1)], dataCounter);
                            }

                            // check to see if data counter is less than 1024, if so it is the last segment
                            if (dataCounter < 1024) {
                                // disable the out most loop control
                                control = 0;
                                // make the last segment variable equal to this segment
                                lastSeg = 5;
                                // make the last segment data  index equal to the data counter
                                lastSegDataIndex = dataCounter;
                            }

                            // send ack for this current segment
                            // print attempt number of loop
                            printf("Sending block #%d, segment #%d acknowledgement to the client\n", (atoi(blockNum)), 5);

                            // create variable to hold the ack structure
                            char ackStruct[authSize + 1];
                            // cleaning buffers of the variable to hold incoming data
                            memset(ackStruct, 0, sizeof(ackStruct));
                            // get the ack packet
                            generateACK("00000", blockNum, "5", ackStruct);
                            // send the auth packet to the server
                            errorStatus = sendto(serverSocket2, ackStruct, strlen(ackStruct), 0,
                                                 (struct sockaddr *) &clientAddress,
                                                 sizeOfClientAddress);

                            // check if there was an error sending to the server, if so errorStatus < 0, then print an error message
                            // and exit with error code 1
                            if (errorStatus < 0) {
                                printf("\nError unable to send data acknowledgement data to the client!\n");
                                printf("Server aborting connection to the client...\n");
                                goto  WRQBreak;
                            }


                            // process segment 6
                        } else if (incomingData[dataBlockIndex + 1] == '6') {
                            // disable segment 2 flag
                            seg6Flag = 0;

                            // counter for how long the data segment is
                            int dataCounter = 0;

                            // perform different things depending on if we are in binary mode or not
                            if(replace){
                                seg6DataIndex = dataSZE;
                                dataCounter = dataSZE;
                                // copy the data only, from the first index (segment # index + 1)
                                memcpy(segment6, &incomingData[(dataSegIndex + 1)], dataCounter);

                            }else {
                                // loop through the data segment of the data packet
                                for (int i = (dataSegIndex + 1); i <= dataDataIndex; i++) {
                                    // if the i-th character in the session number isn't our line separator increment the sessionNumLen
                                    if (incomingData[i] != '\31') {
                                        dataCounter++;
                                    }
                                }
                                // set data index
                                seg6DataIndex = dataCounter;

                                // copy the data only, from the first index (segment # index + 1)
                                strncpy(segment6, &incomingData[(dataSegIndex + 1)], dataCounter);
                            }

                            // check to see if data counter is less than 1024, if so it is the last segment
                            if (dataCounter < 1024) {
                                // disable the out most loop control
                                control = 0;
                                // make the last segment variable equal to this segment
                                lastSeg = 6;
                                // make the last segment data  index equal to the data counter
                                lastSegDataIndex = dataCounter;
                            }

                            // send ack for this current segment
                            // print attempt number of loop
                            printf("Sending block #%d, segment #%d acknowledgement to the client\n", (atoi(blockNum)), 6);

                            // create variable to hold the ack structure
                            char ackStruct[authSize + 1];
                            // cleaning buffers of the variable to hold incoming data
                            memset(ackStruct, 0, sizeof(ackStruct));
                            // get the ack packet
                            generateACK("00000", blockNum, "6", ackStruct);
                            // send the auth packet to the server
                            errorStatus = sendto(serverSocket2, ackStruct, strlen(ackStruct), 0,
                                                 (struct sockaddr *) &clientAddress,
                                                 sizeOfClientAddress);

                            // check if there was an error sending to the server, if so errorStatus < 0, then print an error message
                            // and exit with error code 1
                            if (errorStatus < 0) {
                                printf("\nError unable to send data acknowledgement data to the client!\n");
                                printf("Server aborting connection to the client...\n");
                                goto  WRQBreak;
                            }


                            //process segment 7
                        } else if (incomingData[dataBlockIndex + 1] == '7') {
                            // disable segment 7 flag
                            seg7Flag = 0;

                            // counter for how long the data segment is
                            int dataCounter = 0;

                            // perform different things depending on if we are in binary mode or not
                            if(replace){
                                seg7DataIndex = dataSZE;
                                dataCounter = dataSZE;
                                // copy the data only, from the first index (segment # index + 1)
                                memcpy(segment7, &incomingData[(dataSegIndex + 1)], dataCounter);

                            }else {
                                // loop through the data segment of the data packet
                                for (int i = (dataSegIndex + 1); i <= dataDataIndex; i++) {
                                    // if the i-th character in the session number isn't our line separator increment the sessionNumLen
                                    if (incomingData[i] != '\31') {
                                        dataCounter++;
                                    }
                                }

                                // set data index
                                seg7DataIndex = dataCounter;
                                // copy the data only, from the first index (segment # index + 1)
                                strncpy(segment7, &incomingData[(dataSegIndex + 1)], dataCounter);
                            }

                            // check to see if data counter is less than 1024, if so it is the last segment
                            if (dataCounter < 1024) {
                                // disable the out most loop control
                                control = 0;
                                // make the last segment variable equal to this segment
                                lastSeg = 7;
                                // make the last segment data  index equal to the data counter
                                lastSegDataIndex = dataCounter;
                            }

                            // send ack for this current segment
                            // print attempt number of loop
                            printf("Sending block #%d, segment #%d acknowledgement to the client\n", (atoi(blockNum)), 7);

                            // create variable to hold the ack structure
                            char ackStruct[authSize + 1];
                            // cleaning buffers of the variable to hold incoming data
                            memset(ackStruct, 0, sizeof(ackStruct));
                            // get the ack packet
                            generateACK("00000", blockNum, "7", ackStruct);
                            // send the auth packet to the server
                            errorStatus = sendto(serverSocket2, ackStruct, strlen(ackStruct), 0,
                                                 (struct sockaddr *) &clientAddress,
                                                 sizeOfClientAddress);

                            // check if there was an error sending to the server, if so errorStatus < 0, then print an error message
                            // and exit with error code 1
                            if (errorStatus < 0) {
                                printf("\nError unable to send data acknowledgement data to the client!\n");
                                printf("Server aborting connection to the client...\n");
                                goto  WRQBreak;
                            }


                            // process segment 8
                        } else if (incomingData[dataBlockIndex + 1] == '8') {
                            // disable segment 8 flag
                            seg8Flag = 0;

                            // counter for how long the data segment is
                            int dataCounter = 0;

                            // perform different things depending on if we are in binary mode or not
                            if(replace){
                                seg8DataIndex = dataSZE;
                                dataCounter = dataSZE;
                                // copy the data only, from the first index (segment # index + 1)
                                memcpy(segment8, &incomingData[(dataSegIndex + 1)], dataCounter);

                            }else {
                                // loop through the data segment of the data packet
                                for (int i = (dataSegIndex + 1); i <= dataDataIndex; i++) {
                                    // if the i-th character in the session number isn't our line separator increment the sessionNumLen
                                    if (incomingData[i] != '\31') {
                                        dataCounter++;
                                    }
                                }

                                // set data index
                                seg8DataIndex = dataCounter;
                                // copy the data only, from the first index (segment # index + 1)
                                strncpy(segment8, &incomingData[(dataSegIndex + 1)], dataCounter);
                            }


                            // check to see if data counter is less than 1024, if so it is the last segment
                            if (dataCounter < 1024) {
                                // disable the out most loop control
                                control = 0;
                                // make the last segment variable equal to this segment
                                lastSeg = 8;
                                // make the last segment data  index equal to the data counter
                                lastSegDataIndex = dataCounter;
                            }

                            // send ack for this current segment
                            // print attempt number of loop
                            printf("Sending block #%d, segment #%d acknowledgement to the client\n", (atoi(blockNum)), 8);

                            // create variable to hold the ack structure
                            char ackStruct[authSize + 1];
                            // cleaning buffers of the variable to hold incoming data
                            memset(ackStruct, 0, sizeof(ackStruct));
                            // get the ack packet
                            generateACK("00000", blockNum, "8", ackStruct);
                            // send the auth packet to the server
                            errorStatus = sendto(serverSocket2, ackStruct, strlen(ackStruct), 0,
                                                 (struct sockaddr *) &clientAddress,
                                                 sizeOfClientAddress);

                            // check if there was an error sending to the server, if so errorStatus < 0, then print an error message
                            // and exit with error code 1
                            if (errorStatus < 0) {
                                printf("\nError unable to send data acknowledgement data to the client!\n");
                                printf("Server aborting connection to the client...\n");
                                goto  WRQBreak;
                            }


                            // if opcode is 6 we have an error message from the server, so display error message and end the program
                        } else if (incomingData[dataOpIndex] == '6') {
                            // counter for error message length
                            int errorMsgLen = 0;

                            // loop through the error EFPT packet to see how big the message is, start at the first index of the error
                            // message
                            for (int i = (errOpIndex + 1); i <= errStrIndex; i++) {
                                // if the i-th character in the error message isn't our line separator increment the errorMsgLen
                                if (incomingData[i] != '\31') {
                                    errorMsgLen++;
                                }
                            }
                            // create a variable for the error message (+1 so we can append a '\0' later on)
                            char errMSG[errorMsgLen + 1];
                            // cleaning buffers of the variable to hold incoming data
                            memset(errMSG, 0, sizeof(errMSG));

                            // copy the error message only, from the first index (op code index + 1)
                            strncpy(errMSG, &incomingData[(errOpIndex + 1)], errorMsgLen);


                            // print the error message, close socket, and exit the program with error code 1
                            printf("\n%s\n", errMSG);
                            printf("Server aborting connection to the Client...\n");
                            close(serverSocket2);
                            goto WRQBreak;

                        }
                    }
                    // setting socket to time out if nothing is there is inactivity for 5 seconds, this way the ack delays will be normal again
                    setsockopt(serverSocket2, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout20sec, sizeof(struct timeval));

                }

                // writing block to file

                // segment 1 data
                if(seg1Flag == 0) {
                    // create a trimmed down version of segment1 without all of the dead space
                    char segment1c[seg1DataIndex];
                    // cleaning buffers of the variable to hold incoming data
                    memset(segment1c, 0, sizeof(segment1c));
                    // use memcpy if we are in binary mode
                    if(replace){
                        memcpy(segment1c, segment1, seg1DataIndex);
                    }else {
                        strncpy(segment1c, segment1, seg1DataIndex);
                    }

                    if((fwrite(segment1c, sizeof(segment1c), 1, outFile) == 0) && (sizeof(segment1c) != 0)){
                        printf("\nError: server was not able to write to the output file \"%s\"\n",
                               fileName);
                        printf("Server aborting connection to the client...\n");

                        // create error message
                        char errMSG[] = {
                                "Error: the server was unable write any data to its file and had to abort the connection\n"};
                        char errPCK[errSize + 1];
                        // cleaning buffers of the variable to hold incoming data
                        memset(errPCK, 0, sizeof(errPCK));
                        generateError(errMSG, errPCK);

                        // send error message to the server
                        sendto(serverSocket2, errPCK, strlen(errPCK), 0, (struct sockaddr *) &clientAddress,
                               sizeOfClientAddress);

                        goto WRQBreak;
                    }
                }



                // segment 2 data

                if(seg2Flag == 0) {
                    // create a trimmed down version of segment2 without all of the dead space
                    char segment2c[seg2DataIndex];
                    // cleaning buffers of the variable to hold incoming data
                    memset(segment2c, 0, sizeof(segment2c));

                    // if in binary mode we will use memcpy
                    if(replace){
                        memcpy(segment2c, segment2, seg2DataIndex);
                    }else {
                        strncpy(segment2c, segment2, seg2DataIndex);
                    }

                    if((fwrite(segment2c, sizeof(segment2c), 1, outFile) == 0) && (sizeof(segment2c) != 0)){
                        printf("\nError: server was not able to write to the output file \"%s\"\n",
                               fileName);
                        printf("Server aborting connection to the client...\n");

                        // create error message
                        char errMSG[] = {
                                "Error: the server was unable write any data to its file and had to abort the connection\n"};
                        char errPCK[errSize + 1];
                        // cleaning buffers of the variable to hold incoming data
                        memset(errPCK, 0, sizeof(errPCK));
                        generateError(errMSG, errPCK);

                        // send error message to the server
                        sendto(serverSocket2, errPCK, strlen(errPCK), 0, (struct sockaddr *) &clientAddress,
                               sizeOfClientAddress);

                        goto WRQBreak;
                    }
                }


                // segment 3 data
                if(seg3Flag == 0) {
                    // create a trimmed down version of segment3 without all of the dead space
                    char segment3c[seg3DataIndex];
                    // cleaning buffers of the variable to hold incoming data
                    memset(segment3c, 0, sizeof(segment3c));

                    // in in binary mode use memcpy
                    if(replace){
                        memcpy(segment3c, segment3, seg3DataIndex);
                    }else {
                        strncpy(segment3c, segment3, seg3DataIndex);
                    }

                    if((fwrite(segment3c, sizeof(segment3c), 1, outFile) == 0) && (sizeof(segment3c) != 0)){
                        printf("\nError: server was not able to write to the output file \"%s\"\n",
                               fileName);
                        printf("Server aborting connection to the client...\n");

                        // create error message
                        char errMSG[] = {
                                "Error: the server was unable write any data to its file and had to abort the connection\n"};
                        char errPCK[errSize + 1];
                        // cleaning buffers of the variable to hold incoming data
                        memset(errPCK, 0, sizeof(errPCK));
                        generateError(errMSG, errPCK);

                        // send error message to the server
                        sendto(serverSocket2, errPCK, strlen(errPCK), 0, (struct sockaddr *) &clientAddress,
                               sizeOfClientAddress);

                        goto WRQBreak;
                    }
                }


                // segment 4 data

                if(seg4Flag == 0) {
                    // create a trimmed down version of segment4 without all of the dead space
                    char segment4c[seg4DataIndex];
                    // cleaning buffers of the variable to hold incoming data
                    memset(segment4c, 0, sizeof(segment4c));

                    // if in binary mode use memcpy
                    if(replace){
                        memcpy(segment4c, segment4, seg4DataIndex);
                    }else {
                        strncpy(segment4c, segment4, seg4DataIndex);
                    }

                    if((fwrite(segment4c, sizeof(segment4c), 1, outFile) == 0) && (sizeof(segment4c) != 0)){
                        printf("\nError: server was not able to write to the output file \"%s\"\n",
                               fileName);
                        printf("Server aborting connection to the client...\n");

                        // create error message
                        char errMSG[] = {
                                "Error: the server was unable write any data to its file and had to abort the connection\n"};
                        char errPCK[errSize + 1];
                        // cleaning buffers of the variable to hold incoming data
                        memset(errPCK, 0, sizeof(errPCK));
                        generateError(errMSG, errPCK);

                        // send error message to the server
                        sendto(serverSocket2, errPCK, strlen(errPCK), 0, (struct sockaddr *) &clientAddress,
                               sizeOfClientAddress);

                        goto WRQBreak;
                    }
                }


                // segment 5 data
                if(seg5Flag == 0) {
                    // create a trimmed down version of segment5 without all of the dead space
                    char segment5c[seg5DataIndex];
                    // cleaning buffers of the variable to hold incoming data
                    memset(segment5c, 0, sizeof(segment5c));

                    // if in binary mode use memcpy
                    if(replace){
                        memcpy(segment5c, segment5, seg5DataIndex);
                    }else {
                        strncpy(segment5c, segment5, seg5DataIndex);
                    }

                    if((fwrite(segment5c, sizeof(segment5c), 1, outFile) == 0) && (sizeof(segment5c) != 0)){
                        printf("\nError: server was not able to write to the output file \"%s\"\n",
                               fileName);
                        printf("Server aborting connection to the client...\n");

                        // create error message
                        char errMSG[] = {
                                "Error: the server was unable write any data to its file and had to abort the connection\n"};
                        char errPCK[errSize + 1];
                        // cleaning buffers of the variable to hold incoming data
                        memset(errPCK, 0, sizeof(errPCK));
                        generateError(errMSG, errPCK);

                        // send error message to the server
                        sendto(serverSocket2, errPCK, strlen(errPCK), 0, (struct sockaddr *) &clientAddress,
                               sizeOfClientAddress);

                        goto WRQBreak;
                    }
                }


                // segment 6 data
                if(seg6Flag == 0) {
                    // create a trimmed down version of segment6 without all of the dead space
                    char segment6c[seg6DataIndex];
                    // cleaning buffers of the variable to hold incoming data
                    memset(segment6c, 0, sizeof(segment6c));

                    // if in binary mode use memcpy
                    if(replace){
                        memcpy(segment6c,segment6,seg6DataIndex);
                    } else{
                        strncpy(segment6c,segment6,seg6DataIndex);
                    }

                    if((fwrite(segment6c, sizeof(segment6c), 1, outFile) == 0) && (sizeof(segment6c) != 0)){
                        printf("\nError: server was not able to write to the output file \"%s\"\n",
                               fileName);
                        printf("Server aborting connection to the client...\n");

                        // create error message
                        char errMSG[] = {
                                "Error: the server was unable write any data to its file and had to abort the connection\n"};
                        char errPCK[errSize + 1];
                        // cleaning buffers of the variable to hold incoming data
                        memset(errPCK, 0, sizeof(errPCK));
                        generateError(errMSG, errPCK);

                        // send error message to the server
                        sendto(serverSocket2, errPCK, strlen(errPCK), 0, (struct sockaddr *) &clientAddress,
                               sizeOfClientAddress);

                        goto WRQBreak;
                    }
                }


                // segment 7 data
                if(seg7Flag == 0) {
                    // create a trimmed down version of segment4 without all of the dead space
                    char segment7c[seg7DataIndex];
                    // cleaning buffers of the variable to hold incoming data
                    memset(segment7c, 0, sizeof(segment7c));

                    // if in binary mode use memcpy
                    if(replace){
                        memcpy(segment7c,segment7,seg7DataIndex);
                    }else{
                        strncpy(segment7c,segment7,seg7DataIndex);
                    }

                    if((fwrite(segment7c, sizeof(segment7c), 1, outFile) == 0) && (sizeof(segment7c) != 0)){
                        printf("\nError: server was not able to write to the output file \"%s\"\n",
                               fileName);
                        printf("Server aborting connection to the client...\n");

                        // create error message
                        char errMSG[] = {
                                "Error: the server was unable write any data to its file and had to abort the connection\n"};
                        char errPCK[errSize + 1];
                        // cleaning buffers of the variable to hold incoming data
                        memset(errPCK, 0, sizeof(errPCK));
                        generateError(errMSG, errPCK);

                        // send error message to the server
                        sendto(serverSocket2, errPCK, strlen(errPCK), 0, (struct sockaddr *) &clientAddress,
                               sizeOfClientAddress);

                        goto WRQBreak;
                    }
                }


                // segment 8 data
                if(seg8Flag == 0) {
                    // create a trimmed down version of segment8 without all of the dead space
                    char segment8c[seg8DataIndex];
                    // cleaning buffers of the variable to hold incoming data
                    memset(segment8c, 0, sizeof(segment8c));

                    // if in binary mode use memcpy
                    if(replace){
                        memcpy(segment8c,segment8, seg8DataIndex);
                    }else{
                        strncpy(segment8c,segment8, seg8DataIndex);
                    }

                    if((fwrite(segment8c, sizeof(segment8c), 1, outFile) == 0) && (sizeof(segment8c) != 0)){
                        printf("\nError: server was not able to write to the output file \"%s\"\n",
                               fileName);
                        printf("Server aborting connection to the client...\n");

                        // create error message
                        char errMSG[] = {
                                "Error: the server was unable write any data to its file and had to abort the connection\n"};
                        char errPCK[errSize + 1];
                        // cleaning buffers of the variable to hold incoming data
                        memset(errPCK, 0, sizeof(errPCK));
                        generateError(errMSG, errPCK);

                        // send error message to the server
                        sendto(serverSocket2, errPCK, strlen(errPCK), 0, (struct sockaddr *) &clientAddress,
                               sizeOfClientAddress);

                        goto WRQBreak;
                    }
                }



            }
            // print that the file was downloaded
            printf("Finished Downloading file named: %s\n", fileName);

            // label for goto calls in WRQ process
            WRQBreak:

            // close file
            fclose(outFile);
        }



















        // label for goto command incase of an error
        breakfree:

        // closing the second socket
        close(serverSocket2);

        // get new session number
        SessionNum = randSessionNum();

        // print message saying data transfer completed
        printf("Data transfer process completed! Disconnecting from current client...\n\n\n");


    }
// technically this point will never be reached because the server will continue looping forever, this is just here to
// satisfy the syntax
    return 0;
}
