ver=release
# The name of the executable to be created
TARGET := libsmpmalloc.a
# Compiler used
CC ?= gcc
# Extension of source files used in the project
SRC_EXT = c
# Path to the source directory, relative to the makefile
SRC_PATH = .

# General compiler flags
COMPILE_FLAGS = -Wall -MMD

INCLUDES = -I $(SRC_PATH)

OBJS = $(addsuffix .o, $(basename $(wildcard $(SRC_PATH)/*.$(SRC_EXT))))

$(TARGET): $(OBJS)
	echo $(OBJS)
	ar cqs $@ $^

-include $(OBJS:.o=.d)

%.o: $(SRC_PATH)/%.$(SRC_EXT)
	$(CC) $(COMPILE_FLAGS) -c -o $@ $< $(INCLUDES)
	@mv -f $*.d $*.d.tmp
	@sed 's,\($*\)\.o[ :]*,\1.d $@ : ,g' < $*.d.tmp > $*.d
	@rm -f $*.d.tmp

.PHONY: clean

clean:
	-rm -f $(SRC_PATH)/*.o $(TARGET) $(SRC_PATH)/*.d