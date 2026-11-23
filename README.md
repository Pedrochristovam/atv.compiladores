# Compilador MACSLang

Compilador da linguagem **MACSLang** em C, com as 4 fases do A3:

1. Análise léxica (`lexico.c`)
2. Análise sintática (`parser.c` → AST)
3. Análise semântica (`semantica.c`)
4. Geração de código x86-64 Windows (`codegen.c` → `.s`)

Documentação completa para apresentação: **[DOCUMENTACAO.md](DOCUMENTACAO.md)**

## Compilar o compilador

```powershell
$env:Path = "C:\msys64\mingw64\bin;" + $env:Path
gcc -Wall -std=c11 main.c lexico.c ast.c semantica.c parser.c codegen.c -o compilador.exe
```

## Compilar um programa MACSLang

```powershell
.\compilador.exe testes\soma.macs -o saida.s --ast
gcc saida.s -o programa.exe
.\programa.exe
```

## Testes

Veja a pasta `testes/` (`soma.macs`, `while_contagem.macs`, `for_contagem.macs`, ...).
