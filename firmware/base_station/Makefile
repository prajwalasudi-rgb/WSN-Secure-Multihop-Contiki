# Contiki 3.0 project: base station / sink
# Build:  make TARGET=remote            (Zolertia RE-Mote, TI CC2538)
# Flash:  make TARGET=remote Base_Station.upload
CONTIKI_PROJECT = Base_Station
all: $(CONTIKI_PROJECT)

# uses the Rime stack (broadcast/unicast/runicast), not IPv6
CONTIKI_WITH_RIME = 1
# powf() in the key exchange
TARGET_LIBFILES += -lm
# path to a Contiki 3.0 checkout
CONTIKI ?= ../../contiki
include $(CONTIKI)/Makefile.include
