/*
 * FILE: client.c
 * PROGRAMMER: Tyler Gee, Cy Iver Torrefranca, Tuan Thanh Nguyen, George S.
 * PROJECT: SENG2031 - Assignment 1
 * DESCRIPTION:
 * The client program collects trip and client data from the user,
 * then it validates the input, and sends it to the server via a FIFO.
 */

// Include necessary header files for client.c functions and variables
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <regex.h>

#include "shared.h"

// Conversion functions
bool convertToInt(const char *buffer, int *result);

// Validation Utility Functions
void printInputError(const char *fieldName, int errorCode, size_t bufSize);
bool isNullTerminated(const char *buffer, size_t bufSize);
bool stringMatchesRegex(const char *string, size_t bufSize, const char *pattern);

// Trip and Client Input Functions
bool getInputFromClient(
    const char *label, char *buffer, size_t bufSize, char *destination
);
bool getTripDestination(char *destination);
void splitClientName(const char *buffer, size_t bufSize, char *firstName, char *lastName);
bool getClientName(char *firstName, char *lastName);
bool getClientAge(int *age);
bool getClientAddress(char *address);
char *clientToString(const Client *client);

// FIFO Stream Functions
int writestringToFIFO(const char *fifoname, const char *string, bool showConnectionMsg);

// Timeout functions
void timeout_handler(int sig);
void reset_timeout(void);

int main(void) {
    char buffer[MAX_BUFFER_SIZE] = {0};   // Buffer for user input
    int  numberOfClients         = 0;     // Number of clients in current party
    int  err                     = 0;     // Error code for input validation
    Trip tripIfo                 = {0};

    // Variables for client input validation
    bool quitProgram         = false;
    bool partyStarted        = false;
    bool awaitingClientInput = false;
    bool endOfClientList     = false;
    bool isValidDestination  = false;
    bool isValidName         = false;
    bool isValidAge          = false;
    bool isValidAddress      = false;

    printf("Travel Agency Client\n");
    printf("Note: Please ensure the server is running before proceeding.\n");
    
    // Set up timeout handler for inactivity
    signal(SIGALRM, timeout_handler);
    alarm(TIMEOUT_DURATION); // 2 minutes = 120 seconds

    do {
        // ---------- PARTY / STOP LOOP ----------
        printf("\nEnter 'party' to start a new party or 'stop' to exit: ");
        if ((err = getInputFromStream(stdin, buffer, MAX_BUFFER_SIZE, false)) != 0) {
            printInputError("Start/stop input", err, MAX_BUFFER_SIZE);
            continue;
        }
        
        // Reset timeout on user activity
        reset_timeout();

        partyStarted = stringMatchesRegex(buffer, MAX_BUFFER_SIZE, "^party$");
        quitProgram  = stringMatchesRegex(buffer, MAX_BUFFER_SIZE, "^stop$");

        if (!partyStarted && !quitProgram) {
            printf("Input must be 'party' or 'stop'\n");
            continue;
        } else if (quitProgram) {
            // Send stop command to server
            if (writestringToFIFO(FIFO_PATH, "stop", false) == -1) {
                printf("Error: Failed to write stop command to FIFO\n");
            }
            break;
        }

        // i enable party input loop
        // - cy
        // ---------- Start FIFO Stream ----------
        if ((mkfifo(FIFO_PATH, PERM_ALL_RW)) == ERROR) {
            // FIFO might already exist, which is okay
            if (errno != EEXIST) {
                printf("Could not create FIFO pipe.\n");
                return ERROR;
            }
        }
        printf("FIFO pipe ready.\n");

        // Write 'party' to FIFO
        if (writestringToFIFO(FIFO_PATH, buffer, true) == ERROR) {
            printf("Error: Failed to write to FIFO\n");
            return ERROR;
        }

        // ---------- TRIP DATA INPUT BEGINS: DESTINATION ----------
        do {
            isValidDestination = getTripDestination(tripIfo.destination);
        } while (!isValidDestination);

        // Check if user wants to stop during destination input
        if (stringMatchesRegex(tripIfo.destination, MAX_DESTINATION_LEN, "^stop$")) {
            printf("Stopping the program...\n");
            if (writestringToFIFO(FIFO_PATH, "stop", false) == SUCCESS) {
                printf("Sent stop command to server.\n");
            }
            break;  // Exit the main loop
        }

        // Write destination to FIFO //
        if (writestringToFIFO(FIFO_PATH, tripIfo.destination, false) == ERROR) {
            printf("Error: Failed to write to FIFO\n");
            return ERROR;
        }

        numberOfClients = 0;

        // ---------- CLIENT LOOP ----------
        while (!endOfClientList) {
            printf(
                "\nEnter 'client' to add a client or 'end' to finish the "
                "party: "
            );
            if ((err = getInputFromStream(stdin, buffer, MAX_BUFFER_SIZE, false)) != SUCCESS) {
                printInputError("Client/end input", err, MAX_BUFFER_SIZE);
                continue;
            }
            
            // Reset timeout on user activity
            reset_timeout();

            endOfClientList     = stringMatchesRegex(buffer, MAX_BUFFER_SIZE, "^end$");
            awaitingClientInput = stringMatchesRegex(buffer, MAX_BUFFER_SIZE, "^client$");

            if (!awaitingClientInput && !endOfClientList) {
                printf("Input must be 'client' or 'end'\n");
                continue;
            } else if (endOfClientList) {
                printf("Finished gathering clients for this party.\n");
                
                // Write "end" signal to indicate party completion
                // this is just to signal the server that the party is over - cy
                if (writestringToFIFO(FIFO_PATH, "END_PARTY", false) == ERROR) {
                    printf("Error: Failed to write end signal to FIFO\n");
                    return ERROR;
                }
                
                break;   // back to party/stop
            }

            // ---------- CLIENT DATA INPUT ----------
            do {
                isValidName = getClientName(
                    tripIfo.clients[numberOfClients].firstName,
                    tripIfo.clients[numberOfClients].lastName
                );
            } while (!isValidName);

            do {
                isValidAge = getClientAge(&(tripIfo.clients[numberOfClients].age));
            } while (!isValidAge);

            do {
                isValidAddress
                    = getClientAddress(tripIfo.clients[numberOfClients].address);
            } while (!isValidAddress);

            // Display client information in formatted style
            // Tuan Thanh Nguyen
            printf("\n-----------------------------\n");
            printf("Client %d\n", numberOfClients + 1);
            printf("Name    : %s %s\n", 
                   tripIfo.clients[numberOfClients].firstName,
                   tripIfo.clients[numberOfClients].lastName);
            printf("Age     : %d\n", tripIfo.clients[numberOfClients].age);
            printf("Address : %s\n", tripIfo.clients[numberOfClients].address);
            printf("-----------------------------\n\n");

            char *clientString = clientToString(&tripIfo.clients[numberOfClients]);

            // check if client string allocation failed
            if (!clientString) {
                printf("Error: Failed to allocate memory for client string\n");
                return ERROR;
            } else {
                // Write client string to FIFO
                if (writestringToFIFO(FIFO_PATH, clientString, false) == ERROR) {
                    printf("Error: Failed to write to FIFO\n");
                    return ERROR;
                }

                // free client string
                if (clientString) {
                    free(clientString);
                    clientString = NULL;
                }

                numberOfClients++;
            }   // end of client information input loop
        }

        // reset all loop control variables for next party
        partyStarted        = false;
        awaitingClientInput = false;
        endOfClientList     = false;
        numberOfClients     = 0;
        memset(&tripIfo, 0, sizeof(Trip));
    } while (!quitProgram);

    printf("Exiting the program...\n");
    return SUCCESS;
}

/*

 * FUNCTION: timeout_handler
 * PROGRAMMER: Cy Iver Torrefranca
 * DESCRIPTION:
    *  Signal handler for SIGALRM. Terminates the client after timeout.
 * PARAMETERS:
    *  int sig - Signal number (SIGALRM)
 * RETURN: 
    * n/a (exits program)

 */
void timeout_handler(int sig) {
    (void)sig; // Suppress unused parameter warning
    printf("\n\nClient timeout: No activity for 2 minutes. Terminating client...\n");
    exit(TIMEOUT_SUCCESSFUL);
}

/*

 * FUNCTION: reset_timeout
 * PROGRAMMER: Cy Iver Torrefranca
 * DESCRIPTION: Resets the alarm timer to 2 minutes from current time.
 * PARAMETERS: n/a
 * RETURN: n/a
 */
void reset_timeout(void) {
    alarm(CANCEL_TIMEOUT);   // Cancel current alarm
    alarm(TIMEOUT_DURATION); // Reset to 2 minutes (120 seconds)
}

// #####################################################################################################################
// Shared header file Function Definitions (shared.h)
// #####################################################################################################################

/* FUNCTION: clearStream
 * PROGRAMMER: Tyler Gee
 * DESCRIPTION:
    *  Clears the input stream until a newline character or EOF is found and
    *  returns the number of characters cleared. Newline characters and EOF are
    *  **NOT** included in the returned count.
    *
 * PARAMETERS:
    *  FILE *stream: Pointer to the Stream to clear
    *
 * RETURN:
    *  int: The number of characters cleared from the stream
    *       (Excluding newline and EOF).
 */
int clearStream(FILE *stream) {
    int currentChar = 0;
    int count       = 0;
    while ((currentChar = fgetc(stream)) != '\n' && currentChar != EOF) {
        count++;
    }
    return count;
}

/* FUNCTION: getInputFromStream
 * PROGRAMMER: Tyler Gee
 * DESCRIPTION:
    *  Reads a line of text from a stream into the destination buffer.
    *  At most (bufSize - 1) characters are stored before truncation -
    *  automatic null terminator is added.
    *
    *  The newline character at the end of the input line can be kept depending on
    *  the keepNewline flag:
    *      - true:     newline is preserved IF present
    *      - false:    newline is removed IF present
    *
    *  If the input line exceeds (bufSize - 1) bytes, all extra characters
    *  (excluding newline and EOF characters) are discarded. An error code of
    *  ENOBUFS will be returned
    *
    *  **NOTE:** newline and EOF characters left in the stream DO NOT COUNT! No
    *  error code will be thrown if only those characters are left in the stream.
    *
 * PARAMETERS:
    *  FILE *stream:       Stream to read from
    *  char *destination:  Pointer to the buffer to store intput string in.
    *  size_t bufSize:     Max Size of buffer **including null terminator **
    *  bool keepNewline:   Flag to keep the newline character at the end
 *
 * RETURN:
    *  EOF(-1):        End-of-file encountered before input. Usually means EOF
    *  0:              Success
    *  EIO(5):         Generic I/O error if no specific errno is set.
    *  EINVAL(22):     Invalid arguments or empty input.
    *  ENOBUFS(105):   Input exceeded buffer size and was discarded.
 */
int getInputFromStream(
    FILE *stream, char *destination, size_t bufSize, bool keepNewline
) {
    // validate input parameters
    if (!stream || !destination || bufSize < BUFFER_SIZE_OF_TWO) {
        return EINVAL;   // Must be at least 1 char + null terminator
    }

    // Attempt to read input from the stream
    if (fgets(destination, (int)bufSize, stream) == NULL) {
        if (feof(stream)) {
            return EOF;   // end-of-file reached
        }

        return errno ? errno : EIO;
    }

    size_t len = strlen(destination);

    // Reject empty input or input that is just a newline
    if (len == BUFFER_SIZE_OF_ZERO || (len == BUFFER_SIZE_OF_ONE && destination[0] == '\n')) {
        return EINVAL;   // empty input
    }

    bool bufferFull        = (len == bufSize - BUFFER_SIZE_OF_ONE);
    bool lastCharIsNewline = (destination[len - BUFFER_SIZE_OF_ONE] == '\n');

    // Remove the newline if keepNewline flag is false
    if (!keepNewline && lastCharIsNewline) {
        destination[len - BUFFER_SIZE_OF_ONE] = '\0';
        len--;   // adjust length for consistency
    }

    // If buffer was full and the last character was not a newline
    if (bufferFull && !lastCharIsNewline) {
        /* clearStream discards any extra characters until a newline or EOF is
        found. It returns the number of excess characters **NOT** including the
        newline */
        if (clearStream(stream) > 0) {
            return ENOBUFS;   // input was too long
        }
    }
    return 0;   // success
}

/* FUNCTION: printInputError
 * PROGRAMMER: Tyler Gee
 * DESCRIPTION:
    *  Print an error message based on the common error code received from stream
    *  reading. Error codes are defined in <errno.h>. Response messages are
    *  specific to the fieldName passed using the fieldName parameter.
 *
 * PARAMETERS:
    *  const char *fieldName: Label for field that the input failed validation from
    *  int errorCode:         Error Code received from stream reading
    *  size_t bufSize:        Used to print Max buffer size in error message
 *
 * RETURN: None.
 */
void printInputError(const char *fieldName, int errorCode, size_t bufSize) {
    if (errorCode == EINVAL) {   // Invalid arguments or empty input
        printf(
            "Invalid %s input - Input cannot be empty or only whitespace.\n", fieldName
        );
    } else if (errorCode == ENOBUFS) {   // Input exceeded buffer size
        printf(
            "Invalid %s input - Exceeded maximum input length of %zu "
            "characters.\n",
            fieldName, bufSize - BUFFER_SIZE_OF_ONE
        );
    } else {   // Unknown error code
        printf("An unknown error occurred while reading input.\n");
    }
}

/* FUNCTION: isNullTerminated
 * PROGRAMMER: Tyler Gee
 * DESCRIPTION:
 *  Checks whether a buffer contains a null terminator ('\0') within the
 *  first bufSize bytes.
 *  **NOTE:** This function does not check for a null terminator in the
 *  rest of the buffer that exceeds bufSize bytes.
 *
 * PARAMETERS:
 *   const char *buffer:    Pointer to the buffer to check.
 *   size_t bufSize:        Max number of bytes to examine in the buffer.
 * RETURN:
 *   true: A null terminator was found within the first bufSize bytes.
 *   false: No null terminator was found.
 */
bool isNullTerminated(const char *buffer, size_t bufSize) {
    if (!buffer || bufSize == BUFFER_SIZE_OF_ZERO) {   // Invalid input
        return false;
    }

    /* Iterate through the buffer and check for a null terminator
    size_t is used to ensure the loop doesn't go beyond the buffer's
    boundaries. (e.g., bufSize > INT_MAX)*/
    for (size_t i = 0; i < bufSize; i++) {
        if (buffer[i] == '\0') {
            return true;
        }
    }
    return false;
}

/*
 * FUNCTION: stringMatchesRegex
 * PROGRAMMER: Tyler Gee
 * DESCRIPTION:
    *  Using a regex pattern, check if the given string matches the pattern. String
    *  is null-terminated and does not exceed bufSize bytes.
 * PARAMETERS:
    *  const char *string:     The string to match against the pattern.
    *  size_t bufSize:         Max size of the buffer in bytes, including the null
    *                          terminator.
    *  const char *pattern:    The regex pattern to match against.
 * RETURN:
    *   true: The string matches the pattern.
    *   false: The string does not match the pattern.
 */
bool stringMatchesRegex(const char *string, size_t bufSize, const char *pattern) {
    if (!string || !pattern || bufSize <= BUFFER_SIZE_OF_ZERO) {   // invalid input parameters
        return false;
    }

    if (!isNullTerminated(string, bufSize)) {
        return false;
    }

    size_t length = strlen(string);
    if (length == 0 || length > (bufSize - BUFFER_SIZE_OF_ONE)) {
        return false;
    }

    regex_t regex;

    // REG_EXTENDED allows  +, *, ^, $, etc.
    if (regcomp(&regex, pattern, REG_EXTENDED) != SUCCESS) {
        return false;   // Regex could not be compiled.
    }

    // Execute the compiled pattern against the input string.
    int result = regexec(&regex, string, 0, NULL, 0);
    regfree(&regex);   // Free the compiled regular expression.

    return result == SUCCESS;
}

// #####################################################################################################################
// Client Specific Function Definitions
// #####################################################################################################################

/*
 * FUNCTION: convertToInt
 * PROGRAMMER: Tyler Gee
 * DESCRIPTION:
 *  Attempts to convert a string to an integer using strtol().
 *  If conversion fails or if the string contains a non-numerical character is
 *  found, the function returns false.
 * PARAMETERS:
 *  const char *buffer:     String to convert.
 *  int *result:            Pointer to integer to store the result.
 * RETURN:
 *  true: Conversion successful and base 10 integer stored in result.
 *  false: Conversion failed (not an valid integer or contains non-digits).
 */
bool convertToInt(const char *buffer, int *result) {
    if (!buffer || !result) {
        return false;   // Invalid input
    }

    const int MAX_INT_DIGITS = 11;   // 32-bit int including sign
    char      bufCopy[MAX_INT_DIGITS + 1];
    snprintf(bufCopy, sizeof(bufCopy), "%s", buffer);   // create copy

    // Attempt to convert using strtol()
    errno                = 0;      // clear error flag
    char *numEndPtr      = NULL;   // success if endPtr points to '\0'
    long  convertedValue = strtol(bufCopy, &numEndPtr, 10);

    // Check for errors
    if (errno != 0 || *numEndPtr != '\0') {
        return false;   // conversion failed
    }

    // Assign the converted value and typecast to int
    *result = (int)convertedValue;
    return true;
}

/*
 * FUNCTION: getInputFromClient
 * PROGRAMMER: Tyler Gee
 * DESCRIPTION:
 *  Reads a line of text from the client's stdin input stream and stores it in
 *  the destination buffer. If the input exceeds bufSize - 1 characters or only
 *  a newline characters is received, an error message related to the label is
 *  printed to the console and the function returns false. In the case of an
 *  error, all characters in the buffer are discarded.
 *
 * PARAMETERS:
 *  const char *label:  Used to prompt the user for input or contents of related
 *                      error messages.
 *  char *buffer:       Buffer to store input, **always** set if successful.
 *  size_t bufSize:     Max size of the buffer in bytes, including the null
 *                      terminator.
 *  char *destination:  pointer to a buffer to store the input string in
 *                      **Optional** Set to NULL to avoid storing input in
 *                      *destination pointer.
 * RETURN:
 *  true:   Input was successfully read and stored in buffer (and destination).
 *  false:  An error occurred or was found during input reading and processing.
 */
bool getInputFromClient(
    const char *label, char *buffer, size_t bufSize, char *destination
) {
    int err = 0;

    printf("%s: ", label);
    if ((err = getInputFromStream(stdin, buffer, bufSize, false)) == SUCCESS) {
        // Reset timeout on successful input
        reset_timeout();
        
        if (destination) {
            snprintf(destination, bufSize, "%s", buffer);
        }
        return true;
    }

    printInputError(label, err, bufSize);
    return false;
}

/*
 * FUNCTION: getTripDestination
 * PROGRAMMER: Tyler Gee
 * DESCRIPTION:
 *  Reads a line of text from the client's stdin input stream and stores it in
 *  the destination buffer. If input exceeds MAX_DESTINATION_LEN - 1 characters,
 *  an error message related to the label is printed to the console and the
 *  function returns false.
 *
 *  Use this function to set the destination field of a Trip struct.
 *
 * PARAMETERS:
 *  char *destination: Buffer to store the destination input in.
 *
 * RETURN:
 *  true: Input was successfully read and stored in *destination.
 *  false: An error occurred or was found during input reading and processing.
 */
bool getTripDestination(char *destination) {
    char buffer[MAX_DESTINATION_LEN] = {0};
    return getInputFromClient("Destination", buffer, MAX_DESTINATION_LEN, destination);
}

/* FUNCTION: splitClientName
 * PROGRAMMER: Tyler Gee
 * DESCRIPTION:
 *  Split a client's full name into first and last names at the first
 *  occurrence of a space.
 *
 *  All characters before the space are copied in the buffer pointed to by
 *  *firstName, and all characters after the space are copied into the buffer
 *  pointed to by *lastName.
 *
 *  IMPORTANT NOTE: Validation of input is not performed in this function, the
 *  caller is responsible for ensuring the input is valid and safe:
 *      - buffer is not NULL
 *      - buffer contains at least one space.
 *      - The buffer is null-terminated.
 *      - firstName and lastName have enough space for copied values
 *
 * PARAMETERS:
 *  const char *buffer:     buffer containing a space to split at
 *  size_t bufSize:         Max size of the buffer in bytes, including \0
 *  char *firstName:        Pointer to store the first split string into
 *  char *lastName          Pointer to store the remaining bytes into
 * RETURN: None
 */
void splitClientName(
    const char *buffer, size_t bufSize, char *firstName, char *lastName
) {
    char *spacePosition = strchr(buffer, ' ');
    if (spacePosition == NULL) {   // Defensive check — should not happen
        firstName[0] = '\0';
        lastName[0]  = '\0';
        return;
    }

    int firstNameLength = spacePosition - buffer;
    // -1 to exclude the space when calculating lastNameLength
    // int lastNameLength  = (int)(strlen(buffer) - firstNameLength - 1);

    // Copy first name
    // "%.*s" prints N chars -> N = firstNameLength, snprintf() null terminates
    // added +1 to firstNameLength to account for null terminator (missing letters) -cy
    //
    snprintf(firstName, firstNameLength + 1, "%.*s", firstNameLength, buffer);

    // Copy last name
    snprintf(lastName, bufSize - firstNameLength - 1, "%s", spacePosition + 1);
}

/*
 * FUNCTION: getClientName
 * PROGRAMMER: Tyler Gee
 * DESCRIPTION:
 *  Prompts the user for a client's full name and validate input against the
 *  regex pattern "REGEX_NAME" and a max length of MAX_CLIENT_NAME_LEN defined
 *  in shared.h. If the input is valid, the full name input is split into first
 *  and last names and stored in the provided buffers. If the input is invalid,
 *  an error message is printed to the console and the function returns false.
 *
 * PARAMETERS:
 *  char *firstName: Buffer to store the first name into
 *  char *lastName:  Buffer to store the last name into
 *
 * RETURN:
 *  true: Input was successfully validated and split into first and last names.
 *  false: An error occurred/was found during input validation and/or splitting.
 */
bool getClientName(char *firstName, char *lastName) {
    char buffer[MAX_NAME_LEN] = {0};

    if (!getInputFromClient("Name", buffer, MAX_NAME_LEN, NULL)) {
        return false;
    }

    if (!stringMatchesRegex(buffer, MAX_NAME_LEN, REGEX_NAME)) {
        printf("Invalid Name Input: Must be in 'First Last' format\n");
        return false;
    }

    splitClientName(buffer, MAX_NAME_LEN, firstName, lastName);
    return true;
}

/*
 * FUNCTION: getClientAge
 * PROGRAMMER: Tyler Gee
 * DESCRIPTION:
 *  Prompts the user for a client's age and validate input against the regex
 *  pattern "REGEX_NUMBER" defined in shared.h. If the input is valid attempt to
 *  convert the input to an integer and store it in the provided pointer. If the
 *  input is invalid, an error message is printed to the console and the
 *  function returns false.
 *
 *  If the input is outside the valid range MIN_CLIENT_AGE to MAX_CLIENT_AGE
 *  (Provided in shared.h), an error message is printed to the console and the
 *  function returns false.
 *
 * PARAMETERS:
 *  int *age: Pointer to store the client's age into
 *
 * RETURN:
 *  true: Input was successfully validated and converted to an integer.
 *  false: An error occurred/was found during input validation and/or conversion
 */
bool getClientAge(int *age) {
    const size_t bufSize = BUFFER_SIZE_OF_FOUR;   // Max age is 3 characters + null terminator
    char         buffer[bufSize];

    // age cannot be NULL
    if (!age) {
        return false;
    }

    if (!getInputFromClient("Age", buffer, bufSize, NULL)) {
        return false;
    }

    int  result  = 0;
    bool isMatch = stringMatchesRegex(buffer, bufSize, REGEX_NUMBER);
    if (!isMatch || !convertToInt(buffer, &result)
        || (result < MIN_CLIENT_AGE || result > MAX_CLIENT_AGE)) {
        printf(
            "Invalid Age Input: Must be a number between %d and %d.\n", MIN_CLIENT_AGE,
            MAX_CLIENT_AGE
        );
        return false;
    }

    *age = result;
    return true;
}

/*
 * FUNCTION: getClientAddress
 * PROGRAMMER: Tyler Gee
 * DESCRIPTION:
 *  Prompts the user for a client's address and store it in the provided buffer.
 *  If input exceeds MAX_ADDRESS_LEN - 1 characters, an error message related to
 *  the label is printed to the console and the function returns false.
 *
 * PARAMETERS:
 *  char *address: Buffer to store the client's address into
 *
 * RETURN:
 *  true: Input was successfully read and stored in *address.
 *  false: An error occurred or was found during input reading and processing.
 */
bool getClientAddress(char *address) {
    char buffer[MAX_ADDRESS_LEN] = {0};
    return getInputFromClient("Address", buffer, MAX_ADDRESS_LEN, address);
}

/*
 * FUNCTION: clientToString
 * PROGRAMMER: Tyler Gee
 * DESCRIPTION:
 *  Converts a Client struct into a comma-separated string in the format:
 *  "firstName,lastName,age,address". Dynamically allocates memory for
 *  the returned string or returns NULL if any of the required fields are
 *  missing, invalid or a memory allocation error occurs.
 *
 *  The returned string must be freed by the caller.
 *
 * PARAMETERS:
 *  const Client *client: Pointer to the Client struct to convert to a string.
 *
 * RETURN:
 *  char *: Dynamically allocated string representing the client's information
 *          int the format specified or NULL if an error occurred, a field is
 *          missing or memory allocation fails.
 *
 */
char *clientToString(const Client *client) {
    if (!client || strlen(client->firstName) == BUFFER_SIZE_OF_ZERO || strlen(client->lastName) == BUFFER_SIZE_OF_ZERO
        || strlen(client->address) == BUFFER_SIZE_OF_ZERO || client->age <= BUFFER_SIZE_OF_ZERO) {
        return NULL;
    }
    // (*__stream = NULL, * __n = 0) -> snprint calculates characters count
    const int totalLength = snprintf(
        NULL, 0, "%s,%s,%d,%s", client->firstName, client->lastName, client->age,
        client->address
    );

    // Allocate memory for the client string + null terminator
    char *clientString = calloc(totalLength + 1, sizeof(char));
    if (!clientString) {
        printf("Memory allocation failed\n");
        return NULL;
    }

    // Construct the client string
    snprintf(
        clientString, totalLength + 1, "%s,%s,%d,%s", client->firstName, client->lastName,
        client->age, client->address
    );

    return clientString;
}

/*
 * FUNCTION: writestringToFIFO
 * PROGRAMMER: Tyler Gee & Cy Iver Torrefranca
 * DESCRIPTION:
    *  Writes a string to a named FIFO stream.
 * PARAMETERS:
    *  const char *fifoname: Name of the FIFO to write to.
    *  const char *string: String to write to the FIFO.
    *  bool showConnectionMsg: If true, displays "Waiting for server..." and "Connected to server!" messages.
 * RETURN:
    *  int: Returns 0 on success, ERROR on failure.
 */
int writestringToFIFO(const char *fifoname, const char *string, bool showConnectionMsg) {
    // Open FIFO stream for writing
    if (showConnectionMsg) {
        printf("Waiting for server...\n");
    }
    int fd = open(fifoname, O_WRONLY);
    if (fd == -1) {   // Check for error
        perror("Error opening FIFO stream for writing");
        return ERROR;
    }
    if (showConnectionMsg) {
        printf("Connected to server!\n");
    }

    // Write string plus newline to FIFO
    size_t len = strlen(string);
    char *buffer = malloc(len + 2); // +1 for newline, +1 for null terminator
    if (!buffer) {
        perror("Memory allocation failed");
        close(fd);
        return ERROR;
    }

    snprintf(buffer, len + 2, "%s\n", string);

    ssize_t bytesWritten = write(fd, buffer, len + 1);
    if (bytesWritten == ERROR) {   // Check for error
        perror("Error writing to FIFO stream");
        free(buffer);
        close(fd);
        return ERROR;
    }
    
    printf("Sent to server: %s\n", string);
    free(buffer);
    close(fd);   // close fifo
    return SUCCESS; // success
}
