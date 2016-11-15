TSCR = tscrapp.exe

CC = gcc
SRCPATH		= src
BUILDPATH	= build

CPFLAGS = -W -Wall -O2 -iquote"src/include"
LIB      = C:/MinGW/lib/libsetupapi.a C:/MinGW/lib/libws2_32.a

FILES = main fx2_interface usb_interface isc
OBJS	= $(addprefix $(BUILDPATH)/, $(addsuffix .o, $(FILES)))

all: $(TSCR)

$(TSCR): $(OBJS)
	@echo linking $(@F)
	@$(CC) $(CPFLAGS) $^ -o $@ $(LIB)

$(OBJS): build/%.o: src/%.c
	@echo Compiling $(<F)
	@$(CC) -MMD -c $(CPFLAGS) $< -o $@

clean:
	rm -rf $(TSCR) $(OBJS) build/*.d

-include $(OBJS:.o=.d)

.PHONY: all clean

