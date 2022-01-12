# Modified from provided makefile on LMS

# CC - compiler
# OBJ - compiled source files that should be linked
# COPT - compiler flags
# BIN - binary
CC=clang
OBJ=server.o cache.o message.o log.o
COPT=-Wall -Wpedantic -g

# Rules of the form
#     target_to_be_made : dependencies_to_be_up-to-date_first
#     <tab>commands_to_make_target
# (Note that spaces will not work.)

dns_svr: main.c $(OBJ)
	$(CC) -o dns_svr main.c $(OBJ) $(COPT) -pthread

server.o: server.c server.h
	$(CC) -c server.c cache.c message.c log.c $(COPT)

cache.o: cache.c cache.h
	$(CC) -c message.c log.c $(COPT)

message.o: message.c message.h
	$(CC) -c message.c $(COPT)

log.o: log.c log.h
	$(CC) -c log.c message.c $(COPT)

# Wildcard rule to make any  .o  file,
# given a .c and .h file with the same leading filename component
%.o: %.c %.h
	$(CC) -c $< $(COPT) -g

format:
	clang-format -i *.c *.h

clean:
	rm -f *.o
	rm -f dns_svr