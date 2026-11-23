/*
 * ============================================================================
 * LEGENDA (APRESENTAÇÃO) — main.c (orquestrador das 4 fases)
 * ============================================================================
 * Pipeline do compilador MACSLang:
 *
 *   [1] Léxico     lexico.c      → tokens
 *   [2] Sintático  parser.c      → AST
 *   [3] Semântico  semantica.c   → valida escopos/tipos
 *   [4] Codegen    codegen.c     → assembly x86-64 (.s)
 *
 * Uso:
 *   compilador arquivo.macs
 *   compilador arquivo.macs -o saida.s
 *   compilador arquivo.macs --ast          (só mostra a árvore)
 * ============================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexico.h"
#include "parser.h"
#include "semantica.h"
#include "codegen.h"
#include "ast.h"

static void uso(const char* argv0) {
    fprintf(stderr,
        "Uso: %s <arquivo.macs> [-o saida.s] [--ast] [--no-codegen]\n"
        "  --ast         imprime a AST\n"
        "  --no-codegen  nao gera assembly\n"
        "  -o arquivo    nome do .s de saida (padrao: saida.s)\n",
        argv0);
}

int main(int argc, char** argv) {
    const char* entrada = NULL;
    const char* saida = "saida.s";
    int mostrar_ast = 0;
    int fazer_codegen = 1;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--ast") == 0) mostrar_ast = 1;
        else if (strcmp(argv[i], "--no-codegen") == 0) fazer_codegen = 0;
        else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) saida = argv[++i];
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            uso(argv[0]);
            return 0;
        } else if (argv[i][0] != '-') {
            entrada = argv[i];
        } else {
            fprintf(stderr, "Opcao desconhecida: %s\n", argv[i]);
            uso(argv[0]);
            return 1;
        }
    }

    FILE* fin = stdin;
    if (entrada) {
        fin = fopen(entrada, "r");
        if (!fin) {
            perror(entrada);
            return 1;
        }
    } else {
        fprintf(stderr, "Lendo programa da entrada padrao (stdin)...\n");
    }

    /* FASE 1 — Léxico (preparação do fluxo de caracteres) */
    lexico_iniciar(fin);

    /* FASE 2 — Sintático */
    NoArvore* raiz = parse_programa();

    /* FASE 3 — Semântico */
    analisar_semantica(raiz);

    if (mostrar_ast) {
        printf("\n========== AST ==========\n");
        imprimirArvore(raiz, 0);
        printf("=========================\n\n");
    }

    /* FASE 4 — Geração de Código x86-64 */
    if (fazer_codegen) {
        FILE* fout = fopen(saida, "w");
        if (!fout) {
            perror(saida);
            liberarArvore(raiz);
            if (fin != stdin) fclose(fin);
            return 1;
        }
        gerar_codigo(raiz, fout);
        fclose(fout);
        fprintf(stderr, "Assembly gerado em: %s\n", saida);
        fprintf(stderr, "Para gerar o .exe:  gcc %s -o programa.exe\n", saida);
    }

    liberarArvore(raiz);
    if (fin != stdin) fclose(fin);
    return 0;
}
