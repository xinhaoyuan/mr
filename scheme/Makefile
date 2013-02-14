.PHONY: repl unit-test-vm unit-test-compiler clean

SCHEME ?= racket -f
# SCHEME ?= scm -l

struct.def.ss: struct.def
	${SCHEME} struct.ss < $< > $@

unit-test-vm:
	${SCHEME} boot-vm.ss -e "(vm:unit-test)"

unit-test-compiler: struct.def.ss
	${SCHEME} compiler.ss -e "(shell:unit-test)"

repl: struct.def.ss
	${SCHEME} compiler.ss -e "(shell:repl)"

clean: 
	-rm struct.def.ss
