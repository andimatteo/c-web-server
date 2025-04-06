CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
BIN_DIR = ..

OBJ = main.o server.o worker_process.o thread_pool.o request_parser.o http_response.o \
      event_loop.o file_cache.o performance_log.o

all: $(BIN_DIR)/server

$(BIN_DIR)/server: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

main.o: main.c server.h worker_process.h thread_pool.h file_cache.h performance_log.h
server.o: server.c server.h
worker_process.o: worker_process.c worker_process.h thread_pool.h event_loop.h
thread_pool.o: thread_pool.c thread_pool.h request_parser.h http_response.h
request_parser.o: request_parser.c request_parser.h
http_response.o: http_response.c http_response.h request_parser.h file_cache.h performance_log.h
event_loop.o: event_loop.c event_loop.h
file_cache.o: file_cache.c file_cache.h
performance_log.o: performance_log.c performance_log.h

clean:
	rm -f *.o $(BIN_DIR)/server $(BIN_DIR)/performance.log
