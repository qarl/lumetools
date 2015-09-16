SHADER_NAME     = WrapAround
SHADER_TYPE     = len
SHADER_TYPE_DIR = CAMERA_SHADERS

TOP_DIR     = ..

DB_DIR      = $(TOP_DIR)/LumeTools
EXTRA_DIR   = $(TOP_DIR)/extras

INC      = -I$(EXTRA_DIR) -I/pkg/softimage/mental_ray/inc
CCOPT    = -O
LIB      = $(EXTRA_DIR)/liblume.a

SHADER_C  = $(SHADER_NAME).c
SHADER_O  = $(SHADER_NAME).o
SHADER_SO = $(SHADER_NAME).so
SHADER_DESC = $(SHADER_NAME).$(SHADER_TYPE)

SHELL = /sbin/sh

all: $(SHADER_SO) $(SHADER_DESC)

clean:
	rm -f *.o *.so *.$(SHADER_TYPE) *~ *# so_locations out.mi

install: all
	install -m 644 $(SHADER_SO) $(DB_DIR)/lib/$(SHADER_SO)
	install -m 644 $(SHADER_DESC) $(DB_DIR)/$(SHADER_TYPE_DIR)/$(SHADER_DESC)

$(SHADER_SO) : $(SHADER_O)
	$(LD) -shared -x -o $@ $(SHADER_O) $(LIB)
	$(EXTRA_DIR)/lmstrip $(SHADER_SO)

$(SHADER_O) : $(SHADER_C)
	$(CC) $(CCOPT) $(INC) -c $? -o $@

$(SHADER_DESC): $(SHADER_NAME).dg $(SHADER_NAME).doc $(SHADER_NAME).build
	m4 < $(SHADER_NAME).build > $@
