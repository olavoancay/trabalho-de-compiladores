# README

Componentes:

- `lexer.h`: Arquivo de cabeçalho com as definições de Tokens.
- `lexer.c`: Código-fonte principal do Lexer, Tabela de Símbolos e Main.
- `teste_correto.lsi`: Um programa de exemplo válido na linguagem.
- `teste_incorreto1.lsi`: Um programa de exemplo com um erro léxico (caractere '@').
- `teste_incorreto2.lsi`: Um programa de exemplo com um erro léxico (caractere '!').

Compilação:

O programa foi escrito em C (compatível com GCC 13.3.0). Para compilar, use o comando:

gcc -o lexer lexer.c -std=gnu99

Execução:

1. Teste com Arquivo Correto

Execute o comando:

./lexer teste_correto.lsi

Saída Esperada:

O programa irá listar todos os tokens reconhecidos e imprimirá o conteúdo da Tabela de Símbolos.

2. Teste com Arquivo Incorreto (caractere '@')

Execute o comando:

./lexer teste_incorreto1.lsi

Saída Esperada:

O programa irá parar no primeiro erro léxico e reportar uma mensagem indicando a linha e a coluna do erro.

3. Teste com Arquivo Incorreto (caractere '!')

Execute o comando:

./lexer teste_incorreto2.lsi

Saída Esperada:

O programa irá parar e reportar o erro no caractere '!' na linha 6.