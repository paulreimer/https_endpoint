CXXFLAGS := \
	-std=c++14 \
	-I../cpp17_headers/include \
	-I../delegate \
	-I. \
	-D'ESP_LOGE(tag, format, ...)=printf(format, \#\#__VA_ARGS__)'\
	-D'ESP_LOGW(tag, format, ...)=printf(format, \#\#__VA_ARGS__)'\
	-D'ESP_LOGI(tag, format, ...)=printf(format, \#\#__VA_ARGS__)'\
	-D'ESP_LOGD(tag, format, ...)=printf(format, \#\#__VA_ARGS__)'\
	-D'ESP_LOGV(tag, format, ...)=printf(format, \#\#__VA_ARGS__)'\
	-DMBEDTLS_ERR_SSL_WANT_READ=-0x6900\
	-DMBEDTLS_ERR_SSL_WANT_WRITE=0x6880\
	-DMBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY=-0x7880

TEST_SRCS := \
	https_endpoint_test.cpp \
	uri_parser_test.cpp

TEST_DEPS := \
	../src/uri_parser.cpp

test_runner: $(TEST_SRCS) $(TEST_DEPS) test_runner.cpp
	c++ $(CXXFLAGS) $^ -o $@

.PHONY: test
test: test_runner
	@./test_runner
