LIBEXEC_TARGETS :=

ifeq ($(MULTICALL),1)

BIN_TARGETS := $(package)
BIN_SYMLINKS := $(notdir $(wildcard src/$(package)/deps-exe/*))
EXTRA_TARGETS += src/multicall/$(package).c

define symlink_definition
SYMLINK_TARGET_$(1) := $(package)
endef
$(foreach name,$(BIN_SYMLINKS),$(eval $(call symlink_definition,$(name))))

src/multicall/$(package).c: tools/gen-multicall.sh $(BIN_SYMLINKS:%=src/$(package)/%.c)
	./tools/gen-multicall.sh $(package) > src/multicall/$(package).c

src/multicall/$(package).o: src/multicall/$(package).c src/include/$(package)/config.h

else

BIN_TARGETS := $(notdir $(wildcard src/$(package)/deps-exe/*))
BIN_SYMLINKS :=

endif
