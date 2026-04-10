#ifndef NINECC_H
#define NINECC_H

#include <stdbool.h>

typedef enum {
    TK_RESERVED,
    TK_IDENT,
    TK_NUM,
    TK_EOF,
    TK_RETURN,
    // TK_SEP,
} TokenKind;

// 自己参照に使う構文らしい, (TokenがTokenを参照しているでしょ、このstruct)
typedef struct Token Token;

struct Token {
    TokenKind kind;
    Token *next;
    int val;
    char *str;
    int len; // 記号トークンの長さが1文字から可変長になった
};

// ASTのノードの種類
typedef enum {
    ND_ADD,
    ND_SUB,
    ND_MUL,
    ND_DIV,
    ND_NUM,
    ND_LT,
    ND_LTE,
    // ND_GT,
    // ND_GTE,
    ND_EQ,
    ND_NEQ,
    ND_ASSIGN,
    ND_LVAR,
    ND_RETURN,

    ND_ADDR,
    ND_DEREF,

    ND_WHILE,
    ND_IF,
    ND_FOR,
    ND_BLOCK,
    ND_FUNC_CALL,
} NodeKind;

typedef struct Node Node;
struct Node {
    NodeKind kind;
    Node *lhs; // left-hand side
    Node *rhs; // right-hand side
    int val; // kindがND_NUMの時だけ
    int offset; // kindがND_LVARのときだけ（RBPからどれだけずれるか？）
    Node *cond; // for only if, while, for
    Node *body; // for, while, if-else
    Node *_else; // if-else
    Node *inc; // for
    Node *init; // for

    Node **stmt; // pointer to a stmt pointer array (only for block)
    int stmt_len;

    Node **args;
    int arg_len;
};

typedef struct LVar LVar;
struct LVar {
    LVar *next; // 連結リスト
    char *name;
    int len;
    int offset;
};

extern LVar *locals;
LVar *find_lvar(Token *tok);


// parser
int is_alnum(char c);

Node *program();
Node *stmt();
Node *expr();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();

// general
Node *new_node(NodeKind kind, Node *lhs, Node *rhs);
Node *new_node_num(int val);
bool consume(char *op);
Node *consume_arg();
void expect(char *op);
int expect_number();


// gen
void gen(Node *node);

// tokenizer
Token *new_token(TokenKind kind, Token *cur, char *str);
Token *tokenize(char *p);


// global
extern char *user_input;
extern Token *token;
extern Node *code[100];

// util
void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
bool at_eof();

#endif
