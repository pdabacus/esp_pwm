ROOT    = /home/pavan/.arduino/03_esp_pwm_web
INSTALL = /home/pavan/.arduino15
SRC     = pwm_web.ino
OUTPUT  = pwm_web.ino.bin
BIN     = bin
CACHE   = cache
COMM    = /dev/ttyUSB0
BAUD_FL = 115200 #flashing baud rate
BAUD_SR = 9600  #serial monitor baud rate
CXX     = arduino-builder -compile
CXXOPTS = -warnings=all -verbose -logger=humantags \
          -build-path=$(ROOT)/$(BIN) -build-cache=$(ROOT)/$(CACHE) \
          -vid-pid=10C4_EA60 -ide-version=10819 \
          -fqbn=esp8266:esp8266:generic:xtal=80,vt=flash,exception=disabled,stacksmash=disabled,ssl=all,mmu=3232,non32xfer=fast,ResetMethod=nodemcu,CrystalFreq=26,FlashFreq=40,FlashMode=dout,eesz=1M64,led=2,sdk=nonosdk_190703,ip=lm2f,dbg=Disabled,lvl=None____,wipe=none,baud=115200 \
          -hardware=/usr/share/arduino/hardware -hardware=$(INSTALL)/packages \
          -tools=/usr/share/arduino/tools-builder -tools=$(INSTALL)/packages \
          -prefs=build.warn_data_percentage=75 \
          -prefs=runtime.tools.xtensa-lx106-elf-gcc.path=$(INSTALL)/packages/esp8266/tools/xtensa-lx106-elf-gcc/3.0.4-gcc10.3-1757bed \
          -prefs=runtime.tools.xtensa-lx106-elf-gcc-3.0.4-gcc10.3-1757bed.path=$(INSTALL)/packages/esp8266/tools/xtensa-lx106-elf-gcc/3.0.4-gcc10.3-1757bed \
          -prefs=runtime.tools.python3.path=$(INSTALL)/packages/esp8266/tools/python3/3.7.2-post1 \
          -prefs=runtime.tools.python3-3.7.2-post1.path=$(INSTALL)/packages/esp8266/tools/python3/3.7.2-post1 \
          -prefs=runtime.tools.mkspiffs.path=$(INSTALL)/packages/esp8266/tools/mkspiffs/3.0.4-gcc10.3-1757bed \
          -prefs=runtime.tools.mkspiffs-3.0.4-gcc10.3-1757bed.path=$(INSTALL)/packages/esp8266/tools/mkspiffs/3.0.4-gcc10.3-1757bed \
          -prefs=runtime.tools.mklittlefs.path=$(INSTALL)/packages/esp8266/tools/mklittlefs/3.0.4-gcc10.3-1757bed \
          -prefs=runtime.tools.mklittlefs-3.0.4-gcc10.3-1757bed.path=$(INSTALL)/packages/esp8266/tools/mklittlefs/3.0.4-gcc10.3-1757bed

FLASHER = python3 -I $(INSTALL)/packages/esp8266/hardware/esp8266/3.0.2/tools/upload.py
FLASHOPT= --chip esp8266 --port $(COMM) --baud $(BAUD_FL) --before default_reset --after hard_reset

.PHONY: all init flash clean

all: $(OUTPUT) flash

init:
	@if [ ! -d $(BIN) ]; then mkdir $(BIN); fi
	@if [ ! -d $(CACHE) ]; then mkdir $(CACHE); fi

$(OUTPUT): init $(SRC)
	$(CXX) $(CXXOPTS) $(SRC)
	cp $(BIN)/$(OUTPUT) $(OUTPUT)

flash:
	$(FLASHER) $(FLASHOPT) write_flash 0x0 $(OUTPUT)

serial:
	@echo "screen connection to $(COMM): to exit press CTRL+A K Y"
	@sleep 2
	screen $(COMM) $(BAUD_SR)

clean:
