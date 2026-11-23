/*
 * ============================================================================
 * LEGENDA (APRESENTAÇÃO) — Funções da AST
 * ============================================================================
 * criarNo / adicionarFilho / imprimirArvore / liberarArvore
 * Estrutura compartilhada por parser, semântica e gerador de código.
 * ============================================================================
 */
#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

NoArvore* criarNo(const char* tipo, const char* valor) {
    NoArvore* no = (NoArvore*)malloc(sizeof(NoArvore));
    if (!no) {
        fprintf(stderr, "Erro: sem memoria para no da AST\n");
        exit(1);
    }
    strncpy(no->tipo, tipo, sizeof(no->tipo) - 1);
    no->tipo[sizeof(no->tipo) - 1] = '\0';
    strncpy(no->valor, valor ? valor : "", sizeof(no->valor) - 1);
    no->valor[sizeof(no->valor) - 1] = '\0';
    no->tipoVariavel[0] = '\0';
    no->offset = 0;
    no->numArgs = 0;
    no->filho = NULL;
    no->proximo = NULL;
    return no;
}

void adicionarFilho(NoArvore* pai, NoArvore* filho) {
    if (!pai || !filho) return;
    if (pai->filho == NULL) {
        pai->filho = filho;
        return;
    }
    NoArvore* temp = pai->filho;
    while (temp->proximo != NULL)
        temp = temp->proximo;
    temp->proximo = filho;
}

void imprimirArvore(NoArvore* no, int nivel) {
    if (!no) return;
    for (int i = 0; i < nivel; i++) printf("  ");
    printf("%s", no->tipo);
    if (no->valor[0] != '\0')
        printf(": %s", no->valor);
    if (no->tipoVariavel[0] != '\0')
        printf(" [tipo: %s]", no->tipoVariavel);
    printf("\n");
    for (NoArvore* f = no->filho; f; f = f->proximo)
        imprimirArvore(f, nivel + 1);
}

void liberarArvore(NoArvore* raiz) {
    if (!raiz) return;
    liberarArvore(raiz->filho);
    liberarArvore(raiz->proximo);
    free(raiz);
}
