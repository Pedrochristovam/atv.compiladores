# ============================================================================
# LEGENDA (APRESENTAÇÃO) — Makefile
# Compila os 4 módulos do compilador MACSLang e facilita os testes.
# ============================================================================

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O0 -g
SRC = main.c lexico.c ast.c semantica.c parser.c codegen.c
OBJ = $(SRC:.c=.o)
BIN = compilador.exe

.PHONY: all clean test run-soma

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(BIN)

clean:
	del /Q $(OBJ) $(BIN) saida.s programa.exe 2>NUL || exit 0

# Exemplo: compilar um programa MACSLang até o .exe
# make run-soma
run-soma: $(BIN)
	./$(BIN) testes/soma.macs -o saida.s --ast
	$(CC) saida.s -o programa.exe
	./programa.exe

test: $(BIN)
	./$(BIN) testes/soma.macs -o saida.s
	./$(BIN) testes/while_contagem.macs -o saida.s
	./$(BIN) testes/maior.macs -o saida.s
	@echo Testes de compilacao (AST/semantica/codegen) OK
