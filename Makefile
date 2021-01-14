
# Makefile for genplus SDL2
#
# (c) 1999, 2000, 2001, 2002, 2003  Charles MacDonald
# modified by Eke-Eke <eke_eke31@yahoo.fr>
#
# Defines :
# -DLSB_FIRST : for little endian systems.
# -DLOGERROR  : enable message logging
# -DLOGVDP    : enable VDP debug messages
# -DLOGSOUND  : enable AUDIO debug messages
# -DLOG_SCD   : enable SCD debug messages
# -DLOG_CDD   : enable CDD debug messages
# -DLOG_CDC   : enable CDC debug messages
# -DLOG_PCM   : enable PCM debug messages
# -DLOGSOUND  : enable AUDIO debug messages
# -D8BPP_RENDERING  - configure for 8-bit pixels (RGB332)
# -D15BPP_RENDERING - configure for 15-bit pixels (RGB555)
# -D16BPP_RENDERING - configure for 16-bit pixels (RGB565)
# -D32BPP_RENDERING - configure for 32-bit pixels (RGB888)
# -DUSE_LIBCHDR      : enable CHD file support
# -DUSE_LIBTREMOR    : enable OGG file support for CD emulation using provided TREMOR library
# -DUSE_LIBVORBIS    : enable OGG file support for CD emulation using external VORBIS library
# -DISABLE_MANY_OGG_OPEN_FILES : only have one OGG file opened at once to save RAM
# -DMAXROMSIZE       : defines maximal size of ROM/SRAM buffer (also shared with CD hardware)
# -DHAVE_YM3438_CORE : enable (configurable) support for Nuked cycle-accurate YM3438 core
# -DHOOK_CPU         : enable CPU hooks

.DEFAULT_GOAL := all

NAME	  = s2cx
SUFFIX	  = 
PKGCONFIG = pkg-config
DEBUG	 ?= 0
STATIC	 ?= 1
VERBOSE  ?= 0
PROFILE	 ?= 0
STRIP	 ?= strip

# =============================================================================
# Detect default platform if not explicitly specified
# =============================================================================

ifeq ($(OS),Windows_NT)
	PLATFORM ?= Windows
else
	UNAME_S := $(shell uname -s)

	ifeq ($(UNAME_S),Linux)
		PLATFORM ?= Linux
	endif

	ifeq ($(UNAME_S),Darwin)
		PLATFORM ?= macOS
	endif

endif

ifdef EMSCRIPTEN
	PLATFORM = Emscripten
endif

PLATFORM ?= Unknown

# =============================================================================

OUTDIR = bin/$(PLATFORM)
OBJDIR = objects/$(PLATFORM)

include Makefile_cfgs/Platforms/$(PLATFORM).cfg

# =============================================================================

ifeq ($(STATIC),1)
	PKGCONFIG +=  --static
endif

ifeq ($(DEBUG),1)
	CFLAGS += -g
	DEFINES += -DDEBUG
	STRIP = :
else
	CFLAGS += -O3
endif

ifeq ($(PROFILE),1)
	CFLAGS += -pg -g -fno-inline-functions -fno-inline-functions-called-once -fno-optimize-sibling-calls -fno-default-inline
endif

ifeq ($(VERBOSE),0)
	CC := @$(CC)
	CXX := @$(CXX)
endif

ifeq ($(ENABLE_DIALOGS),1)
	INCLUDES += -I./lib/portable-file-dialogs
	DEFINES += -DENABLE_DIALOGS
endif

# =============================================================================
# Backends
# =============================================================================

BACKEND_VIDEO ?= sdl2
BACKEND_INPUT ?= sdl2
BACKEND_AUDIO ?= soloud

include Makefile_cfgs/Backends/Video/$(BACKEND_VIDEO).cfg
include Makefile_cfgs/Backends/Input/$(BACKEND_INPUT).cfg
include Makefile_cfgs/Backends/Audio/$(BACKEND_AUDIO).cfg

DEFINES += \
	-DBACKEND_VIDEO_$(BACKEND_VIDEO) \
	-DBACKEND_INPUT_$(BACKEND_INPUT) \
	-DBACKEND_AUDIO_$(BACKEND_AUDIO)

# =============================================================================

CFLAGS += `$(PKGCONFIG) --cflags zlib vorbisfile jansson`
LIBS   += `$(PKGCONFIG) --libs-only-l --libs-only-L zlib vorbisfile jansson`

CFLAGS += -Wno-strict-aliasing -Wno-narrowing -Wno-write-strings -msse -fpermissive

ifeq ($(STATIC),1)
	CFLAGS += -static
endif

LDFLAGS   =

DEFINES   += -DLSB_FIRST -DUSE_32BPP_RENDERING \
			-DMAXROMSIZE=4194304 -DHAVE_NO_SPRITE_LIMIT \
			-DM68K_OVERCLOCK_SHIFT=20 -DVDP_FIX_DMA_BOUNDARY_BUG #-DHOOK_CPU

INCLUDES  += 	-I./src \
				-I./src/core \
				-I./src/core/z80 \
				-I./src/core/m68k \
				-I./src/core/sound \
				-I./src/core/input_hw \
				-I./src/core/cart_hw \
				-I./src/core/cart_hw/svp \
				-I./src/core/cd_hw \
				-I./src/core/debug \
				-I./src/core/ntsc \
				-I./src/backends/input \
				-I./src/backends/sound \
				-I./src/backends/video \
				-I./lib/argparse

INCLUDES += $(LIBS)

# Core Sources
SOURCES	+=	src/core/z80/z80 \
			src/core/m68k/m68kcpu \
			src/core/m68k/s68kcpu \
			src/core/genesis \
			src/core/vdp_ctrl \
			src/core/vdp_render \
			src/core/system \
			src/core/io_ctrl \
			src/core/mem68k \
			src/core/memz80 \
			src/core/membnk \
			src/core/state \
			src/core/loadrom	\
			src/core/input_hw/input \
			src/core/input_hw/gamepad \
			src/core/input_hw/lightgun \
			src/core/input_hw/mouse \
			src/core/input_hw/activator \
			src/core/input_hw/xe_1ap \
			src/core/input_hw/teamplayer \
			src/core/input_hw/paddle \
			src/core/input_hw/sportspad \
			src/core/input_hw/terebi_oekaki \
			src/core/input_hw/graphic_board \
			src/core/sound/sound \
			src/core/sound/psg \
			src/core/sound/opll \
			src/core/sound/ym2413 \
			src/core/sound/ym3438 \
			src/core/sound/ym2612 \
			src/core/sound/blip_buf \
			src/core/sound/eq \
			src/core/cart_hw/sram \
			src/core/cart_hw/svp/svp \
			src/core/cart_hw/svp/ssp16 \
			src/core/cart_hw/ggenie \
			src/core/cart_hw/areplay \
			src/core/cart_hw/eeprom_93c \
			src/core/cart_hw/eeprom_i2c \
			src/core/cart_hw/eeprom_spi \
			src/core/cart_hw/md_cart \
			src/core/cart_hw/sms_cart \
			src/core/cd_hw/scd \
			src/core/cd_hw/cdd \
			src/core/cd_hw/cdc \
			src/core/cd_hw/gfx \
			src/core/cd_hw/pcm \
			src/core/cd_hw/cd_cart \
			src/core/debug/cpuhook \
			src/core/ntsc/sms_ntsc \
			src/core/ntsc/md_ntsc

# Main Sources
SOURCES	+=	src/main \
			src/config \
			src/error \
			src/ioapi \
			src/unzip \
			src/fileio \
			src/ips \
			src/inputact \
			src/gamehacks

# Main Sources
SOURCES	+=	lib/argparse/argparse

PKGSUFFIX ?= $(SUFFIX)

BINPATH = $(OUTDIR)/$(NAME)$(SUFFIX)
PKGPATH = $(OUTDIR)/$(NAME)$(PKGSUFFIX)

OBJECTS += $(addprefix $(OBJDIR)/, $(addsuffix .o, $(SOURCES)))

$(shell mkdir -p $(OUTDIR))
$(shell mkdir -p $(OBJDIR))

$(OBJDIR)/%.o: %.c
	@mkdir -p $(@D)
	@echo -n Compiling $<...
	$(CC) -c $(CFLAGS) $(INCLUDES) $(DEFINES) $< -o $@
	@echo " Done!"

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(@D)
	@echo -n Compiling $<...
	$(CXX) -c $(CFLAGS) $(INCLUDES) $(DEFINES) $< -o $@
	@echo " Done!"

$(BINPATH): $(OBJDIR) $(OBJECTS)
	@echo -n Linking...
	$(CXX) $(CFLAGS) $(LDFLAGS) $(OBJECTS) -o $@ $(LIBS)
	@echo " Done!"
	$(STRIP) $@

ifeq ($(BINPATH),$(PKGPATH))
all: $(BINPATH)
else
all: $(PKGPATH)
endif

clean:
	rm -rf $(OBJDIR)