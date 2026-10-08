# Contiki 3.0 project: sensor node v1: neighbour discovery
# Build:  make TARGET=remote            (Zolertia RE-Mote, TI CC2538)
# Flash:  make TARGET=remote Neighbor_Discovery.upload
CONTIKI_PROJECT = Neighbor_Discovery
all: $(CONTIKI_PROJECT)

# uses the Rime stack (broadcast/unicast/runicast), not IPv6
CONTIKI_WITH_RIME = 1
# powf() in the key exchange
TARGET_LIBFILES += -lm
# path to a Contiki 3.0 checkout
CONTIKI ?= ../../contiki
include $(CONTIKI)/Makefile.include
