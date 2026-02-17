STB_INCLUDE_PATH = /home/yuxin-hu/Learning/stb
TINYOBJ_INCLUDE_PATH = /home/yuxin-hu/Learning/tinyobjloader
CFLAGS = -std=c++17 -I$(STB_INCLUDE_PATH) -I$(TINYOBJ_INCLUDE_PATH) -Iexternal/volk

LDFLAGS = -lglfw -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi


SRCS = main.cpp Cube.cpp Floor.cpp VulkanInstance.cpp Camera.cpp utility.cpp Object.cpp external/volk/volk.c
OBJS = $(SRCS:.cpp=.o)
OBJS := $(OBJS:.c=.o)
VulkanBasic: $(OBJS)
	g++ $(CFLAGS) -o VulkanBasic $(OBJS) $(LDFLAGS)

%.o: %.cpp
	g++ $(CFLAGS) -c $< -o $@

# Special rule for volk.c (since it's C, not C++)
external/volk/volk.o: external/volk/volk.c
	gcc -Iexternal/volk -c $< -o $@

.PHONY: test clean

test: VulkanBasic
	./VulkanBasic

clean:
	rm -f VulkanBasic $(OBJS)