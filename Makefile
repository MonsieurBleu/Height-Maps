ifeq ($(OS),Windows_NT)
	GEXEC = Game.exe
else
	GEXEC = Game
endif

MAKE_FLAGS = --no-print-directory
MAKE_PARALLEL = -j -k

default : build-debug

run :
ifeq ($(OS),Windows_NT)
	cd build && $(GEXEC)
else
	cd build && ./$(GEXEC)
endif

run-debug :
	@cd build && gdb ./$(GEXEC)

build-debug : 
	@$(MAKE) -C ./Engine game $(MAKE_FLAGS) $(MAKE_PARALLEL) OPTFLAGS="-g"

build-debug-linear : 
	@$(MAKE) -C ./Engine game $(MAKE_FLAGS) OPTFLAGS="-g"

build-debug-fast :
	@$(MAKE) -C ./Engine game $(MAKE_FLAGS) OPTFLAGS="-g -Ofast"

build-release : 
	@$(MAKE) -C ./Engine game $(MAKE_FLAGS) $(MAKE_PARALLEL) OPTFLAGS="-Ofast"

clean : 
	@$(MAKE) -C ./Engine clean $(MAKE_FLAGS)
