STB_INCLUDE_PATH = /home/yuxin-hu/Learning/stb
TINYOBJ_INCLUDE_PATH = /home/yuxin-hu/Learning/tinyobjloader
CFLAGS = -std=c++17 -I$(STB_INCLUDE_PATH) -I$(TINYOBJ_INCLUDE_PATH)

LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi


SRCS = main.cpp cube.cpp floor.cpp vulkanInstance.cpp
OBJS = $(SRCS:.cpp=.o)
VulkanBasic: $(OBJS)
	g++ $(CFLAGS) -o VulkanBasic $(OBJS) $(LDFLAGS)

%.o: %.cpp
	g++ $(CFLAGS) -c $< -o $@

.PHONY: test clean

test: VulkanBasic
	./VulkanBasic

clean:
	rm -f VulkanBasic