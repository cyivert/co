################################################################################
#                                    Program                                   #
################################################################################
CLIENT_APPNAME = client
SERVER_APPNAME = server
################################################################################
#                              File Structure Linux                            #
################################################################################
SRCDIR = .
IDIR = .
OBJDIR = .
EXECDIR = .
################################################################################
#                                 File Names Linux                             #
################################################################################
CLIENT_SRC = client.c
SERVER_SRC = server.c
CLIENT_OBJ = client.o
SERVER_OBJ = server.o
CLIENT_EXEC = client
SERVER_EXEC = server
################################################################################
#                          C Compiler Settings Linux                           #
################################################################################
# Compiler Options: cc, gcc, clang - Defaults to cc
CC        	?= cc
# C Standard Options: -std=c89, -std=c99, -std=c11
CSTANDARD	?= -std=c17
# The CFLAGS variables sets compile flags for gcc:
#  	-Wall		Give Verbose compile warnings
#  	-Wextra     Give extra warnings
#  	-Werror		Treat all warnings as errors
#  	-Wpedantic  Require strict conformance to the ISO C standard
CFLAGS 		= -Wall -Wextra -Wpedantic -Werror $(CSTANDARD)
################################################################################
#                                    Labels                                    #
################################################################################
# Padded labels for build output
LABEL_MKDIR  = [MKDIR]
LABEL_CC     = [CC   ]
LABEL_LD     = [LD   ]
LABEL_RUN    = [RUN  ]
LABEL_CLEAN  = [CLEAN]
# Director Creation Labels
ECHO_MKDIR        = @echo "$(LABEL_MKDIR) Creating directory $@"
# Compile Labels
ECHO_CLIENT_CC    = @echo "$(LABEL_CC) client.c -> client.o"
ECHO_SERVER_CC    = @echo "$(LABEL_CC) server.c -> server.o"
# Link Labels
ECHO_CLIENT_LD    = @echo "$(LABEL_LD) client.o -> $(CLIENT_EXEC)"
ECHO_SERVER_LD    = @echo "$(LABEL_LD) server.o -> $(SERVER_EXEC)"
# Execution Labels
ECHO_CLIENT_RUN   = @echo "$(LABEL_RUN) $(CLIENT_EXEC)" && echo ""
ECHO_SERVER_RUN   = @echo "$(LABEL_RUN) $(SERVER_EXEC)" && echo ""
# Clean Up Label
ECHO_CLEAN        = @echo "$(LABEL_CLEAN)"
################################################################################
#                                  Linux Targets                               #
################################################################################
.PHONY: all client server run-client run-server clean

all: client server
client: $(CLIENT_EXEC)
server: $(SERVER_EXEC)

# Compile client source to object file
$(CLIENT_OBJ): $(CLIENT_SRC)
	@$(ECHO_CLIENT_CC)
	@$(CC) $(CFLAGS) -c $(CLIENT_SRC) -o $(CLIENT_OBJ)

$(SERVER_OBJ): $(SERVER_SRC)
	@$(ECHO_SERVER_CC)
	@$(CC) $(CFLAGS) -c $(SERVER_SRC) -o $(SERVER_OBJ)

$(CLIENT_EXEC): $(CLIENT_OBJ)
	@$(ECHO_CLIENT_LD)
	@$(CC) $(CFLAGS) $(CLIENT_OBJ) -o $(CLIENT_EXEC)

$(SERVER_EXEC): $(SERVER_OBJ)
	@$(ECHO_SERVER_LD)
	@$(CC) $(CFLAGS) $(SERVER_OBJ) -o $(SERVER_EXEC)

# Run the client program
run-client: $(CLIENT_EXEC)
	@$(ECHO_CLIENT_RUN)
	@echo "─────────────────── PROGRAM OUTPUT ───────────────────"
	./$(CLIENT_EXEC)
	@echo && echo "──────────────────────────────────────────────────────"

# Run the server program
run-server: $(SERVER_EXEC)
	@$(ECHO_SERVER_RUN)
	@echo "─────────────────── PROGRAM OUTPUT ───────────────────"
	@./$(SERVER_EXEC)
	@echo && echo "──────────────────────────────────────────────────────"
# Clean build artifacts
clean:
	$(ECHO_CLEAN) Removing build artifacts...
	rm -f *.o $(CLIENT_EXEC) $(SERVER_EXEC) || true
	$(ECHO_CLEAN) Clean complete.

