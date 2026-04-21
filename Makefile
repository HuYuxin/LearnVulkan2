CFLAGS = -std=c++17
CFLAGS += -Iexternal/stb
CFLAGS += -Iexternal/volk
CFLAGS += -Iexternal/imgui -Iexternal/imgui/backends -DIMGUI_IMPL_VULKAN_USE_VOLK

LDFLAGS = -lglfw -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

DEBUG_FLAGS = -g -O0
CFLAGS += $(DEBUG_FLAGS)

SRCS = main.cpp BoundingBox.cpp Frustum.cpp VulkanInstance.cpp Camera.cpp utility.cpp Object.cpp glTF3DModel.cpp\
		external/volk/volk.c \
		external/imgui/imgui_demo.cpp external/imgui/imgui_draw.cpp external/imgui/imgui_tables.cpp external/imgui/imgui_widgets.cpp external/imgui/imgui.cpp \
		external/imgui/backends/imgui_impl_vulkan.cpp \
		external/imgui/backends/imgui_impl_glfw.cpp \
		external/imgui/misc/cpp/imgui_stdlib.cpp
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