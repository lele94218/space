CC = clang++
INC = include
CFLAGS = -std=c++11 -stdlib=libc++ -lglfw -lglog
TARGET = ./build/main
SRCS = main.cc glad.cc error.cc shader.cc texture.cc strings.cc stab_image.cc
MAIN = main

all: $(MAIN)
$(MAIN): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -I $(INC) -o $(TARGET)

clean:
	$(RM) *.o $(TARGET)
