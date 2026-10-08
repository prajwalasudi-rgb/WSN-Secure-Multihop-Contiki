# Contiki 3.0 project: sensor node v2: neighbour discovery + hop-count routing
# Build:  make TARGET=remote            (Zolertia RE-Mote, TI CC2538)
# Flash:  make TARGET=remote Hop_Routing.upload
CONTIKI_PROJECT = Hop_Routing
all: $(CONTIKI_PROJECT)

# uses the Rime stack (broadcast/unicast/runicast), not IPv6
CONTIKI_WITH_RIME = 1
# powf() in the key exchange
TARGET_LIBFILES += -lm
# path to a Contiki 3.0 checkout
CONTIKI ?= ../../contiki
include $(CONTIKI)/Makefile.include
