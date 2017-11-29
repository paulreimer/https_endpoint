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

# Embed files from as binary data symbols
# Use COMPONENT_EMBED_TXTFILES instead of COMPONENT_EMBED_FILES
# So it will be null-terminated
COMPONENT_EMBED_TXTFILES := \
	src/oidc.fbs \
	src/oidc.bfbs

# Depends on oidc.fbs
src/id_token_protected_endpoint.o: \
	$(COMPONENT_PATH)/src/oidc_generated.h \
	$(COMPONENT_PATH)/src/oidc.bfbs

# Output these files to the component path
$(COMPONENT_PATH)/src/%_generated.h: $(COMPONENT_PATH)/src/%.fbs
	flatc --cpp -o $(COMPONENT_PATH)/src \
		--gen-mutable \
		--gen-object-api \
		--gen-name-strings \
		--reflect-types \
		--reflect-names \
		$^

# Output these files to the component path
$(COMPONENT_PATH)/src/%.bfbs: $(COMPONENT_PATH)/src/%.fbs
	flatc --binary --schema -o $(COMPONENT_PATH)/src $^

flatbuffers/src/idl_parser.o: CXXFLAGS += -Wno-maybe-uninitialized -Wno-type-limits
