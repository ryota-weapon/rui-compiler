#ifndef NINECC_H
#define NINECC_H

#include <stdbool.h>

typedef enum {
    TK_RESERVED,
    TK_TYPE,
    TK_IDENT,
    TK_NUM,
    TK_EOF,
    TK_RETURN,
    // TK_SEP,
    TK_STR, // 文字列リテラル
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
    ND_GVAR,
    ND_RETURN,

    ND_ADDR,
    ND_DEREF,

    ND_WHILE,
    ND_IF,
    ND_FOR,
    ND_BLOCK,
    ND_FUNC_CALL,
    ND_FUNC_DEF,

    ND_LVAR_DEF,
    ND_SIZEOF,

    ND_DECAY, // trialです
    ND_STR, // 文字列リテラル
} NodeKind;


typedef struct Function Function;
typedef struct Node Node;
typedef struct Type Type;
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

    Function *func;
    char *func_symbol;
    int func_symbol_len;

    char *gvar_name;
    int gvar_name_len;

    char *str; // 文字列リテラルの内容
    int str_len;
    

    Type *type;
};

typedef enum { TY_INT, TY_PTR, TY_ARRAY, TY_CHR } TypeKind;

// 配列はサイズ分だけスタック領域に確保する

typedef unsigned long size_t;
struct Type {
    TypeKind kind;
    Type *ptr_to; // typeがPTRのとき、指している型
    size_t array_size; // size_tは符号なし整数型
};

typedef struct Function Function;
struct Function {
    Function *next;
    char *name;
    int len;
    int arg_len;
    Node *body;
};

typedef struct LVar LVar;
struct LVar {
    LVar *next; // 連結リスト
    char *name;
    int len;
    int offset;
    Type *type;
};


typedef struct GVar GVar;
struct GVar {
    GVar *next; // 連結リスト
    char *name;
    int len;
    Type *type;
};


extern LVar *locals;
LVar *find_lvar(Token *tok);

extern GVar *globals;
GVar *find_gvar(Token *tok);

extern Function *funcs; // 連結リスト
Function *find_fn(Token *tok);

typedef struct StringLiteral StringLiteral;
struct StringLiteral {
    char *content;
    int len;
    StringLiteral *next;
};

extern StringLiteral *string_literals; // 文字列リテラルの内容を保存するための配列

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
Token *consume_ident();
bool consume_type();
int consume_pointer();
void expect(char *op);
int expect_number();

Type *build_type(TypeKind kind, int pointer_count);
Type *new_type(TypeKind kind, Type *base);
Type *new_type_array(Type *base, int array_size);
Type *ty_int(void);
Type *pointer_to(Type *base);
TypeKind token_to_type_kind(Token *tok);

int size_of(Type *ty);

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
