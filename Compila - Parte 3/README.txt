============================================================================
TRABALHO DE COMPILADORES - PARTE 3: ANALISADOR SINTÁTICO PREDITIVO
============================================================================

INTEGRANTES DO GRUPO:
- Eduardo Boçon
- Darnley Ribeiro
- Jiliard Peifer
- Olavo Ançay

----------------------------------------------------------------------------
DESCRIÇÃO
----------------------------------------------------------------------------
Esta entrega contém a implementação de um Analisador Sintático Preditivo 
(Top-Down Tabular) para a linguagem LSI-2025-2, integrado com o Analisador 
Léxico desenvolvido na Parte 1.

O analisador verifica se o código fonte fornecido pertence à linguagem gerada 
pela gramática LSI-2025-2 (adaptada para LL(1) conforme a Parte 2).

----------------------------------------------------------------------------
ARQUIVOS INCLUÍDOS
----------------------------------------------------------------------------
- analisador.c             : Código fonte principal do Analisador Sintático.
- analisador.h             : Cabeçalho com definições e protótipos do Analisador Sintático.
- lexer.c                  : Implementação do Analisador Léxico.
- lexer.h                  : Cabeçalho com definições de tokens e funções do lexer.
- teste_correto_50linhas.lsi : Programa válido na linguagem LSI-2025-2 (>50 linhas).
- teste_sintatico_erro1.lsi  : Teste contendo erro sintático (Ex: falta de ';').
- teste_sintatico_erro2.lsi  : Teste contendo erro sintático (Ex: estrutura if incorreta).
- teste_sintatico_erro3.lsi  : Teste contendo erro sintático (Ex: definição de função inválida).
- README.txt               : Este arquivo.

----------------------------------------------------------------------------
COMO COMPILAR (LINUX/GCC)
----------------------------------------------------------------------------
Para compilar o projeto, utilize o compilador GCC. Certifique-se de estar no 
diretório "Compila - Parte 3".

Comando:
    gcc -o analisador analisador.c lexer.c

Isso gerará um executável chamado "analisador".

----------------------------------------------------------------------------
COMO EXECUTAR
----------------------------------------------------------------------------
A execução requer passar o caminho do arquivo de código fonte (.lsi) como 
argumento.

Sintaxe:
    ./analisador <caminho_do_arquivo>

Exemplos de execução com os arquivos fornecidos:

1. Teste Correto (Sucesso esperado):
    ./analisador teste_correto_50linhas.lsi

2. Testes com Erros (Mensagens de erro esperadas):
    ./analisador teste_sintatico_erro1.lsi
    ./analisador teste_sintatico_erro2.lsi
    ./analisador teste_sintatico_erro3.lsi

----------------------------------------------------------------------------
SAÍDA ESPERADA
----------------------------------------------------------------------------
- Em caso de SUCESSO: O programa exibirá a pilha de análise passo a passo e 
  finalizará com a mensagem: 
  "SUCESSO: Arquivo reconhecido pela gramática LSI-2025-2."

- Em caso de ERRO: O programa encerrará a execução indicando a linha, coluna 
  e o tipo de erro (Léxico ou Sintático) encontrado.
