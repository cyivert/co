#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include "shared.h"

void processMessages(const char *fifoname);
void writeToLog(FILE *logFile, const char *message);
void timeout_handler(int sig);
void reset_timeout(void);

int main(void) {
    printf("Travel Agency Server - Waiting for client data...\n");
    
    // Set up timeout handler for inactivity
    signal(SIGALRM, timeout_handler);
    alarm(120); // 2 minutes = 120 seconds
    
    // Create FIFO if it doesn't exist
    if (mkfifo(FIFO_PATH, PERM_OWNER_RW_ALL_R) == -1) {
        // FIFO might already exist, which is okay
        if (errno != EEXIST) {
            perror("Error creating FIFO");
            return -1;
        }
    }
    
    // Process messages from clients
    processMessages(FIFO_PATH);
    
    return 0;
}

//
// FUNCTION : processMessages
// DESCRIPTION : Processes messages from the FIFO, handling party and client data,
//               and logging activities to a log file.
// PARAMETERS : 
// const char *fifoname : Path to the FIFO to read messages from.
// RETURNS : n/a
//
void processMessages(const char *fifoname) {
    FILE *logFile = fopen("travel_agency.log", "a");
    if (!logFile) {
        perror("Error opening log file");
        return;
    }
    
    int fd;
    char buffer[MAX_BUFFER_SIZE];
    char destination[MAX_DESTINATION_LEN] = {0};
    int clientCount = 0;
    bool inParty = false;
    bool serverRunning = true;
    
    writeToLog(logFile, "Server started");
    
    while (serverRunning) {
        // Open FIFO for reading
        fd = open(fifoname, O_RDONLY);
        if (fd == -1) {
            perror("Error opening FIFO for reading");
            break;
        }
        
        // Read message from FIFO
        ssize_t bytesRead = read(fd, buffer, sizeof(buffer) - 1);
        if (bytesRead > 0) {
            // Reset timeout on activity
            reset_timeout();
            
            buffer[bytesRead] = '\0';
            
            // Remove trailing newline if present
            size_t len = strlen(buffer);
            if (len > 0 && buffer[len-1] == '\n') {
                buffer[len-1] = '\0';
            }
            
            printf("Received: %s\n", buffer);
            writeToLog(logFile, buffer);
            
            // Process the message based on content
            if (strcmp(buffer, "party") == 0) {
                inParty = true;
                clientCount = 0;
                memset(destination, 0, sizeof(destination));
                printf("New party started\n");
            }
            else if (strcmp(buffer, "stop") == 0) {
                printf("Stop command received. Shutting down server.\n");
                writeToLog(logFile, "Server received stop command");
                serverRunning = false;
            }
            else if (inParty && strlen(destination) == 0) {
                // First message after "party" should be destination
                strncpy(destination, buffer, sizeof(destination) - 1);
                printf("Party destination: %s\n", destination);
            }
            else if (strcmp(buffer, "client") == 0) {
                printf("New client being added...\n");
            }
            else if (strcmp(buffer, "END_PARTY") == 0 || strcmp(buffer, "end") == 0) {
                if (inParty) {
                    printf("=== PARTY SUMMARY ===\n");
                    printf("Destination: %s\n", destination);
                    printf("Number of clients: %d\n", clientCount);
                    printf("====================\n\n");
                    
                    char summary[512];
                    snprintf(summary, sizeof(summary), "Party completed - Destination: %s, Clients: %d", 
                            destination, clientCount);
                    writeToLog(logFile, summary);
                }
                inParty = false;
            }
            else if (inParty && strchr(buffer, ',') != NULL) {
                // This looks like client data (contains commas)
                clientCount++;
                printf("Client %d: %s\n", clientCount, buffer);
            }
        }
        
        close(fd);
    }
    
    writeToLog(logFile, "Server stopped");
    fclose(logFile);
}

//
// FUNCTION : writeToLog
// DESCRIPTION : Writes a message to the log file with a timestamp.
// PARAMETERS :
// FILE *logFile : Pointer to the opened log file.
// const char *message : Message to log.
// RETURNS : n/a
//
void writeToLog(FILE *logFile, const char *message) {
    if (logFile) {
        time_t now;
        time(&now);
        char *timeStr = ctime(&now);
        // Remove newline from time string
        if (timeStr) {
            timeStr[strlen(timeStr) - 1] = '\0';
        }
        fprintf(logFile, "[%s] %s\n", timeStr ? timeStr : "Unknown time", message);
        fflush(logFile);
    }
}

//
// FUNCTION : timeout_handler
// DESCRIPTION : Signal handler for SIGALRM. Terminates the server after timeout.
// PARAMETERS : int sig - Signal number (SIGALRM)
// RETURNS : n/a (exits program)
//
void timeout_handler(int sig) {
    (void)sig; // Suppress unused parameter warning
    printf("\nServer timeout: No activity for 2 minutes. Terminating server...\n");
    
    // Log the timeout event
    FILE *logFile = fopen("travel_agency.log", "a");
    if (logFile) {
        writeToLog(logFile, "Server terminated due to inactivity timeout (2 minutes)");
        fclose(logFile);
    }
    
    exit(0);
}

//
// FUNCTION : reset_timeout
// DESCRIPTION : Resets the alarm timer to 2 minutes from current time.
// PARAMETERS : n/a
// RETURNS : n/a
//
void reset_timeout(void) {
    alarm(0);   // Cancel current alarm
    alarm(120); // Reset to 2 minutes (120 seconds)
}
