################################################################################
#                                    Program                                   #
################################################################################
# Name of the client executable (without extension)
CLIENT_APPNAME	= client
# Name of the server executable (without extension)
SERVER_APPNAME 	= server

################################################################################
#                              File Structure Linux                            #
################################################################################
# Source Directory
SRCDIR			= src
# Dependency Directory
IDIR    		= inc
# Object Directory
OBJDIR  		= obj
# Executable Directory
EXECDIR 		= bin

################################################################################
#                                 File Names Linux                             #
################################################################################
# := Means evaluate immediately not at time of use
# Client Source files
CLIENT_SRC		:= $(SRCDIR)/client.c
# Server Source files
SERVER_SRC  	:= $(SRCDIR)/server.c
# Client Object files
CLIENT_OBJ  	:= $(OBJDIR)/client.o
# Server Object files
SERVER_OBJ  	:= $(OBJDIR)/server.o
# Client Executable
CLIENT_EXEC 	:= $(EXECDIR)/client
# Server Executable
SERVER_EXEC 	:= $(EXECDIR)/server
LOG_FILE		:= travel_agency.log
FIFO_PIPE		:= travel_agency_fifo

################################################################################
#                          C Compiler Settings Linux                           #
################################################################################
# ?= Means default to if not set
CC        		?= cc
CSTANDARD		?= -std=c17
CFLAGS 			:= -Wall -Wextra -Wpedantic -Werror $(CSTANDARD) -I$(IDIR)

################################################################################
#                                  Linux Targets                               #
################################################################################
# Declare phony targets (not real files)
.PHONY: all client server run-client run-server clean clean-log clean-FIFO distclean

# Default target: build client and server, then run both
all: client server

# Build client executable and run it
client: $(CLIENT_EXEC)

# Build server executable and run it
server: $(SERVER_EXEC)

# Create /obj and /bin (mkdir -p flag: No error if exists)
$(OBJDIR) $(EXECDIR):
	@echo "Creating directory $@..."
	@mkdir -p $@

# Compile client.c -> obj/client.o (order-only prerequisite Ensures /obj exists)
$(CLIENT_OBJ): $(CLIENT_SRC) | $(OBJDIR)
	$(CC) $(CFLAGS) -c $(CLIENT_SRC) -o $(CLIENT_OBJ)

# Compile server.c -> obj/server.o (order-only prerequisite Ensures /obj exists)
$(SERVER_OBJ): $(SERVER_SRC) | $(OBJDIR)
	$(CC) $(CFLAGS) -c $(SERVER_SRC) -o $(SERVER_OBJ)

# Link client.o → bin/client (order-only prerequisite Ensures /bin exists)
$(CLIENT_EXEC): $(CLIENT_OBJ) | $(EXECDIR)
	$(CC) $(CFLAGS) $(CLIENT_OBJ) -o $(CLIENT_EXEC)

# Link server.o → bin/server (order-only prerequisite Ensures /bin exists)
$(SERVER_EXEC): $(SERVER_OBJ) | $(EXECDIR)
	$(CC) $(CFLAGS) $(SERVER_OBJ) -o $(SERVER_EXEC)

# Run the client program
run-client: $(CLIENT_EXEC)
	@echo "Running client..."
	@./$(CLIENT_EXEC)

# Run the server program
run-server: $(SERVER_EXEC)
	@echo "Running server..."
	@./$(SERVER_EXEC)
	
# Clean build artifacts
clean:
	@echo "Removing build artifacts..."
	@rm -f $(OBJDIR)/*.o $(CLIENT_EXEC) $(SERVER_EXEC) || true
	@echo "Build artifacts removed successfully."

# Clean log files
clean-log:
	@echo "Removing log file..."
	@rm -f $(LOG_FILE)
	@echo "All log files removed successfully..."

# Clean FIFO files
clean-FIFO:
	@echo "Removing FIFO pipt..."
	@rm -f $(FIFO_PIPE)
	@echo "FIFO pipe removed successfully..."

# Clean all generated files
distclean: clean clean-log clean-FIFO
	@echo "Removing build directories..."
	@rm -rf $(OBJDIR) $(EXECDIR)
	@echo "All build directories removed."

