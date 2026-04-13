#include <stdio.h>
#include "9cc.h"

// （後からの解釈にはなるが、）スタックトップに変数のアドレスを積むことをやっている
void gen_lval(Node *node) {
    if (node->kind != ND_LVAR)
        error("左辺値が変数ではないのでだめ");

    printf("  mov rax, rbp\n");
    printf("  sub rax, %d\n", node->offset);
    printf("  push rax\n");
}

void gen_func(Function *func) {
    printf("%.*s:\n", func->len, func->name);

    // // プロローグ
    // // 変数a-z用のスタック領域を確保、26*8
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    // printf("  sub rsp, 208\n");
    // //  → 不要になると思われる

    gen(func->body);

    //エピローグ (すでにリターンが呼ばれていれば不要だが、一応追加)
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
}


void gen(Node *node) {
    // TODO: ND_FUNC_DEFもここでさばくようにしようかな
    if (node->kind == ND_LVAR_DEF) { 
        return;
    }

    if (node->kind == ND_RETURN) {
        gen(node->lhs);
        // 返り値がスタックに積まれるので、それをリターンしてプログラムを終了;
        printf("  pop rax\n");
        printf("  mov rsp, rbp\n");
        printf("  pop rbp\n");
        printf("  ret\n");
        return;
    }

    switch (node->kind) {
    case ND_NUM:
        printf("  push %d\n", node->val); // 最後にスタックに1つ返り値が乗っているという状況にするのね
        return;
    case ND_LVAR:
        gen_lval(node);
        printf("  pop rax\n");
        printf("  mov rax, [rax]\n");
        printf("  push rax\n");
        return;
    case ND_ASSIGN:
        gen_lval(node->lhs);
        gen(node->rhs);

//  上記2行によって、スタックには、左辺の変数などのアドレスと、右辺の評価結果が積まれている
        printf("  pop rdi\n");
        printf("  pop rax\n");
        printf("  mov [rax], rdi\n"); //　左辺のアドレスを使ってアドレッシングして、右辺の値を格納
        printf("  push rdi\n"); 
        // なぜpushするか？？教科書に解説あったっけな？ この式全体の値としては右辺の値になるべきなのかもです。　
        // C言語の仕様はそのようになってるんだっけ？　-> YES
        return;
    case ND_IF:
        gen(node->cond);
        printf("  pop rax\n");
        printf("  cmp rax, 0\n");
        printf("  je .Lelse2\n");
        gen(node->body);
        printf("  jmp .Lend2\n");
        printf(".Lelse2:\n");
        if (node->_else) {
            gen(node->_else);    
        }
        
        printf(".Lend2:\n");
        return;
    case ND_WHILE:
        printf(".Lbegin1:\n");
        gen(node->cond);
        printf("  pop rax\n");
        printf("  cmp rax, 0\n");
        printf("  je .Lend1\n");
        gen(node->body);
        printf("  jmp .Lbegin1\n");
        printf(".Lend1:\n");
        // printf("  push rax\n"); // 必要？
        return;
    case ND_FOR:
        if (node->init)
            gen(node->init);
        printf(".Lbegin3:\n");
        if (node->cond)
            gen(node->cond);
        printf("  pop rax\n");
        printf("  cmp rax, 0\n");
        printf("  je .Lend3\n");
        
        gen(node->body);
        if (node->inc)
            gen(node->inc);
        printf("  jmp .Lbegin3\n");
        
        printf(".Lend3:\n");
        return;
    case ND_BLOCK:
        for (int i=0; i<node->stmt_len; i++) {
            gen(node->stmt[i]);
        }
        return;
    case ND_FUNC_DEF:
        char *arg_regs[] = {"rax", "rcx", "rdx", "rsi", "rdi", "r8", "r9"};
        // 引数をスタック領域に移して、ローカル変数としてアクセスできるようにする
        // TODO: これはパフォーマンスの観点としては、よくなさそう、レジスタそのままのアクセスが早そう
        for (int i=0; i<node->func->arg_len; i++) {
            printf("  mov rax, rbp\n");
            printf("  sub rax, %d\n", i*8);
            printf("  push [rax], %s\n", arg_regs[i]);
        }

        gen_func(node->func);
        return;
    case ND_FUNC_CALL:
        Function* fn = node->func;
        if (node->arg_len > 6) {
            error("引数の数が多すぎます: %.*s\n", fn->len, fn->name);
        }

        // char *arg_regs[] = {"RAX", "RCX", "RDX", "RSI", "RDI", "R8", "R9"};
        // char *arg_regs[] = {"rax", "rcx", "rdx", "rsi", "rdi", "r8", "r9"};

        // 引数の準備 (第6まで)
        // スタックに退避する
        for (int i=0; i<fn->arg_len; i++) {
            printf("  push %s\n", arg_regs[i]);
        }
        for (int i=0; i<fn->arg_len; i++) {
            gen_lval(node->args[i]);
            printf("  pop %s\n", arg_regs[i]);
        }


        // TODO: ABI, align 16
        // if (rsp)
        printf("  call %.*s\n", fn->len, fn->name);

        return;
    case ND_ADDR:
        // スタックトップにアクセスしたい変数のアドレスを積むことをやる
        gen_lval(node->lhs);
        return;
    case ND_DEREF:
        gen(node->lhs);
        // スタックトップに変数のアドレスを積む
        printf("  pop rax\n");
        printf("  mov rax, [rax]\n");
        printf("  push rax\n");
        // スタックトップに変数のアドレスがさす値を積む
        return;
    }
    

    gen(node->lhs);
    gen(node->rhs);

    // やりたかった演算をやるよー
    printf("  pop rdi\n");
    printf("  pop rax\n");

    switch (node->kind) {
    case ND_ADD:
        printf("  add rax, rdi\n");
        break;
    case ND_SUB:
        printf("  sub rax, rdi\n");
        break;
    case ND_MUL:
        printf("  imul rax, rdi\n");
        break;
    case ND_DIV:
        printf("  cqo\n");
        printf("  idiv rdi\n");
        break;
    case ND_EQ:
        printf("  cmp rax, rdi\n");
        printf("  sete al\n"); // seteは8ビットレジスタしか受け取れない
        printf("  movzb rax, al\n"); // 上位56ビットをゼロクリアしながら、raxを更新
        break;
    case ND_NEQ:
        printf("  cmp rax, rdi\n");
        printf("  setne al\n");
        printf("  movzb rax, al\n");
        break;
    case ND_LT:
        printf("  cmp rax, rdi\n");
        printf("  setl al\n");
        printf("  movzb rax, al\n");
        break;
    case ND_LTE:
        printf("  cmp rax, rdi\n");
        printf("  setle al\n");
        printf("  movzb rax, al\n");
        break;
    // case ND_GT:
    // case ND_GTE: これらはパーサー側で処理するよ
    }
    printf("  push rax\n");
}
