# README

Componentes:

- `lexer.h`: Arquivo de cabeçalho com as definições de Tokens.
- `lexer.c`: Código-fonte principal do Analisador Léxico, Tabela de Símbolos e tokenização.
- `parser.c`: Código-fonte do Analisador Sintático Preditivo com função main.
- `teste_correto_50linhas.lsi`: Um programa de exemplo válido na linguagem (65 linhas).
- `teste_sintatico_erro1.lsi`: Um programa de exemplo com erro sintático (falta de ponto-e-vírgula).
- `teste_sintatico_erro2.lsi`: Um programa de exemplo com erro sintático (parêntese fechado faltando).
- `teste_sintatico_erro3.lsi`: Um programa de exemplo com erro sintático (expressão malformada).

Abordagem de Implementação:

O analisador sintático foi implementado usando a técnica de Análise Preditiva
Guiada por Tabela (Table-Driven Predictive Parsing), conforme especificado nos
requisitos do trabalho. Características da implementação:

- Tabela de Reconhecimento Sintático LL(1): Tabela bidimensional que mapeia
  combinações (não-terminal, terminal) para regras de produção

- Pilha Explícita: Gerencia símbolos (terminais e não-terminais) durante a
  análise sintática

- Gramática LL(1): A gramática LSI-2025-2 foi transformada para LL(1) através
  de eliminação de recursão à esquerda e fatoração à esquerda

- Algoritmo de Parsing: Consulta a tabela parse_table[não-terminal][terminal]
  para determinar qual regra de produção aplicar em cada passo

Esta abordagem é mais sistemática e formal que a descida recursiva, sendo
facilmente demonstrável e verificável através da tabela de reconhecimento.

Compilação:

O programa foi escrito em C (compatível com GCC 13.3.0). Para compilar, use o comando:

gcc -o parser parser.c lexer.c -std=gnu99 -Wall

Execução:

1. Teste com Arquivo Correto

Execute o comando:

./parser teste_correto_50linhas.lsi

Saída Esperada:

O programa irá validar a sintaxe do arquivo e imprimir a mensagem de sucesso.

2. Teste com Arquivo Incorreto (Falta Semicolon)

Execute o comando:

./parser teste_sintatico_erro1.lsi

Saída Esperada:

O programa irá parar no primeiro erro sintático e reportar uma mensagem indicando que esperava um TOKEN_SEMICOLON na linha 6, coluna 5.

3. Teste com Arquivo Incorreto (Parêntese Faltando)

Execute o comando:

./parser teste_sintatico_erro2.lsi

Saída Esperada:

O programa irá parar e reportar um erro sintático indicando que esperava um TOKEN_RPAREN na linha 8, coluna 15.

4. Teste com Arquivo Incorreto (Expressão Malformada)

Execute o comando:

./parser teste_sintatico_erro3.lsi

Saída Esperada:

O programa irá parar e reportar um erro sintático indicando que esperava um operando após um operador na linha 12.
