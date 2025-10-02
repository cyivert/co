#ifndef SHARED_H
#define SHARED_H

// FIFO definitions ----> Not sure which one to use for final copy
#define FIFO_PATH           "./travel_agency_fifo"
#define PERM_OWNER_RW       0600   // (Owner: rw, Group: --, Other: --)
#define PERM_OWNER_RW_ALL_R 0644   // (Owner: rw, Group: r-, Other: r-)
#define PERM_ALL_RW         0666   // (Owner: rw, Group: rw, Other: rw)
#define PERM_ALL_RWX        0777   // (Owner: rwx, Group: rwx, Other: rwx)

// General purpose defines
#define MAX_BUFFER_SIZE 256

// Regex patterns for input validation (*: 0 or more, +: 1 or more)
#define REGEX_NAME   "^[A-Z][a-z]* [A-Z][a-z]*$"   // Format: Firstname Lastname
#define REGEX_NUMBER "^[0-9]+$"   // Format: Numbers only, At least one digit

// Client field defines
#define MAX_NAME_LEN        50
#define MAX_ADDRESS_LEN     200
#define MAX_DESTINATION_LEN 200
#define MIN_CLIENT_AGE      18
#define MAX_CLIENT_AGE      125
#define MAX_CLIENTS         100

// Define of Client struct
typedef struct Client {
    char firstName[MAX_NAME_LEN];
    char lastName[MAX_NAME_LEN];
    int  age;
    char address[MAX_ADDRESS_LEN];
} Client;

// Define of Party struct
typedef struct Trip {
    char   destination[MAX_DESTINATION_LEN];
    int    numberOfClients;
    Client clients[MAX_CLIENTS];   // Change to dynamic array?
} Trip;

// Stream-based input gathering functions
int clearStream(FILE *stream);
int getInputFromStream(FILE *stream, char *destination, size_t bufSize, bool keepNewline);

// Validation Utility Functions
void printInputError(const char *fieldName, int errorCode, size_t bufSize);
bool isNullTerminated(const char *buffer, size_t bufSize);
bool stringMatchesRegex(const char *string, size_t bufSize, const char *pattern);

#endif   // SHARED_H
