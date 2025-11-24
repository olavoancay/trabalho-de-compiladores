/*
 * ============================================================================
 * ANALISADOR LÉXICO PARA A LINGUAGEM LSI-2025-2
 * ============================================================================
 * 
 * Integrantes: Eduardo Boçon, Darnley Ribeiro, Jiliard Peifer e Olavo Ançay
 * 
 * Este analisador léxico implementa:
 *   - Leitura caractere por caractere
 *   - Reconhecimento baseado em diagramas de transição (autômatos)
 *   - Tabela de símbolos com técnica "maximal munch"
 *   - Detecção de erros léxicos com linha e coluna
 * 
 * ============================================================================
 */

#define _DEFAULT_SOURCE

#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ============================================================================
 * CONSTANTES
 * ============================================================================ */

#define SYMBOL_TABLE_SIZE 100    /* Tamanho da tabela hash */
#define LEXEME_BUFFER_SIZE 256   /* Tamanho máximo de um lexema */

/* ============================================================================
 * TABELA DE SÍMBOLOS
 * ----------------------------------------------------------------------------
 * Implementada como tabela hash com encadeamento para colisões.
 * 
 * A tabela é inicializada com as palavras-chave da linguagem, permitindo
 * a implementação da técnica "maximal munch": quando um identificador é
 * reconhecido, consultamos a tabela para verificar se é palavra-chave.
 * ============================================================================ */

typedef struct Symbol {
    char* lexeme;           /* Texto do símbolo */
    TokenType type;         /* Tipo do token */
    struct Symbol* next;    /* Próximo símbolo (encadeamento) */
} Symbol;

static Symbol* symbol_table[SYMBOL_TABLE_SIZE];

/*
 * Função hash para strings.
 * Usa o método shift-add-XOR para distribuição uniforme.
 */
static unsigned int hash(const char* str) {
    unsigned int hash_value = 0;
    while (*str) {
        hash_value = (hash_value << 5) + *str++;
    }
    return hash_value % SYMBOL_TABLE_SIZE;
}

/*
 * Inicializa a tabela de símbolos com as palavras-chave.
 * Isso permite distinguir palavras-chave de identificadores comuns.
 */
void symtable_init(void) {
    /* Limpa a tabela */
    for (int i = 0; i < SYMBOL_TABLE_SIZE; i++) {
        symbol_table[i] = NULL;
    }

    /* Palavras-chave da linguagem LSI-2025-2 */
    const char* keywords[] = {"int", "if", "else", "def", "print", "return"};
    TokenType types[] = {TOKEN_INT, TOKEN_IF, TOKEN_ELSE, 
                         TOKEN_DEF, TOKEN_PRINT, TOKEN_RETURN};
    int num_keywords = sizeof(keywords) / sizeof(keywords[0]);

    /* Insere cada palavra-chave na tabela */
    for (int i = 0; i < num_keywords; i++) {
        unsigned int index = hash(keywords[i]);
        Symbol* new_symbol = (Symbol*)malloc(sizeof(Symbol));
        new_symbol->lexeme = strdup(keywords[i]);
        new_symbol->type = types[i];
        new_symbol->next = symbol_table[index];
        symbol_table[index] = new_symbol;
    }
}

/*
 * Busca um lexema na tabela. Se encontrar, retorna o token correspondente.
 * Se não encontrar, insere como identificador (TOKEN_ID).
 * 
 * Esta função implementa a técnica "maximal munch":
 *   1. O lexer reconhece uma sequência de letras/dígitos
 *   2. Consulta a tabela de símbolos
 *   3. Se for palavra-chave (pré-inserida), retorna o tipo correto
 *   4. Caso contrário, é um identificador comum
 */
Token symtable_lookup_insert(const char* lexeme, int line, int col) {
    unsigned int index = hash(lexeme);
    Symbol* current = symbol_table[index];

    /* Busca na lista encadeada */
    while (current != NULL) {
        if (strcmp(current->lexeme, lexeme) == 0) {
            /* Encontrou: retorna token com tipo da tabela */
            return (Token){current->type, current->lexeme, line, col};
        }
        current = current->next;
    }

    /* Não encontrou: insere como identificador */
    Symbol* new_symbol = (Symbol*)malloc(sizeof(Symbol));
    new_symbol->lexeme = strdup(lexeme);
    new_symbol->type = TOKEN_ID;
    new_symbol->next = symbol_table[index];
    symbol_table[index] = new_symbol;

    return (Token){TOKEN_ID, new_symbol->lexeme, line, col};
}

/*
 * Imprime a tabela de símbolos.
 */
void symtable_print(void) {
    printf("\n");
    printf("============================================================\n");
    printf("                    TABELA DE SÍMBOLOS                      \n");
    printf("============================================================\n");
    
    for (int i = 0; i < SYMBOL_TABLE_SIZE; i++) {
        Symbol* current = symbol_table[i];
        if (current) {
            printf("Bucket[%2d]: ", i);
            while (current != NULL) {
                printf("('%s', %s)", current->lexeme, 
                       token_type_to_string(current->type));
                current = current->next;
                if (current) printf(" -> ");
            }
            printf("\n");
        }
    }
    printf("============================================================\n");
}

/*
 * Libera memória da tabela de símbolos.
 */
void symtable_free(void) {
    for (int i = 0; i < SYMBOL_TABLE_SIZE; i++) {
        Symbol* current = symbol_table[i];
        while (current != NULL) {
            Symbol* temp = current;
            current = current->next;
            free(temp->lexeme);
            free(temp);
        }
        symbol_table[i] = NULL;
    }
}

/* ============================================================================
 * LEITURA DE CARACTERES
 * ----------------------------------------------------------------------------
 * Funções para ler o arquivo de entrada caractere por caractere,
 * mantendo controle da linha e coluna para mensagens de erro.
 * ============================================================================ */

static FILE* inputFile;      /* Arquivo de entrada */
static int line = 1;         /* Linha atual */
static int col = 1;          /* Coluna atual */
static char currentChar;     /* Caractere atual */
static char lexemeBuffer[LEXEME_BUFFER_SIZE];  /* Buffer para lexemas */

/*
 * Avança para o próximo caractere do arquivo.
 * Atualiza contadores de linha e coluna.
 */
void advance(void) {
    currentChar = fgetc(inputFile);
    if (currentChar == '\n') {
        line++;
        col = 1;
    } else {
        col++;
    }
}

/*
 * Espia o próximo caractere sem consumí-lo.
 */
char peek(void) {
    char c = fgetc(inputFile);
    ungetc(c, inputFile);
    return c;
}

/* ============================================================================
 * ANALISADOR LÉXICO - DIAGRAMAS DE TRANSIÇÃO
 * ----------------------------------------------------------------------------
 * Implementação dos autômatos finitos para reconhecimento de tokens.
 * Cada tipo de token possui seu próprio diagrama de transição.
 * ============================================================================ */

/*
 * Cria um token de erro com mensagem descritiva.
 */
static Token create_error_token(const char* message) {
    char* errorLexeme = (char*)malloc(LEXEME_BUFFER_SIZE);
    snprintf(errorLexeme, LEXEME_BUFFER_SIZE, "%s", message);
    return (Token){TOKEN_ERROR, errorLexeme, line, col - 1};
}

/*
 * getToken - Função principal do analisador léxico
 * 
 * Implementa os diagramas de transição para todos os tokens da linguagem.
 * Retorna o próximo token encontrado na entrada.
 */
Token getToken(void) {
    int pos = 0;

    /* Pula espaços em branco (não são tokens) */
    while (isspace(currentChar)) {
        advance();
    }

    int startLine = line;
    int startCol = col - 1;

    /* ----------------------------------------------------------------
     * VERIFICAÇÃO DE FIM DE ARQUIVO
     * ---------------------------------------------------------------- */
    if (currentChar == EOF) {
        return (Token){TOKEN_EOF, "EOF", startLine, startCol};
    }

    /* ----------------------------------------------------------------
     * NÚMEROS (num)
     * Reconhece: sequências de dígitos decimais (0-9)
     * Exemplos: 0, 42, 100, 2025
     * ---------------------------------------------------------------- */
    if (isdigit(currentChar)) {
        /* Estado q1: consome dígitos */
        while (isdigit(currentChar)) {
            lexemeBuffer[pos++] = currentChar;
            advance();
        }
        lexemeBuffer[pos] = '\0';
        return (Token){TOKEN_NUM, strdup(lexemeBuffer), startLine, startCol};
    }

    /* ----------------------------------------------------------------
     * IDENTIFICADORES E PALAVRAS-CHAVE (id)
     * 
     * Reconhece: sequências começando com letra, seguida de letras/dígitos
     * Exemplos: x, X, func1, variavel, A1B2
     * 
     * Após reconhecer, consulta a tabela de símbolos:
     *   - Se for palavra-chave → retorna tipo específico
     *   - Caso contrário → retorna TOKEN_ID
     * ---------------------------------------------------------------- */
    if (isalpha(currentChar)) {
        /* Estado q1: consome letras e dígitos */
        while (isalnum(currentChar)) {
            lexemeBuffer[pos++] = currentChar;
            advance();
        }
        lexemeBuffer[pos] = '\0';
        
        /* Consulta tabela de símbolos (maximal munch) */
        return symtable_lookup_insert(lexemeBuffer, startLine, startCol);
    }

    /* ----------------------------------------------------------------
     * OPERADOR '==' OU ATRIBUIÇÃO '='
     * ---------------------------------------------------------------- */
    if (currentChar == '=') {
        advance();
        if (currentChar == '=') {
            advance();
            return (Token){TOKEN_EQ, "==", startLine, startCol};
        }
        return (Token){TOKEN_ASSIGN, "=", startLine, startCol};
    }

    /* ----------------------------------------------------------------
     * OPERADOR '!='
     * ---------------------------------------------------------------- */
    if (currentChar == '!') {
        advance();
        if (currentChar == '=') {
            advance();
            return (Token){TOKEN_NEQ, "!=", startLine, startCol};
        }
        return create_error_token("Caractere '!' inesperado. Esperava '!='");
    }

    /* ----------------------------------------------------------------
     * OPERADORES '<' OU '<='
     * ---------------------------------------------------------------- */
    if (currentChar == '<') {
        advance();
        if (currentChar == '=') {
            advance();
            return (Token){TOKEN_LTE, "<=", startLine, startCol};
        }
        return (Token){TOKEN_LT, "<", startLine, startCol};
    }

    /* ----------------------------------------------------------------
     * OPERADORES '>' OU '>='
     * ---------------------------------------------------------------- */
    if (currentChar == '>') {
        advance();
        if (currentChar == '=') {
            advance();
            return (Token){TOKEN_GTE, ">=", startLine, startCol};
        }
        return (Token){TOKEN_GT, ">", startLine, startCol};
    }

    /* ----------------------------------------------------------------
     * TOKENS DE CARACTERE ÚNICO
     * 
     * Estes tokens não precisam de autômato complexo pois são
     * reconhecidos por um único caractere.
     * 
     * Operadores aritméticos: + - * /
     * Delimitadores: ( ) { } , ;
     * ---------------------------------------------------------------- */
    switch (currentChar) {
        /* Operadores aritméticos */
        case '+': advance(); return (Token){TOKEN_PLUS, "+", startLine, startCol};
        case '-': advance(); return (Token){TOKEN_MINUS, "-", startLine, startCol};
        case '*': advance(); return (Token){TOKEN_MULT, "*", startLine, startCol};
        case '/': advance(); return (Token){TOKEN_DIV, "/", startLine, startCol};
        
        /* Delimitadores */
        case '(': advance(); return (Token){TOKEN_LPAREN, "(", startLine, startCol};
        case ')': advance(); return (Token){TOKEN_RPAREN, ")", startLine, startCol};
        case '{': advance(); return (Token){TOKEN_LBRACE, "{", startLine, startCol};
        case '}': advance(); return (Token){TOKEN_RBRACE, "}", startLine, startCol};
        case ',': advance(); return (Token){TOKEN_COMMA, ",", startLine, startCol};
        case ';': advance(); return (Token){TOKEN_SEMICOLON, ";", startLine, startCol};
    }

    /* ----------------------------------------------------------------
     * ERRO LÉXICO: caractere não reconhecido
     * 
     * Se chegou aqui, o caractere não pertence à linguagem LSI-2025-2.
     * Reporta erro com linha e coluna.
     * ---------------------------------------------------------------- */
    char errorMsg[64];
    snprintf(errorMsg, sizeof(errorMsg), 
             "Caractere invalido: '%c' (ASCII %d)", currentChar, currentChar);
    advance();
    return create_error_token(errorMsg);
}

/* ============================================================================
 * CONVERSÃO DE TIPO PARA STRING
 * ============================================================================ */

const char* token_type_to_string(TokenType type) {
    switch (type) {
        /* Palavras-chave */
        case TOKEN_INT:       return "int";
        case TOKEN_IF:        return "if";
        case TOKEN_ELSE:      return "else";
        case TOKEN_DEF:       return "def";
        case TOKEN_PRINT:     return "print";
        case TOKEN_RETURN:    return "return";
        
        /* Identificadores e literais */
        case TOKEN_ID:        return "id";
        case TOKEN_NUM:       return "num";
        
        /* Operadores relacionais */
        case TOKEN_LT:        return "<";
        case TOKEN_LTE:       return "<=";
        case TOKEN_GT:        return ">";
        case TOKEN_GTE:       return ">=";
        case TOKEN_EQ:        return "==";
        case TOKEN_NEQ:       return "!=";
        
        /* Operadores aritméticos */
        case TOKEN_PLUS:      return "+";
        case TOKEN_MINUS:     return "-";
        case TOKEN_MULT:      return "*";
        case TOKEN_DIV:       return "/";
        
        /* Atribuição */
        case TOKEN_ASSIGN:    return "=";
        
        /* Delimitadores */
        case TOKEN_LPAREN:    return "(";
        case TOKEN_RPAREN:    return ")";
        case TOKEN_LBRACE:    return "{";
        case TOKEN_RBRACE:    return "}";
        case TOKEN_COMMA:     return ",";
        case TOKEN_SEMICOLON: return ";";
        
        /* Especiais */
        case TOKEN_EOF:       return "EOF";
        case TOKEN_ERROR:     return "ERRO";
        
        default:              return "DESCONHECIDO";
    }
}

/* ============================================================================
 * FUNÇÃO PRINCIPAL
 * ============================================================================ */

int main(int argc, char* argv[]) {
    /* Verifica argumentos */
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <arquivo.lsi>\n", argv[0]);
        fprintf(stderr, "Exemplo: %s programa.lsi\n", argv[0]);
        return 1;
    }

    /* Abre o arquivo de entrada */
    inputFile = fopen(argv[1], "r");
    if (!inputFile) {
        perror("Erro ao abrir arquivo");
        return 1;
    }

    printf("============================================================\n");
    printf("       ANALISADOR LÉXICO - LINGUAGEM LSI-2025-2             \n");
    printf("============================================================\n");
    printf("Arquivo: %s\n", argv[1]);
    printf("============================================================\n\n");

    /* Inicializa tabela de símbolos e lê primeiro caractere */
    symtable_init();
    advance();

    /* Processa todos os tokens */
    Token token;
    int hasError = 0;
    int tokenCount = 0;

    printf("--- LISTA DE TOKENS ---\n\n");
    printf("%-6s %-6s %-14s %s\n", "Linha", "Col", "Tipo", "Lexema");
    printf("------ ------ -------------- --------------------\n");

    do {
        token = getToken();
        tokenCount++;
        
        printf("%-6d %-6d %-14s '%s'\n",
               token.line, token.col,
               token_type_to_string(token.type), 
               token.lexeme);

        if (token.type == TOKEN_ERROR) {
            fprintf(stderr, "\n");
            fprintf(stderr, "!!! ERRO LÉXICO !!!\n");
            fprintf(stderr, "Linha %d, Coluna %d: %s\n",
                   token.line, token.col, token.lexeme);
            hasError = 1;
            break;
        }
    } while (token.type != TOKEN_EOF);

    printf("------ ------ -------------- --------------------\n");
    printf("Total: %d tokens\n", tokenCount - 1); /* -1 para excluir EOF */

    /* Imprime tabela de símbolos se não houve erro */
    if (!hasError) {
        symtable_print();
        printf("\n✓ Análise léxica concluída com SUCESSO!\n");
    } else {
        printf("\n✗ Análise léxica FALHOU.\n");
    }

    /* Libera recursos */
    fclose(inputFile);
    symtable_free();

    return hasError;
}