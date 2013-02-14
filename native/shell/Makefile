.PHONY: all 

T_CC_FLAGS       ?= ${T_CC_FLAGS_OPT} -Wall -I ${PATH_mr-lib}/include
T_CC_OPT_FLAGS   ?= -O2
T_CC_DEBUG_FLAGS ?= 

SRCFILES:= $(shell find . '(' '!' -regex '.*/_.*' ')' -and '(' -iname "*.c" -or -iname "*.cpp" ')' | sed -e 's!^\./!!g')

include ${T_BASE}/utl/template.mk

all: ${T_OBJ}/${PRJ}

-include ${DEPFILES}

${T_OBJ}/${PRJ}: ${OBJFILES} ${T_OBJ}/libmr.a
	@echo LINK $@
	${V}${CXX} ${T_CXX_ALL_FLAGS} $^ -o $@
