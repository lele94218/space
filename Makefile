CC = clang++
INC = include
CFLAGS = -std=c++11 -stdlib=libc++ -lglfw
TARGET = ./build/main
SRCS = main.cc glad.cc error.cc shader.cc
MAIN = main

all: $(MAIN)
$(MAIN): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -I $(INC) -o $(TARGET)

clean:
	$(RM) *.o $(TARGET)
