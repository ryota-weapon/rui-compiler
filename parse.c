#include <stdlib.h>
#include <string.h>
#include "9cc.h"

#include <stdio.h> // debug

Node *build_block_node() {
    Node *node;
    node = calloc(1, sizeof(Node));
    node->kind = ND_BLOCK;

    Node **node_buf = calloc(1, sizeof(Node)*10); // 一旦固定で10行までサポート
    int i = 0;
    while (!consume("}")) {
        node_buf[i++] = stmt();
        if (i==10)
            error("too bigなブロックは今サポートされていない.");
    }
    node->stmt_len = i;
    node->stmt = node_buf;
    return node;
}

LVar *register_new_lvar(Token *ident_tok) {
    LVar *lvar = calloc(1, sizeof(LVar));
    lvar->next = locals;
    lvar->name = ident_tok->str;
    lvar->len = ident_tok->len;
    // localsリストが空であり、localsポインタがNULLの時にバグる
    if (locals) {
        lvar->offset = locals->offset + 8;
    } else {
        lvar->offset = 8;
    }
    locals = lvar;

    return lvar;
}

Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node *new_node_num(int val) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_NUM;
    node->val = val;
    return node;
}

// LVarの連結リストから探し出す
LVar *find_lvar(Token *tok) {
    for (LVar *var=locals; var; var = var->next) {
        if (tok->len == var->len && !memcmp(tok->str, var->name, var->len))
            return var;
    }
    return NULL;
}

Function *find_fn(Token *tok) {
    for (Function *var=funcs; var; var = var->next) {
        if (tok->len == var->len && !memcmp(tok->str, var->name, var->len))
            return var;
    }
    return NULL;
}


Node *program() {
    int i = 0;
    while (!at_eof()) {
        code[i++] = stmt();
    }
    code[i] = NULL;
}

// Step17でのサポート、 1. int x;という宣言, 2. int func(int a){...}という関数定義
Node *stmt() {
    Node *node;

    // if (consume(TK_RETURN)) {
    // consumeは現状Returnに対応していない
    if (consume("return")) { // i think this should be like this??
        node = calloc(1, sizeof(Node));
        node->kind = ND_RETURN;
        node->lhs = expr();
    } else if (consume("while")) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_WHILE;
        expect("(");
        node->cond = expr();
        expect(")");
        node->body = stmt();
        return node; // 下にある;の存在チェックと重複してしまって2個必要になってしまうため
    } else if (consume("for")) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_FOR;
        expect("(");
        node->init = expr();
        expect(";");
        node->cond = expr();
        expect(";");
        node->inc = expr();
        expect(")");
        node->body = stmt();

        return node;
    } else if (consume("if")) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_IF;
        expect("(");
        node->cond = expr();
        expect(")");
        node->body = stmt();

        if (consume("else")) {
            node->_else = stmt();
        }
        return node;
    } else if (consume("{")) {
        node = build_block_node(); // consume } in this func
        return node;
    } else if (consume_type()) { // 関数定義 or 変数宣言
        Token *type_tok = token;
        token = token->next;
        if (!consume_ident())
            error_at(token->str, "型が来たのに関数名or変数名がありません");
        Token *ident_tok = token;
        token = token->next;

        if (consume("(")) {
            // 関数定義
            Function *fn = calloc(1, sizeof(Function));
            fn->name = ident_tok->str;
            fn->len = ident_tok->len;
            if (funcs) {
                fn->next = funcs;
            }
            funcs = fn;

            // args
            while (consume_type()) {
                Token *arg_type_tok = token;
                token = token->next;

                if (!consume_ident) {
                    error_at(token->str, "引数に変数名がありません");
                }
                Token *arg_tok = token;
                token = token->next;

                // TODO: 本当はスコープの階層構造みたいな概念が必要だと思うが、いったんLVarに追加
                LVar *lvar = register_new_lvar(arg_tok); 
                
                if (!consume(",")) break;
            }

            expect(")");

            Node *body;
            // TO-FIX: これってさ、stmt()だけで実行できるのでは？
            if (consume("{")) {
                body = build_block_node(); // consume } in this func
            } else {
                body = stmt();
            }
            fn->body = body;
            node = calloc(1, sizeof(Node));
            node->kind = ND_FUNC_DEF;
            node->func = fn;
            // node->func = fn;
            return node;
        } else if (*token->str == ';' || *token->str == ',') { // consumeしちゃうと崩れちゃうから、対応
            // 変数の宣言である: 変数をLocalsに登録する
            LVar *lvar = register_new_lvar(ident_tok); 
            node = calloc(1, sizeof(Node));
            node->offset = lvar->offset;
            node->kind = ND_LVAR_DEF;

            // TODO: , で区切って複数同時宣言のサポート
            while (consume(",")) {
                if (!consume_ident())
                    error_at(token->str, "変数名がありません");
                Token *ident_tok = token;
                token = token->next;
                LVar *lvar = register_new_lvar(ident_tok);
            }

            expect(";");
        } else {
            error_at(token->str, "型の後は、関数定義か変数宣言のどちらかの形式で書いてください");
        }
        return node;
    } else {
        node = expr();
    }

    if (!consume(";"))
        error_at(token->str, "';'ではないトークンです");

    return node;
}

Node *assign() {
    Node *node = equality();
    if (consume("=")) {
        // node->kind = ND_LVAR; // 誤植？　これ必要では？
        // 現状だけかも a=b=cみたいなことができちゃうよ
        node = new_node(ND_ASSIGN, node, assign()); // というか、ここassignが適切なのか？？
    }
    
    return node;
}

Node *expr() {
    return assign();
}

Node *relational() {
    Node *node = add();
    for (;;) {
        if (consume("<=")) {
            node = new_node(ND_LTE, node, add());
        }
        if (consume(">=")) {
            node = new_node(ND_LTE, add(), node);
        }
        if (consume("<")) {
            node = new_node(ND_LT, node, add());
        }
        if (consume(">")) {
            node = new_node(ND_LT, add(), node);
        }

        return node;
    }
}

Node *equality() {
    Node *node = relational();
    for (;;) {
        if (consume("==")) {
            node = new_node(ND_EQ, node, relational());
        }
        if (consume("!=")) {
            node = new_node(ND_NEQ, node, relational());
        }
        return node;
    }
}

Node *add() {
    Node *node = mul();
    
    for (;;) {
        if (consume("+")) 
            node = new_node(ND_ADD, node, mul());
        else if (consume("-")) 
            node = new_node(ND_SUB, node, mul());
        else
            return node;
    }
}

//  tokenを構成する文字か否か？ 
// (returnかどうかの判定に使う、returnの後ろを見る)
int is_alnum(char c) {
    return ('a' <= c && c <= 'z') ||
           ('A' <= c && c <= 'Z') ||
           ('0' <= c && c <= '9') ||
           (c == '_');
}

// この関数が使われているところから、推察して実装
// おそらく変数だったら、そのトークンのポインタを返せばよい
Token *consume_ident() {
    extern Token *token;
    // // このtokenは、グローバル変数

    // 未登録ならとりあえず作るという仕様にしようかな
    if (token->kind != TK_IDENT) 
        return NULL;

    return token;
}

bool consume_type() {
    extern Token *token;
    if (token->kind != TK_TYPE) 
        return false;
    return true;
}

Node *primary() {
    if (consume("(")) {
        Node *expr_node =  expr();
        expect(")");
        return expr_node;
    }
    Token *tok = consume_ident();
    if (tok) {
        Node *node = calloc(1, sizeof(Node));
        node->kind = ND_LVAR;

        LVar *lvar = find_lvar(tok);
        Function *func = find_fn(tok);
        if (lvar) {
            node->offset = lvar->offset;
        } else if (!func) {
            error_at(token->str, "変数 or 関数が宣言されていません");
        }
    
        token = token->next;
        // 関数callかもしれない
        if (consume("(")) {
            int count = 0;
            Node **args = calloc(1, sizeof(Node)*10);
            Node *first_arg = consume_arg();
            if (first_arg) 
                args[0] = first_arg;

            while (!consume(")")) {
                expect(",");
                args[count++] = consume_arg();
            }
            
            //　非効率なコードだけどうわがきする
            node = (Node *)calloc(1, sizeof(Node));
            node->kind = ND_FUNC_CALL;
            node->func = func;
            node->args = args;
            node->arg_len = count;
        }
        // TODO: align RSP to 16byte due to the ABI
        return node;
    }

    return new_node_num(expect_number());
}

Node *unary() {
    if (consume("+")) {
        return primary();
    }
    if (consume("-")) {
        return new_node(ND_SUB, new_node_num(0), primary());
    }
    if (consume("*")) {
        return new_node(ND_DEREF, unary(), NULL);
    }
    if (consume("&")) {
        return new_node(ND_ADDR, unary(), NULL);
    }
    return primary();
}

// mulを先に定義したい, 循環依存がある
Node *mul() {
    Node *node = unary();
    for (;;) {
        if (consume("*")) 
            node = new_node(ND_MUL, node, unary());
        else if (consume("/")) 
            node = new_node(ND_DIV, node, unary());
        else
            return node;
    }
}

