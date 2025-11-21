# README

Componentes:

lexer.h: Arquivo de cabeçalho com as definições de Tokens.
lexer.c: Código-fonte do Analisador Léxico (sem a função main).
parser.c: Código-fonte do Analisador Sintático (com a função main).
teste_correto.lsi: (Teste do Lexer) Programa de exemplo válido.
teste_incorreto1.lsi: (Teste do Lexer) Erro léxico (caractere '@').
teste_incorreto2.lsi: (Teste do Lexer) Erro léxico (caractere '!').
teste_correto_50linhas.lsi: (Teste do Parser) Programa com 50+ linhas sintaticamente correto.
teste_sintatico_erro1.lsi: (Teste do Parser) Erro sintático (falta ';').
teste_sintatico_erro2.lsi: (Teste do Parser) Erro sintático (falta ')').
teste_sintatico_erro3.lsi: (Teste do Parser) Erro sintático (expressão mal formada).

Compilação:

O programa foi escrito em C (compatível com GCC 13.3.0). Para compilar o parser integrado com o lexer, use o seguinte comando:

gcc -o parser parser.c lexer.c -std=gnu99

Execução:

O programa lê o nome de um arquivo de entrada como argumento de linha de comando.

1. Teste Correto (Parser)

Execute o comando:

./parser teste_correto_50linhas.lsi

Saída Esperada:

O programa irá executar sem imprimir erros e, ao final, exibirá a mensagem: "Análise Sintática concluída com sucesso."

2. Testes de Erros Sintáticos (Parser)

Execute o comando para cada arquivo de erro:

./parser teste_sintatico_erro1.lsi

Saída Esperada:

Um erro sintático reportando que esperava ';' mas encontrou 'y' (ou similar).

./parser teste_sintatico_erro2.lsi

Saída Esperada:

Um erro sintático reportando que esperava ')' mas encontrou '{'.

./parser teste_sintatico_erro3.lsi

Saída Esperada:

Um erro sintático reportando um token '*' inesperado.