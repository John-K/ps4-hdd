include Common.mak

# Building for MacOS is enough of a snowflakce
# that is has its own makefile
ifeq ($(shell uname -s), Darwin)
$(error Please run make -f Makefile.MacOS $@)
endif

CC=gcc
CXX=g++
STRIP=strip
# NEON is untested
#AES_FLAGS = -D USE_NEON_AES
#AES_FLAGS = -D USE_CXX_AES
CPPFLAGS=$(BASE_CPPFLAGS) -D USE_INTEL_AESNI -maes
OBJDIR=build

#$(TARGET): $(info $(shell $(CXX) --version))
$(TARGET): $(addprefix $(OBJDIR)/, $(OBJS))
	$(CXX) $(CPPFLAGS) $^ -o $@
	$(STRIP) --strip-all $@

$(OBJDIR):
	@mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: src/%.cpp | $(OBJDIR)
	$(CXX) $(CPPFLAGS) -c -o $@ $<

all: $(TARGET)

.PHONY: clean
clean:
	rm -f $(addprefix $(OBJDIR)/, $(OBJS)) $(TARGET)
