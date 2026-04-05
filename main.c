#include <stdio.h>
#include "9cc.h"

Node *code[100];
Token *token; // 現在のtoken pointer
char *user_input; // ユーザの入力したプログラム
LVar *locals;

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
    printf(".global main\n");
    printf("main:\n");

    // プロローグ
    // 変数a-z用のスタック領域を確保、26*8
    // printf("  push rbp\n");
    // printf("  mov rbp, rsp\n");
    // printf("  sub rsp, 208\n");
    //  → 不要になると思われる


    for (int i=0; code[i]; i++) {
        gen(code[i]);
        printf("  pop rax\n"); //　exprの評価結果として1つ値が残っているので、消しておく。
    }

    //エピローグ
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");

    printf("\n");
    printf(".section .note.GNU-stack,\"\",@progbits\n"); // スタックは実行であるとリンカに伝える
    return 0;
}
