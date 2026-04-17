#include <stdlib.h>
#include <string.h>
#include "9cc.h"

#include <stdio.h> // debug

#define MAX_BLOCK_STMT 20

Node *build_block_node() {
    Node *node;
    node = calloc(1, sizeof(Node));
    node->kind = ND_BLOCK;

    Node **node_buf = calloc(1, sizeof(Node)*MAX_BLOCK_STMT); // 一旦固定でMAX_BLOCK_STMT行までサポート
    int i = 0;
    while (!consume("}")) {
        node_buf[i++] = stmt();
        if (i==MAX_BLOCK_STMT)
            error("too bigなブロックは今サポートされていない.");
    }
    node->stmt_len = i;
    node->stmt = node_buf;
    return node;
}

int size_of(Type *ty) {
    if (ty->kind == TY_INT) {
        return 4;
    } else if (ty->kind == TY_PTR) {
        return 8;
    } else if (ty->kind == TY_ARRAY) {
        return size_of(ty->ptr_to) * ty->array_size;
    } else {
        error("intでもptrでもない型のsizeofはサポートしていません");
    }
}

LVar *register_new_lvar(Token *ident_tok, Type *type) {
    if (find_lvar(ident_tok)) {
        error_at(ident_tok->str, "変数はすでに宣言されています");
    }
    if (find_fn(ident_tok)) {
        error_at(ident_tok->str, "関数と同じ名前の変数は宣言できません");
    }

    LVar *lvar = calloc(1, sizeof(LVar));
    lvar->next = locals;
    lvar->name = ident_tok->str;
    lvar->len = ident_tok->len;
    lvar->type = type;
    // localsリストが空であり、localsポインタがNULLの時にバグる


    if (locals) {
        // sizeを把握する
        // 一個前の変数のオフセット　＋　その変数のサイズ分だけずらせばよいのでは？
        int size = size_of(locals->type);
        int aligned = (size + 7) & ~7; // 8の倍数に切り上げる
        // このコードは、~はビットの反転, 00000111を反転すると11111000になる、
        // これとANDをとると、8で割った余りを捨てることができる
        // そして、+7しているのは、切り下げではなく、切り上げにしたいから
        lvar->offset = locals->offset + aligned;
    } else {
        // ローカル変数のアクセスは、RBP + offset
        // 関数の開始時、*RBPは、前のフレーム（呼び出し元の関数）のRBPを保持している（サイズは8バイト）
        // すなわち、最初のローカル変数のオフセットは、それを避けて8
        lvar->offset = 8;
    }
    locals = lvar;

    return lvar;
}

Type *decay_array(Type *ty) {
    if (ty->kind == TY_ARRAY) {
        ty->kind = TY_PTR;
        return ty;
    } else {
        error("配列以外の型のディケイはサポートしていません");
    }
}

Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->lhs = lhs;
    node->rhs = rhs;

    if (node->kind == ND_DEREF) {
        // if (lhs->type->kind != TY_PTR && lhs->type->kind != TY_ARRAY) {
        if (lhs->type->kind == TY_ARRAY) {
            // WARN: おそらくこのパスは通らなくなった
            // arrayをDEREFの対象にするということで、ディケイを実行する、このタイミングが一定合理的だと思われる
            lhs->type = decay_array(lhs->type);
        }
        // printf("debug: deref node, lhs type kind: %d\n", lhs->type->kind);
        if (lhs->type->kind != TY_PTR) {
            // arrayの場合は、配列の先頭のアドレスを指すポインタにdecay
            // NOTE: もしかしたらポインタのままでよいかも, 別の機構にてarray->pointerのディケイをやっておく
            // すると、*(a+1)みたいなことをやるときに、今まで通りのポインタ加算の仕組みで解決できる
            error_at(token->str, "デリファレンスをするのはポインタか配列に制限している");
        }
        node->type = lhs->type->ptr_to;
        return node; // バグを生んでいる可能性があるので、もう後の処理で上書きされないように終了
    } else if (node->kind == ND_ADDR) {
        // if (lhs->kind != ND_LVAR) {
        //     error_at(token->str, "変数じゃないものに対してアドレスをとることは、現状できない");
        // }
        // NOTE: 一旦別にいいや、てか多分別に良い
        node->type = new_type(TY_PTR, node->lhs->type);
    } else if (node->kind == ND_ASSIGN) {
        if (lhs->type->kind == TY_INT && rhs->type->kind == TY_PTR) {
            error_at(token->str, "int型の変数にptr型の値を代入することはできません");
        }
    }

    if (lhs && rhs) {
        // TODO: 本来はptr + ptrは、エラーになるらしい。
        if (lhs->type->kind == TY_PTR || rhs->type->kind == TY_PTR) {
            Node *pointerNode = lhs->type->kind == TY_PTR ? lhs : rhs;
            node->type = pointerNode->type;
        } else {
            node->type = ty_int();
        }
    } else if (lhs) {
        node->type = lhs->type;
    } else if (rhs) { // こんなケースは存在しない
        // node->type = rhs->type;
        error("rhsだけのノードは今のところ存在しないはず");
    }
    return node;
}

Node *new_node_num(int val) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_NUM;
    node->val = val;
    node->type = ty_int(); // 数値リテラルはint型
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

        int ptr_count = consume_pointer();
        Type *ty = build_type(token_to_type_kind(type_tok), ptr_count);

        if (!consume_ident())
            error_at(token->str, "型が来たのに関数名or変数名がありません");
        Token *ident_tok = token;
        token = token->next;

        // option: 通常の変数、関数定義、配列（new)
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

                int ptr_count = consume_pointer();
                Type *ty = build_type(token_to_type_kind(arg_type_tok), ptr_count);

                if (!consume_ident()) {
                    error_at(token->str, "引数に変数名がありません");
                }
                Token *arg_tok = token;
                token = token->next;

                // TODO: 本当はスコープの階層構造みたいな概念が必要だと思うが、いったんLVarに追加
                LVar *lvar = register_new_lvar(arg_tok, ty);
                
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
        } else if (consume("[")) {
            // 配列の宣言：　スタック領域に直接バッファを確保する
            int size = expect_number();
            // NOTE: これ数値リテラルとは限らないかも？ c言語の仕様を知らない。

            Type *array_ty = new_type_array(ty, size); // 配列の型を作る、配列の要素の型はty
            LVar *lvar = register_new_lvar(ident_tok, array_ty);
            node = calloc(1, sizeof(Node));
            node->kind = ND_LVAR_DEF;
            node->offset = lvar->offset;
            node->type = array_ty;

            expect("]");
            expect(";");
        } else if (*token->str == ';' || *token->str == ',') { // consumeしちゃうと崩れちゃうから、対応
            // 変数の宣言である: 変数をLocalsに登録する
            Type *ty = build_type(token_to_type_kind(type_tok), ptr_count); // 注意、結構遠いところのptr_countを参照している
            LVar *lvar = register_new_lvar(ident_tok, ty);

            node = calloc(1, sizeof(Node));
            node->offset = lvar->offset;
            node->kind = ND_LVAR_DEF;
            node->type = ty;
            // TODO: , で区切って複数同時宣言のサポート
            while (consume(",")) {
                if (!consume_ident())
                    error_at(token->str, "変数名がありません");
                Token *ident_tok = token;
                token = token->next;
                // int ptr_count = consume_pointer(); ここではいらないだろう
                Type *ty = build_type(token_to_type_kind(type_tok), ptr_count);
                LVar *lvar = register_new_lvar(ident_tok, ty);
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
        if (token->str[0] == '+' || token->str[0] == '-') {
            NodeKind kind = token->str[0] == '+' ? ND_ADD : ND_SUB; // どっちの演算子だったかを覚えておく
            token = token->next;

            Node *lhs = node;
            Node *rhs = mul();

            // WARN: 配列のインデクシングについて、このパスを通らないです！ addではなくnew_nodeで作っているから
            // // TODO: add array support
            // if (lhs->type->kind == TY_ARRAY) {
            //     // decayしてポインタにする
            //     lhs->type = decay_array(lhs->type); // これだと, intとかになってしまうのでは？
            // } else if (rhs->type->kind == TY_ARRAY) {
            //     rhs->type = decay_array(rhs->type);
            // }


            // add or sub, pointer 8, int: 4
            if (lhs->type->kind == TY_PTR && rhs->type->kind == TY_INT) {
                rhs = new_node(ND_MUL, rhs, new_node_num(size_of(lhs->type->ptr_to)));
                node = new_node(kind, lhs, rhs);
            } else if (lhs->type->kind == TY_INT && rhs->type->kind == TY_PTR) {
                lhs = new_node(ND_MUL, lhs, new_node_num(size_of(rhs->type->ptr_to)));
                node = new_node(kind, lhs, rhs);
            } else if (lhs->type->kind == TY_PTR && rhs->type->kind == TY_PTR && kind == ND_ADD) {
                error_at(token->str, "ポインタ同士の加算はサポートしていません");
            } else { // ptr同士の減算はサポートしている, この時、倍率を考慮する必要はないはず
                node = new_node(kind, lhs, rhs);
            }
        } else {
            return node;
        }
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

int consume_pointer() {
    int count = 0;
    while (consume("*")) {
        count++;
    }
    // printf("debug: inside func; pointer_count: %d\n", count);
    return count;
}

TypeKind token_to_type_kind(Token *tok) {
    if (strncmp(tok->str, "int", tok->len) == 0) {
        return TY_INT;
    }
    error_at(tok->str, "未対応の型です");
}

Type *new_type(TypeKind kind, Type *base) {
    Type *ty = calloc(1, sizeof(Type));
    ty->kind = kind;
    ty->ptr_to = base;
    return ty;
}

Type *new_type_array(Type *base, int array_size) {
    Type *ty = calloc(1, sizeof(Type));
    ty->kind = TY_ARRAY;
    ty->ptr_to = base;
    ty->array_size = array_size;
    return ty;
}

Type *ty_int(void) {
    static Type ty = { TY_INT };
    return &ty;
}

Type *pointer_to(Type *base) {
    return new_type(TY_PTR, base);
}

Type *build_type(TypeKind kind, int pointer_count) {
    Type *ty;
    if (kind == TY_INT) {
        ty = ty_int();
    } else {
        error("未対応の型です");
    }
    for (int i=0; i < pointer_count; i++) {
        ty = pointer_to(ty);
    }

    return ty;
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
            node->type = lvar->type;
            // printf("debug: var name: %.*s, offset: %d, type: %d\n", lvar->len, lvar->name, lvar->offset, lvar->type->kind);
        } else if (!func) {
            // TODO: 外部関数の呼び出しを雑にできるように臨時パッチ
            // error_at(token->str, "変数 or 関数が宣言されていません");
        }
    
        token = token->next;
        // 関数callかもしれない
        if (consume("(")) {
            int count = 0;
            Node **args = calloc(1, sizeof(Node)*10);
            
            if (!consume(")")) {
                args[count++] = expr();
                while (consume(",")) {
                    args[count++] = expr();
                }
                expect(")");
            }
            
            //　非効率なコードだけどうわがきする
            node = (Node *)calloc(1, sizeof(Node));
            node->kind = ND_FUNC_CALL;
            // node->func = func; // WARN: oh my gosh, 外部関数の時に摘む
            node->func_symbol = tok->str;
            node->func_symbol_len = tok->len;
            node->args = args;
            node->arg_len = count;
        } else if (consume("[")) {
            // 配列の添え字アクセス: a[b] -> *(aのアドレス + b) (*(a+b)になるが, aがディケイしてポインタになる)
            Node *index = expr();
            // ここで、indexの型がintであることを確認しておきたい
            if (index->type->kind != TY_INT) {
                error_at(token->str, "配列のインデックスはint型でなければなりません");
            }
            expect("]");

            // nodeはLVARが入っている, 配列の変数である
            // indexは式として解釈されて、その計算結果が数値としてくると期待する
            // Node *mul = new_node(ND_MUL, index, new_node_num(size_of(node->type->ptr_to))); // インデックスに要素のサイズをかける
            // ↑とんでもなく遅いと思う
            Node *addr = new_node(ND_ADD, node, new_node_num(size_of(node->type->ptr_to)*index->val)); // 配列の先頭アドレス + インデックス * 要素のサイズ
            addr->type = pointer_to(node->type->ptr_to); // 配列の要素の型へのポインタ

            node = new_node(ND_DEREF, addr, NULL);
            node->type = node->lhs->type->ptr_to; // 配列の要素の型
        }
        

        // 型情報の考慮をしてあげたい (ここが適切かは不明)
        // TODO: 関数の戻り値の型の管理
        

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
    if (consume("sizeof")) {
        return new_node(ND_SIZEOF, unary(), NULL);
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

