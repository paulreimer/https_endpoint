#
# Component Makefile
#

COMPONENT_ADD_INCLUDEDIRS := \
	cpp17_headers/include \
	flatbuffers/include \
	picojson \
	src

COMPONENT_SRCDIRS := \
	flatbuffers/src \
	picojson \
	src

CXXFLAGS += \
	-DFLATBUFFERS_NO_ABSOLUTE_PATH_RESOLUTION \
	-DPICOJSON_USE_INT64=1

flatbuffers/src/idl_parser.o: CXXFLAGS += -Wno-maybe-uninitialized -Wno-type-limits
