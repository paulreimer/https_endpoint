#
# Component Makefile
#

COMPONENT_ADD_INCLUDEDIRS := \
	picojson

COMPONENT_SRCDIRS := \
	picojson

CFLAGS += \
	-DPICOJSON_USE_INT64=1
