#include <stdlib.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "9cc.h"
#include <stdio.h> // for debug

// 期待と合致しているか？ それなら読み進めてtrue
bool consume(char *op) {
    if (strncmp("return", op, 6)==0 && strncmp("return", token->str, 6)==0 
    && token->kind == TK_RETURN) {
        token = token->next;
        return true;
    }
    
    // if (strncmp("while", op, 5)==0 && strncmp("while", token->str, 5)==0 
    //      && token->kind == TK_RESERVED) {
    //     token = token->next;
    //     return true;
    // }
    
    // if (strncmp("if", op, 2)==0 && strncmp("if", token->str, 2)==0 
    //      && token->kind == TK_RESERVED) {
    //     token = token->next;
    //     return true;
    // }

    // あれなんかやってる子と同じじゃね、結局最初のコードのままでよかったのか？
    if (strncmp(op, token->str, strlen(op))==0 && token->kind == TK_RESERVED) {
        token = token->next;
        return true;
    }

    if (token->kind != TK_RESERVED 
        || strlen(op) != token->len
        || memcmp(token->str, op, token->len)) {
        return false;
    }
    token = token->next;
    return true;
}

Node * consume_arg() {
    Node *arg_node = (Node *)calloc(1, sizeof(Node));
    // ident or num,
    LVar *lvar = find_lvar(token);
    if (lvar) {
        arg_node->offset = lvar->offset;
        token = token->next;
    } else if (token -> kind == TK_NUM) {
        arg_node = new_node_num(expect_number());
    } else 
        return NULL;
        
    return arg_node;
}

void expect(char *op) {
    if (token->kind != TK_RESERVED || 
    strlen(op) != token->len ||
    memcmp(token->str, op, token->len)) {
        error("'%c'ではありません, (%d)", op, op);
    }
    token = token->next;
}

int expect_number() {
    if (token -> kind != TK_NUM) 
        error("数ではない");
    int val = token->val;
    token = token->next;
    return val;
}

bool at_eof() {
    // バグるので、一応安全策を入れてみる,
    // これでバグの位置が変わったので, tokenがnullの時にバグってたことがわかる
    // eofのトークンがトークン列に存在しないんだろう。バグで
    // if (!token) return false;
    return token->kind == TK_EOF;
}

Token *new_token(TokenKind kind, Token *cur, char *str) {
    Token *tok = calloc(1, sizeof(Token)); // callocとは？
    // callocは、mallocと同じようにメモリ割り当てをするが、mallocと違い割り当てたバッファのゼロクリアをしてくれる
    tok->kind = kind;
    tok->str = str;
    cur->next = tok;
    
    return tok;
}

Token *tokenize(char *p) {
    // extern Token *token; 別に要らない

    Token head;
    head.next = NULL;
    Token *cur = &head;

    
    while (*p) {
        // printf("debug: '%c' (%d)\n", *p, *p);

        if (isspace(*p)) {
            p++;
            continue;
        }

        if (strncmp(p, "while", 5) == 0 && !is_alnum(p[5])) {
            Token *tok = calloc(1, sizeof(Token));
            tok->kind = TK_RESERVED;
            tok->len = 5;
            tok->str = p;
            cur->next = tok;
            cur = tok;
            p += 5;
            continue;
        }
        if (strncmp(p, "if", 2) == 0 && !is_alnum(p[2])) {
            Token *tok = calloc(1, sizeof(Token));
            tok->kind = TK_RESERVED;
            tok->len = 2;
            tok->str = p;
            cur->next = tok;
            cur = tok;
            p += 2;
            continue;
        }
        if (strncmp(p, "else", 2) == 0 && !is_alnum(p[4])) {
            Token *tok = calloc(1, sizeof(Token));
            tok->kind = TK_RESERVED;
            tok->len = 4;
            tok->str = p;
            cur->next = tok;
            cur = tok;
            p += 4;
            continue;
        }
        if (strncmp(p, "for", 3) == 0 && !is_alnum(p[3])) {
            Token *tok = calloc(1, sizeof(Token));
            tok->kind = TK_RESERVED;
            tok->len = 3;
            tok->str = p;
            cur->next = tok;
            cur = tok;
            p += 3;
            continue;
        }

        if (strncmp(p, "return", 6) == 0 && !is_alnum(p[6])) {
            Token *tok = calloc(1, sizeof(Token));
            tok->kind = TK_RETURN;
            tok->str = p;
            cur->next = tok;
            cur = tok;
            // tokens[i].kind = TK_RETURN;　誤植？
            // tokens[i].str = p; 誤植？
            // i++;
            p += 6;
            continue;
        }

        if (strncmp(p, "==", 2)==0 || strncmp(p, "!=", 2)==0 || strncmp(p, ">=", 2)==0 || strncmp(p, "<=", 2)==0) {
            cur = new_token(TK_RESERVED, cur, p++);
            cur->len = 2;
            p++;
            continue;
        }
        
        if (*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '(' || *p == ')' ||
            *p == '<' || *p == '>' || *p == '&'
        ) {
            cur = new_token(TK_RESERVED, cur, p++);
            cur->len = 1;
            continue;
        }

        if (isdigit(*p)) {
            cur = new_token(TK_NUM, cur, p);
            cur->val = strtol(p, &p, 10);
            continue;
        }

        if (is_alnum(*p)) {
            cur = new_token(TK_IDENT, cur, p);
            while (is_alnum(*p)) p++;
            cur->len = p - cur->str;
            continue;
        }

        // 誤植？
        // これ書かれてないけど、必要な気がするがどうでしょうか？
        if (*p == ';') {
            cur = new_token(TK_RESERVED, cur, p++);
            cur->len = 1;
            continue;
        }
        if (*p == '=') {
            cur = new_token(TK_RESERVED, cur, p++);
            cur->len = 1;
            continue;
        }
        if (*p == '{' || *p == '}' || *p == ',') {
            cur = new_token(TK_RESERVED, cur, p++);
            cur->len = 1;
            continue;
        }

        error("トークナイズできません");
    }

    new_token(TK_EOF, cur, p);
    return head.next;
}
