#include <stdio.h>
#include "9cc.h"

Node *code[100];
Token *token; // 現在のtoken pointer
char *user_input; // ユーザの入力したプログラム
LVar *locals;
GVar *globals;
Function *funcs;

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "引数の個数が違うよ\n");
        return 1;
    }

    user_input = argv[1];
    // tokenize(user_input); 誤植？
    token = tokenize(user_input);
    program();

    printf(".intel_syntax noprefix\n");

    // Global variablesを先に定義してしまう
    printf(".data\n");
    for (GVar *gvar=globals; gvar; gvar = gvar->next) {
        if (gvar->type->kind == TY_ARRAY) {
            printf("%.*s:\n", gvar->len, gvar->len, gvar->name);
            printf("  .zero %d\n", size_of(gvar->type)); // 0で初期化された領域を確保する
        } else if (gvar->type->kind == TY_INT || gvar->type->kind == TY_PTR) {
            printf("%.*s:\n", gvar->len, gvar->name);
            printf("  .zero %d\n", size_of(gvar->type)); // 0で初期化された領域を確保する
        } else {
            error("未対応の型のグローバル変数はサポートしていません");
        }
    }


    printf(".text\n");
    printf(".global main\n");

    // もしかしたら、早く終わりすぎるかも、
    // （グローバル変数の定義をしたときに現状Nodeをヌルにするようにしてあるので、そこで終わっちゃうんだよね）
    for (int i=0; i<100; i++) {
    // for (int i=0; code[i]; i++) {
        gen(code[i]);
        // printf("  pop rax\n"); //　exprの評価結果として1つ値が残っているので、消しておく。
        // WARN: exprじゃないノードの時にバグる説
        // 何もスタックに積まない処理もある、ちゃんと図とかを書いて理解した方がよさそう
        // add, sub, mul, div, eq, neq, lt, lte
        int exprs[] = { ND_ADD, ND_SUB, ND_MUL, ND_DIV, ND_EQ, ND_NEQ, ND_LT, ND_LTE };
        for (int j=0; j < sizeof(exprs)/sizeof(exprs[0]); j++ ) {
            if (code[i] && exprs[j] == code[i]->kind) {
                printf("  pop rax\n");
                break;
            }
        }
    }



    printf("\n");
    printf(".section .note.GNU-stack,\"\",@progbits\n"); // スタックは実行であるとリンカに伝える
    return 0;
}
