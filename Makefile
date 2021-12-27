CC = clang++
INC = include
# Use below if your OSX is using Intel's chip.
# CFLAGS = -std=c++11 -stdlib=libc++ -lglfw -lglog
CFLAGS = -std=c++11 -stdlib=libc++ -L/opt/homebrew/lib -lglfw -lglog
TARGET = ./build/main
SRCS = main.cc glad.cc error.cc shader.cc texture.cc strings.cc stab_image.cc camera.cc
MAIN = main

all: $(MAIN)
$(MAIN): $(SRCS)
  # Use below if your OSX is using Intel's chip.
	# $(CC) $(CFLAGS) $(SRCS) -I $(INC) -o $(TARGET)
	mkdir -p build || true
	$(CC) $(CFLAGS) $(SRCS) -I $(INC) -I /opt/homebrew/include -o $(TARGET)

clean:
	$(RM) *.o $(TARGET)
