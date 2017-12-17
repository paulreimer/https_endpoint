out/Default: .gn BUILD.gn build/BUILDCONFIG.gn build/toolchain/BUILD.gn
	gn gen out/Default

.PHONY: test_runner
test_runner: out/Default
	ninja -C out/Default test_runner
	cp out/Default/test_runner .

.PHONY: test
test: test_runner
	@./test_runner
