CC=gcc
CXX=g++
CFLAGS=-g -Wall -Wpedantic -std=c99 -DUNICODE -D_UNICODE -DWIN32_LEAN_AND_MEAN
CXXFLAGS=-g -Wall -Wpedantic -std=c++23 -DUNICODE -D_UNICODE -DWIN32_LEAN_AND_MEAN
LDFLAGS=-mconsole
OUTDIR=build
CSOURCES=$(patsubst %,src/%,main.c buffer.c)
CXXSOURCES=$(patsubst %,src/%,direct2d.cpp)
SOURCES=$(CSOURCES) $(CXXSOURCES)
OBJECTS=$(patsubst src/%.c,$(OUTDIR)/%.o,$(CSOURCES)) $(patsubst src/%.cpp,$(OUTDIR)/%.o,$(CXXSOURCES))
DEPFILES=$(patsubst %.o,%.d,$(OBJECTS))

all: $(OUTDIR)/TheEditor.exe

clean:
	del /s /q $(OUTDIR)\*

-include $(DEPFILES)

$(OUTDIR)/TheEditor.exe: $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^ -ld2d1 -ldwrite

$(OUTDIR)/%.o: src/%.c
	$(CC) -c -o $@ $(CFLAGS) $<
	$(CC) -c $(CFLAGS) $< -MM -MF $(@:.o=.d)

$(OUTDIR)/%.o: src/%.cpp
	$(CXX) -c -o $@ $(CXXFLAGS) $<
	$(CXX) -c $(CXXFLAGS) $< -MM -MF $(@:.o=.d)
