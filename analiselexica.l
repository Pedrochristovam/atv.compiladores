%{
/*
 * ============================================================================
 * LEGENDA (APRESENTAÇÃO) — Análise Léxica (especificação Flex)
 * ============================================================================
 * Este arquivo .l documenta as regras oficiais do alfabeto/tokens.
 * Na prática o projeto usa lexico.c (léxico em C puro), para não depender
 * do Flex instalado. As regras abaixo devem permanecer equivalentes.
 *
 * O léxico só classifica lexemas; NÃO monta árvore nem gera código.
 * ============================================================================
 */
#include <stdio.h>
#include "tokens.h"
char* yytext;
%}

%option noyywrap
%x COMMENT

DIGIT       [0-9]+
FLOAT       [0-9]+\.[0-9]+
LETTER      [a-zA-Z]
ID          {LETTER}({LETTER}|{DIGIT}|_)*
WHITESPACE  [ \t\r\n]
STRING      \"([^\"\n])*\"
CHAR        \'([^\'\n])\'

%%

"if"            { return IF; }
"else"          { return ELSE; }
"for"           { return FOR; }
"while"         { return WHILE; }   /* CORREÇÃO: while NÃO pode retornar FOR */
"switch"        { return SWITCH; }
"case"          { return CASE; }
"default"       { return DEFAULT; }
"break"         { return BREAK; }
"return"        { return RETURN; }
"var"           { return VAR; }
"func"          { return FUNC; }
"main"          { return MAIN; }
"print"         { return PRINT; }
"input"         { return INPUT; }
"true"|"false"  { return BOOL_LITERAL; }
"int"|"float"|"char"|"bool"|"string" { return TIPO; }

"=="            { return IGUAL; }
"!="            { return DIFERENTE; }
">="            { return MAIOR_IGUAL; }
"<="            { return MENOR_IGUAL; }
"&&"            { return E_LOGICO; }
"||"            { return OU_LOGICO; }
"+"             { return SOMA; }
"-"             { return SUBTRACAO; }
"*"             { return MULTIPLICACAO; }
"/"             { return DIVISAO; }
"="             { return ATRIBUICAO; }
">"             { return MAIOR; }
"<"             { return MENOR; }
"!"             { return NOT; }
"("             { return ABRE_PARENTESE; }
")"             { return FECHA_PARENTESE; }
"{"             { return ABRE_CHAVES; }
"}"             { return FECHA_CHAVES; }
";"             { return PONTO_VIRGULA; }
","             { return VIRGULA; }
":"             { return DOIS_PONTOS; }

{FLOAT}         { return FLOAT_LITERAL; }
{DIGIT}         { return NUMERO; }
{ID}            { return IDENTIFICADOR; }
{STRING}        { return STRING_LITERAL; }
{CHAR}          { return CHAR_LITERAL; }

"/*"                    { BEGIN(COMMENT); }
<COMMENT>"*/"           { BEGIN(INITIAL); }
<COMMENT>\n             { /* ignora */ }
<COMMENT>.              { /* ignora */ }
"//".*                  { /* comentário de linha */ }

{WHITESPACE}            { /* ignora */ }
.                       { return ERRO; }
%%
