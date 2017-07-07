
UNAME := $(shell uname)

TARGET := main.exe

CC = gcc
CCFLAGS += -I/usr/include/opengl
LD = gcc
LDFLAGS +=

ifeq ($(UNAME), Darwin)
CCFLAGS += -Wno-unused-variable -Wno-unused-function -Wno-deprecated-declarations
LIBS += -framework GLUT -framework OpenGL mac/lib/libglpng.a -lm
else
ifeq ($(UNAME), Linux)
LIBS += -lglpng -lglut -lGL -lGLU -lm
else
LIBS += -lglpng -lglut32 -lglu32 -lopengl32 -lm
LIBS += img/icon/icon.o
TARGET := img/icon/icon.o $(TARGET)
endif
endif

# === Build rules ===

.PHONY: all clean

all: $(TARGET)

%.exe: %.c
	$(CC) $(CCFLAGS) $< -o $@ $(LIBS)

img/icon/icon.o: img/icon/icon.rc
	windres -i $< -o $@

clean:
	$(RM) $(TARGET) *~
