CC = gcc
GLC = glslc

CFLAGS = -Wall -Wno-missing-braces -Wno-attributes -fPIC
LDFLAGS = -L/opt/hfs18.0/dsolib -L/$(HOME)/lib
INFLAGS = -I$(HOME)/dev
LIBS = -lm -ltanto -lvulkan -lxcb -lxcb-keysyms -lfreetype
GLFLAGS = --target-env=vulkan1.2
BIN = bin
LIB = $(HOME)/lib
LIBNAME = shaderboy

O = build
GLSL = shaders
SPV  = shaders/spv

NAME = shaderboy

DEPS =  \
		game.h \
		render.h \
		shaderboy.h \
		common.h \
		shaders/common.glsl 

OBJS =  \
		$(O)/game.o \
		$(O)/render.o \
		$(O)/shaderboy.o \

SHADERS = $(SPV)/post-frag.spv \
		  $(SPV)/color-frag.spv 

debug: CFLAGS += -g -DVERBOSE=1
debug: all

release: CFLAGS += -DNDEBUG -O3
release: all

all: bin lib tags shaders tanto

FRAGS := $(patsubst %.frag,$(SPV)/%-frag.spv,$(notdir $(wildcard $(GLSL)/*.frag)))
VERTS := $(patsubst %.vert,$(SPV)/%-vert.spv,$(notdir $(wildcard $(GLSL)/*.vert)))
shaders: $(FRAGS) $(VERTS)

clean: 
	rm -f $(O)/* $(LIB)/$(LIBNAME) $(BIN)/* $(SPV)/*

tags:
	ctags -R .

bin: main.c $(OBJS) $(DEPS) shaders
	$(CC) $(CFLAGS) $(INFLAGS) $(LDFLAGS) $(OBJS) $< -o $(BIN)/$(NAME) $(LIBS)

lib: $(OBJS) $(DEPS) shaders
	$(CC) -shared -o $(LIB)/lib$(LIBNAME).so $(OBJS)

.PHONY: tanto clean
tanto:
	cd tanto ; make ; cd ..

staticlib: $(OBJS) $(DEPS) shaders
	ar rcs $(LIB)/lib$(NAME).a $(OBJS)

$(O)/%.o:  %.c $(DEPS)
	$(CC) $(CFLAGS) $(INFLAGS) -c $< -o $@

$(SPV)/%-vert.spv: $(GLSL)/%.vert $(DEPS)
	$(GLC) $(GLFLAGS) $< -o $@

$(SPV)/%-frag.spv: $(GLSL)/%.frag $(DEPS)
	$(GLC) $(GLFLAGS) $< -o $@

$(SPV)/%-rchit.spv: $(GLSL)/%.rchit
	$(GLC) $(GLFLAGS) $< -o $@

$(SPV)/%-rgen.spv: $(GLSL)/%.rgen
	$(GLC) $(GLFLAGS) $< -o $@

$(SPV)/%-rmiss.spv: $(GLSL)/%.rmiss
	$(GLC) $(GLFLAGS) $< -o $@
