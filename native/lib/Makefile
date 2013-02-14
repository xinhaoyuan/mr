.PHONY: all 

T_CC_FLAGS       ?= ${T_CC_FLAGS_OPT} -Wall -I include -D__MR_INTERNAL__
T_CC_OPT_FLAGS   ?= -O2
T_CC_DEBUG_FLAGS ?= 

SRCFILES:= $(shell find src '(' '!' -regex '.*/_.*' ')' -and '(' -iname "*.c" ')' | sed -e 's!^\./!!g')

include ${T_BASE}/utl/template.mk

all: ${T_OBJ}/libmr.a

-include ${DEPFILES}

${T_OBJ}/libmr.a: ${OBJFILES}
	@echo AR $@
	${V}ar r $@ $^
