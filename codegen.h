/*
 * ============================================================================
 * LEGENDA (APRESENTAÇÃO) — Gerador de Código (4ª fase)
 * ============================================================================
 * Percorre a AST e emite assembly x86-64 (sintaxe Intel / GAS)
 * compatível com GCC no Windows (MinGW).
 *
 * Alvo do enunciado: arquitetura X86 + SO Windows.
 * Saída típica: saida.s  →  gcc saida.s -o programa.exe
 * ============================================================================
 */
#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"
#include <stdio.h>

/* Gera assembly no arquivo out (ex.: saida.s) */
void gerar_codigo(NoArvore* raiz, FILE* out);

#endif
