/*
 * ============================================================================
 * LEGENDA (APRESENTAÇÃO) — codegen.c
 * ============================================================================
 * Estratégia de geração:
 *  1) Cada função/main ganha um label assembly
 *  2) Variáveis locais ficam na pilha: [rbp - offset]
 *  3) Expressões deixam o resultado em RAX
 *  4) Condições: 0 = falso, ≠0 = verdadeiro
 *  5) print/input usam printf/scanf da libc (MinGW)
 *  6) Convenção Windows x64: RCX, RDX, R8, R9 + shadow space 32 bytes
 *
 * Isso NÃO é interpretador: o produto final é código de máquina via GCC.
 * ============================================================================
 */
#include "codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LOCAIS 128

typedef struct {
    char nome[100];
    char tipo[20];
    int  offset; /* rbp - offset */
} Local;

static FILE* out;
static Local locais[MAX_LOCAIS];
static int nlocais;
static int proximo_offset;
static int rotulo_seq;
static int strings_seq;

static int novo_rotulo(void) { return ++rotulo_seq; }

static void emit(const char* s) { fprintf(out, "%s\n", s); }

/* --- tabela de locais da função atual --- */
static void limpar_locais(void) {
    nlocais = 0;
    proximo_offset = 0;
}

static int tamanho_tipo(const char* t) {
    (void)t;
    return 8; /* simplificação: tudo em 8 bytes (x64) */
}

static void declarar_local(const char* nome, const char* tipo) {
    if (nlocais >= MAX_LOCAIS) {
        fprintf(stderr, "Codegen: muitas variaveis locais\n");
        exit(1);
    }
    proximo_offset += tamanho_tipo(tipo);
    strncpy(locais[nlocais].nome, nome, 99);
    strncpy(locais[nlocais].tipo, tipo ? tipo : "int", 19);
    locais[nlocais].offset = proximo_offset;
    nlocais++;
}

static Local* buscar_local(const char* nome) {
    for (int i = nlocais - 1; i >= 0; i--)
        if (strcmp(locais[i].nome, nome) == 0) return &locais[i];
    return NULL;
}

static int alinhar16(int n) {
    return (n + 15) & ~15;
}

/* forward */
static void gen_expr(NoArvore* n);
static void gen_stmt(NoArvore* n);
static void coletar_locais_bloco(NoArvore* bloco);

static void coletar_locais_bloco(NoArvore* bloco) {
    if (!bloco) return;
    for (NoArvore* c = bloco->filho; c; c = c->proximo) {
        if (strcmp(c->tipo, "DECLARACAO") == 0) {
            NoArvore* id = c->filho;
            NoArvore* tipo = id ? id->proximo : NULL;
            if (id && tipo) declarar_local(id->valor, tipo->valor);
        }
    }
}

static void gen_endereco_var(const char* nome) {
    Local* L = buscar_local(nome);
    if (!L) {
        fprintf(stderr, "Codegen: variavel '%s' sem offset\n", nome);
        exit(1);
    }
    fprintf(out, "    lea rax, [rbp-%d]\n", L->offset);
}

static void gen_load_var(const char* nome) {
    Local* L = buscar_local(nome);
    if (!L) {
        fprintf(stderr, "Codegen: variavel '%s' nao encontrada\n", nome);
        exit(1);
    }
    fprintf(out, "    mov rax, qword ptr [rbp-%d]\n", L->offset);
}

static void gen_store_var(const char* nome) {
    Local* L = buscar_local(nome);
    if (!L) {
        fprintf(stderr, "Codegen: store em '%s' invalido\n", nome);
        exit(1);
    }
    fprintf(out, "    mov qword ptr [rbp-%d], rax\n", L->offset);
}

/* coloca args em RCX/RDX/R8/R9 conforme Windows x64 */
static void gen_call_setup_args(NoArvore* argsNode) {
    NoArvore* args[4] = {0};
    int n = 0;
    if (argsNode) {
        for (NoArvore* a = argsNode->filho; a && n < 4; a = a->proximo)
            args[n++] = a;
    }
    /* avalia da direita p/ esquerda, empilha temporários */
    for (int i = n - 1; i >= 0; i--) {
        gen_expr(args[i]);
        emit("    push rax");
    }
    if (n > 0) { emit("    pop rcx"); }
    if (n > 1) { emit("    pop rdx"); }
    if (n > 2) { emit("    pop r8"); }
    if (n > 3) { emit("    pop r9"); }
}

static void gen_expr(NoArvore* n) {
    if (!n) {
        emit("    xor rax, rax");
        return;
    }

    if (strcmp(n->tipo, "NUMERO") == 0) {
        fprintf(out, "    mov rax, %s\n", n->valor);
        return;
    }
    if (strcmp(n->tipo, "FLOAT_LITERAL") == 0) {
        /* simplificação acadêmica: trunca para int no codegen */
        long v = (long)atof(n->valor);
        fprintf(out, "    mov rax, %ld\n", v);
        return;
    }
    if (strcmp(n->tipo, "BOOL_LITERAL") == 0) {
        fprintf(out, "    mov rax, %d\n", strcmp(n->valor, "true") == 0 ? 1 : 0);
        return;
    }
    if (strcmp(n->tipo, "CHAR_LITERAL") == 0) {
        char ch = (strlen(n->valor) >= 2) ? n->valor[1] : 0;
        fprintf(out, "    mov rax, %d\n", (int)ch);
        return;
    }
    if (strcmp(n->tipo, "STRING_LITERAL") == 0) {
        /* Codegen focado em int: string como ponteiro nao e emitida neste MVP */
        (void)strings_seq;
        fprintf(stderr, "Aviso codegen: string literal nao suportada na emissao; usando 0\n");
        emit("    xor rax, rax");
        return;
    }
    if (strcmp(n->tipo, "IDENTIFICADOR") == 0) {
        gen_load_var(n->valor);
        return;
    }
    if (strcmp(n->tipo, "NEGATIVO") == 0) {
        gen_expr(n->filho);
        emit("    neg rax");
        return;
    }
    if (strcmp(n->tipo, "NOT") == 0) {
        gen_expr(n->filho);
        emit("    cmp rax, 0");
        emit("    sete al");
        emit("    movzx rax, al");
        return;
    }
    if (strcmp(n->tipo, "CHAMADA_FUNCAO") == 0) {
        NoArvore* nome = n->filho;
        NoArvore* args = nome ? nome->proximo : NULL;
        gen_call_setup_args(args);
        emit("    sub rsp, 32"); /* shadow space */
        fprintf(out, "    call func_%s\n", nome->valor);
        emit("    add rsp, 32");
        return;
    }

    /* binários */
    NoArvore* esq = n->filho;
    NoArvore* dir = esq ? esq->proximo : NULL;

    if (strcmp(n->tipo, "E_LOGICO") == 0) {
        int L_fim = novo_rotulo();
        int L_false = novo_rotulo();
        gen_expr(esq);
        emit("    cmp rax, 0");
        fprintf(out, "    je .L%d\n", L_false);
        gen_expr(dir);
        emit("    cmp rax, 0");
        emit("    setne al");
        emit("    movzx rax, al");
        fprintf(out, "    jmp .L%d\n", L_fim);
        fprintf(out, ".L%d:\n", L_false);
        emit("    xor rax, rax");
        fprintf(out, ".L%d:\n", L_fim);
        return;
    }
    if (strcmp(n->tipo, "OU_LOGICO") == 0) {
        int L_fim = novo_rotulo();
        int L_true = novo_rotulo();
        gen_expr(esq);
        emit("    cmp rax, 0");
        fprintf(out, "    jne .L%d\n", L_true);
        gen_expr(dir);
        emit("    cmp rax, 0");
        emit("    setne al");
        emit("    movzx rax, al");
        fprintf(out, "    jmp .L%d\n", L_fim);
        fprintf(out, ".L%d:\n", L_true);
        emit("    mov rax, 1");
        fprintf(out, ".L%d:\n", L_fim);
        return;
    }

    /* demais binários: avalia esq, push, avalia dir, pop rcx, opera */
    gen_expr(esq);
    emit("    push rax");
    gen_expr(dir);
    emit("    mov rcx, rax");
    emit("    pop rax");

    if (strcmp(n->tipo, "SOMA") == 0) {
        emit("    add rax, rcx");
    } else if (strcmp(n->tipo, "SUBTRACAO") == 0) {
        emit("    sub rax, rcx");
    } else if (strcmp(n->tipo, "MULTIPLICACAO") == 0) {
        emit("    imul rax, rcx");
    } else if (strcmp(n->tipo, "DIVISAO") == 0) {
        emit("    cqo");
        emit("    idiv rcx");
    } else if (strcmp(n->tipo, "IGUAL") == 0) {
        emit("    cmp rax, rcx");
        emit("    sete al");
        emit("    movzx rax, al");
    } else if (strcmp(n->tipo, "DIFERENTE") == 0) {
        emit("    cmp rax, rcx");
        emit("    setne al");
        emit("    movzx rax, al");
    } else if (strcmp(n->tipo, "MAIOR") == 0) {
        emit("    cmp rax, rcx");
        emit("    setg al");
        emit("    movzx rax, al");
    } else if (strcmp(n->tipo, "MENOR") == 0) {
        emit("    cmp rax, rcx");
        emit("    setl al");
        emit("    movzx rax, al");
    } else if (strcmp(n->tipo, "MAIOR_IGUAL") == 0) {
        emit("    cmp rax, rcx");
        emit("    setge al");
        emit("    movzx rax, al");
    } else if (strcmp(n->tipo, "MENOR_IGUAL") == 0) {
        emit("    cmp rax, rcx");
        emit("    setle al");
        emit("    movzx rax, al");
    } else {
        fprintf(stderr, "Codegen: operador desconhecido '%s'\n", n->tipo);
        exit(1);
    }
}

static void gen_atribuicao(NoArvore* n) {
    NoArvore* id = n->filho;
    NoArvore* expr = id ? id->proximo : NULL;
    gen_expr(expr);
    gen_store_var(id->valor);
}

static void gen_stmt(NoArvore* n) {
    if (!n) return;

    if (strcmp(n->tipo, "DECLARACAO") == 0) {
        NoArvore* id = n->filho;
        NoArvore* tipo = id ? id->proximo : NULL;
        NoArvore* init = tipo ? tipo->proximo : NULL;
        if (init) {
            gen_expr(init);
            gen_store_var(id->valor);
        } else {
            emit("    xor rax, rax");
            gen_store_var(id->valor);
        }
        return;
    }
    if (strcmp(n->tipo, "ATRIBUICAO") == 0) {
        gen_atribuicao(n);
        return;
    }
    if (strcmp(n->tipo, "PRINT") == 0) {
        gen_expr(n->filho);
        emit("    mov rdx, rax");
        emit("    lea rcx, [rip+fmt_int]");
        emit("    sub rsp, 32");
        emit("    call printf");
        emit("    add rsp, 32");
        return;
    }
    if (strcmp(n->tipo, "INPUT") == 0) {
        NoArvore* id = n->filho;
        gen_endereco_var(id->valor);
        emit("    mov rdx, rax");
        emit("    lea rcx, [rip+fmt_scan]");
        emit("    sub rsp, 32");
        emit("    call scanf");
        emit("    add rsp, 32");
        return;
    }
    if (strcmp(n->tipo, "RETURN") == 0) {
        gen_expr(n->filho);
        emit("    leave");
        emit("    ret");
        return;
    }
    if (strcmp(n->tipo, "CHAMADA_FUNCAO") == 0) {
        gen_expr(n); /* resultado descartado se statement */
        return;
    }
    if (strcmp(n->tipo, "IF") == 0) {
        NoArvore* cond = n->filho;
        NoArvore* thenB = cond ? cond->proximo : NULL;
        NoArvore* elseB = thenB ? thenB->proximo : NULL;
        int L_else = novo_rotulo();
        int L_fim = novo_rotulo();
        gen_expr(cond);
        emit("    cmp rax, 0");
        fprintf(out, "    je .L%d\n", elseB ? L_else : L_fim);
        if (thenB) {
            for (NoArvore* s = thenB->filho; s; s = s->proximo)
                gen_stmt(s);
        }
        if (elseB) {
            fprintf(out, "    jmp .L%d\n", L_fim);
            fprintf(out, ".L%d:\n", L_else);
            for (NoArvore* s = elseB->filho; s; s = s->proximo)
                gen_stmt(s);
        }
        fprintf(out, ".L%d:\n", L_fim);
        return;
    }
    if (strcmp(n->tipo, "WHILE") == 0) {
        NoArvore* cond = n->filho;
        NoArvore* corpo = cond ? cond->proximo : NULL;
        int L_ini = novo_rotulo();
        int L_fim = novo_rotulo();
        fprintf(out, ".L%d:\n", L_ini);
        gen_expr(cond);
        emit("    cmp rax, 0");
        fprintf(out, "    je .L%d\n", L_fim);
        if (corpo) {
            for (NoArvore* s = corpo->filho; s; s = s->proximo)
                gen_stmt(s);
        }
        fprintf(out, "    jmp .L%d\n", L_ini);
        fprintf(out, ".L%d:\n", L_fim);
        return;
    }
    if (strcmp(n->tipo, "FOR") == 0) {
        NoArvore* init = n->filho;
        NoArvore* cond = init ? init->proximo : NULL;
        NoArvore* incr = cond ? cond->proximo : NULL;
        NoArvore* corpo = incr ? incr->proximo : NULL;
        int L_ini = novo_rotulo();
        int L_fim = novo_rotulo();
        gen_stmt(init);
        fprintf(out, ".L%d:\n", L_ini);
        gen_expr(cond);
        emit("    cmp rax, 0");
        fprintf(out, "    je .L%d\n", L_fim);
        if (corpo) {
            for (NoArvore* s = corpo->filho; s; s = s->proximo)
                gen_stmt(s);
        }
        gen_stmt(incr);
        fprintf(out, "    jmp .L%d\n", L_ini);
        fprintf(out, ".L%d:\n", L_fim);
        return;
    }
    if (strcmp(n->tipo, "BLOCO") == 0) {
        for (NoArvore* s = n->filho; s; s = s->proximo)
            gen_stmt(s);
        return;
    }
}

static void gen_prologue(int frame) {
    emit("    push rbp");
    emit("    mov rbp, rsp");
    if (frame > 0)
        fprintf(out, "    sub rsp, %d\n", frame);
}

static void gen_epilogue_zero(void) {
    emit("    xor rax, rax");
    emit("    leave");
    emit("    ret");
}

static void gen_funcao(NoArvore* n) {
    /* filhos: IDENT, PARAMETROS, TIPO, BLOCO */
    NoArvore* id = n->filho;
    NoArvore* params = id ? id->proximo : NULL;
    NoArvore* tipo = params ? params->proximo : NULL;
    NoArvore* bloco = tipo ? tipo->proximo : NULL;
    (void)tipo;

    limpar_locais();

    /* parâmetros como locais; valores virão dos registradores */
    int nparams = 0;
    char nomesParam[4][100];
    if (params) {
        for (NoArvore* a = params->filho; a; a = a->proximo) {
            if (strcmp(a->tipo, "IDENTIFICADOR") == 0 && a->proximo &&
                strcmp(a->proximo->tipo, "TIPO") == 0) {
                if (nparams < 4) {
                    strncpy(nomesParam[nparams], a->valor, 99);
                    declarar_local(a->valor, a->proximo->valor);
                    nparams++;
                }
                a = a->proximo;
            }
        }
    }
    coletar_locais_bloco(bloco);

    int frame = alinhar16(proximo_offset + 32); /* locais + folga shadow */

    fprintf(out, "\n    .global func_%s\n", id->valor);
    fprintf(out, "func_%s:\n", id->valor);
    gen_prologue(frame);

    /* salva args Windows x64 nos slots */
    if (nparams > 0) fprintf(out, "    mov qword ptr [rbp-%d], rcx\n", buscar_local(nomesParam[0])->offset);
    if (nparams > 1) fprintf(out, "    mov qword ptr [rbp-%d], rdx\n", buscar_local(nomesParam[1])->offset);
    if (nparams > 2) fprintf(out, "    mov qword ptr [rbp-%d], r8\n",  buscar_local(nomesParam[2])->offset);
    if (nparams > 3) fprintf(out, "    mov qword ptr [rbp-%d], r9\n",  buscar_local(nomesParam[3])->offset);

    if (bloco) {
        for (NoArvore* s = bloco->filho; s; s = s->proximo)
            gen_stmt(s);
    }
    gen_epilogue_zero();
}

static void gen_main(NoArvore* n) {
    NoArvore* bloco = n->filho;
    limpar_locais();
    coletar_locais_bloco(bloco);
    int frame = alinhar16(proximo_offset + 32);

    emit("\n    .global main");
    emit("main:");
    gen_prologue(frame);
    if (bloco) {
        for (NoArvore* s = bloco->filho; s; s = s->proximo)
            gen_stmt(s);
    }
    gen_epilogue_zero();
}

void gerar_codigo(NoArvore* raiz, FILE* f) {
    out = f;
    rotulo_seq = 0;
    strings_seq = 0;

    /*
     * Cabeçalho assembly x86-64 Windows (MinGW / GAS Intel syntax)
     */
    emit("/* Gerado pelo Compilador MACSLang — alvo x86-64 Windows */");
    emit("    .intel_syntax noprefix");
    emit("");
    emit("    .extern printf");
    emit("    .extern scanf");
    emit("");
    emit("    .section .rdata,\"dr\"");
    emit("fmt_int:");
    emit("    .ascii \"%d\\n\\0\"");
    emit("fmt_scan:");
    emit("    .ascii \"%d\\0\"");
    emit("");
    emit("    .text");

    if (!raiz) return;
    for (NoArvore* c = raiz->filho; c; c = c->proximo) {
        if (strcmp(c->tipo, "FUNCAO") == 0) gen_funcao(c);
        else if (strcmp(c->tipo, "MAIN") == 0) gen_main(c);
    }

    fprintf(stderr, "Geracao de codigo x86-64: OK\n");
}
