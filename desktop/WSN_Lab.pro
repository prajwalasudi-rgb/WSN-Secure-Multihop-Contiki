# Open this file in Qt Creator to load both apps at once.
TEMPLATE = subdirs
SUBDIRS = serial_link ambient_temperature
serial_link.file = apps/serial_link/SerialLink.pro
ambient_temperature.file = apps/ambient_temperature/AmbientTemperature.pro
