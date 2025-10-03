################################################################################
#                                    Program                                   #
################################################################################
CLIENT_APPNAME = client
SERVER_APPNAME = server
################################################################################
#                              File Structure Linux                            #
################################################################################
# 	Source Directory
SRCDIR	= src
# 	Dependency Directory
IDIR    = inc
# 	Object Directory
OBJDIR  = obj
# 	Executable Directory
EXECDIR = bin
################################################################################
#                                 File Names Linux                             #
################################################################################
# Client Source files
CLIENT_SRC	:= $(SRCDIR)/client.c
# Server Source files
SERVER_SRC  := $(SRCDIR)/server.c
# Shared Header file
SHARED_HDR  := $(IDIR)/shared.h
# Client Object files
CLIENT_OBJ  := $(OBJDIR)/client.o
# Server Object files
SERVER_OBJ  := $(OBJDIR)/server.o
# Client Executable
CLIENT_EXEC := $(EXECDIR)/client
# Server Executable
SERVER_EXEC := $(EXECDIR)/server
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
CFLAGS 		= -Wall -Wextra -Wpedantic -Werror $(CSTANDARD) -I$(IDIR)
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

# Create /obj and /bin (mkdir -p flag: No error if exists)
$(OBJDIR) $(EXECDIR):
	@$(ECHO_MKDIR)
	@mkdir -p $@

# Compile client.c -> obj/client.o (depends on shared.h, order-only prerequisite Ensures /obj exists)
$(CLIENT_OBJ): $(CLIENT_SRC) $(SHARED_HDR) | $(OBJDIR)
	@$(ECHO_CLIENT_CC)
	@$(CC) $(CFLAGS) -c $(CLIENT_SRC) -o $(CLIENT_OBJ)

# Compile server.c -> obj/server.o (depends on shared.h, order-only prerequisite Ensures /obj exists)
$(SERVER_OBJ): $(SERVER_SRC) $(SHARED_HDR) | $(OBJDIR)
	@$(ECHO_SERVER_CC)
	@$(CC) $(CFLAGS) -c $(SERVER_SRC) -o $(SERVER_OBJ)

# Link client.o → bin/client (order-only prerequisite Ensures /bin exists)
$(CLIENT_EXEC): $(CLIENT_OBJ) | $(EXECDIR)
	@$(ECHO_CLIENT_LD)
	@$(CC) $(CFLAGS) $(CLIENT_OBJ) -o $(CLIENT_EXEC)

# Link server.o → bin/server (order-only prerequisite Ensures /bin exists)
$(SERVER_EXEC): $(SERVER_OBJ) | $(EXECDIR)
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
	@rm -f $(OBJDIR)/*.o $(CLIENT_EXEC) $(SERVER_EXEC) || true
	$(ECHO_CLEAN) Clean complete.

