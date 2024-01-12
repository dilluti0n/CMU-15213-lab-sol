# Lab2 BombLab

The original tar file is `bomblab.tar` and the writeup is
`bomblab.pdf`. My solution is in `bomb/defuse_code.txt`.

### Notes
Inside the directory `bomb`, `bomb` is the binary that you need to defuse by code. If you enter wrong input within the process, `bomb` will `explode`....... you can enter input to the process by `stdin` or using a file redirection like `./bomb defuse_code.txt`.

- `strings.txt` and `symt.txt`

string literal and symbol table included in the binary, respectivaly. but I never used it through solving the lab.

- `disasm.s` is made from `objdump -s ./bomb > disasm.s`

it contains a few comments I wrote while solving the problem, but I stopped writing comments after learning how to use `gdb`.

- When I learned how to use gdb and what exactly the %rsp register does, I realized the necessity of using paper and pen to solve the problems. Diagramming the stack of each `phase_n` was quite helpful in understanding the structure of the process, and probably unnecessary for someone more familiar with `gdb` or other debuggers, as this function is provided by the debugger.
- I even printed disasm.s on A4 paper to review it, which was quite helpful for solving the problems, but probably not what the creator of this lab intended.
- And for everyone who is solving this problem, I recommend that you solve it with an x86-64 cheat sheet, like [this](https://web.stanford.edu/class/cs107/resources/x86-64-reference.pdf), on your desk. The same goes for gdb; printing out it was very helpful to me, who is not familiar with the instructions and register names.

### Solution
SPOILER ALERT: People who want to solve this lab wouldn't want to see the content beyond this point, as it literally spoils everything.

- about `bomb.c`

This provides you with a brief structure of `main`. Each `phase_n` receives the `input` as its argument, which is the return value of the function `read_line`, as its first argument. This means that in every block `phase_n`, `%rdi` contains the memory adress of the each first character of lines you entered. Often, `%rdi` might not be visible in the `phase_n` block, which is because `input` is directly passed as the first argument to functions like `read_six_numbers` or `strings_not_equal`.

- phase_1

```asm
0000000000400ee0 <phase_1>:
  400ee0: 48 83 ec 08                  	subq	$8, %rsp
  400ee4: be 00 24 40 00               	movl	$4203520, %esi          # imm = 0x402400
  400ee9: e8 4a 04 00 00               	callq	0x401338 <strings_not_equal>
  400eee: 85 c0                        	testl	%eax, %eax
  400ef0: 74 05                        	je	0x400ef7 <phase_1+0x17>
  400ef2: e8 43 05 00 00               	callq	0x40143a <explode_bomb>
  400ef7: 48 83 c4 08                  	addq	$8, %rsp
  400efb: c3                           	retq
```

As mentioned earlier, `%rdi` is passed directly to `strings_not_equal`, so it's not explicitly shown in the code. `strings_not_equal` compares the string stored in `(%rdi)` (i.e., `input`) with the one in `(%rsi)`, and returns 0 if they are the same. In this case, `4203520` is stored to `%rsi`, from which we know it's the memory address where the string literal for comparison is stored. By `(gdb) x/s 4203520`, you can find out what the string is, which is "Border relations with Canada have never been better." And this is the defuse code for `phase_1`.

- phase_2

I wrote pseudocode for that phase. Although this code won't compile into an identical binary, it creates a program that does the same thing logically.

```c
int read_six_numbers(char *input, int *a)
{
    int num = 0;
    num = sscanf(input, "%d %d %d %d %d %d", a, a + 1, a + 2, a + 3, a + 4, a + 5);
    if (num > 5)
        return num;
    else
        explode_bomb();
}

void phase_2(char *input)
{
    int a[6], ret;
    read_six_numbers(input, a);
    if (*a == 1) {
        int *i = a + 1;
        int *end = a + 6;
        do {
            ret = *(i-1);
            ret += ret;
            if (ret == *i)
                continue;
            explode_bomb();
        } while (i++ == end);
    } else {
        explode_bomb();
    }
}
```

When encountering the declaration of an array like `int a[6];` and dealing with more than 7 function arguments(`sscanf` in `read_six_numbers`), it's important to examine how the compiler uses the stack to compile these into binary.


The declaration `int a[6];` is replaced by `subq $40, %rsp` in the compiled binary, which allocates 40 bytes of space on the stack for the array. When calling `sscanf`, the two excess variables, are stored sequentially from the top of the stack.


there are other algorithms involved, but for those, refer to the provided pseudocode. According to this, the defuse code for `phase_2` is `1 2 4 8 16 32`. Just by looking at this sequence, you should be able to guess what the algorithm is.

- phase_3

```asm
0000000000400f43 <phase_3>:
  400f43: 48 83 ec 18                  	subq	$24, %rsp
  400f47: 48 8d 4c 24 0c               	leaq	12(%rsp), %rcx
  400f4c: 48 8d 54 24 08               	leaq	8(%rsp), %rdx
  400f51: be cf 25 40 00               	movl	$4203983, %esi          # imm = 0x4025CF
  400f56: b8 00 00 00 00               	movl	$0, %eax
  400f5b: e8 90 fc ff ff               	callq	0x400bf0 <__isoc99_sscanf@plt>
  400f60: 83 f8 01                     	cmpl	$1, %eax
  400f63: 7f 05                        	jg	0x400f6a <phase_3+0x27>
  400f65: e8 d0 04 00 00               	callq	0x40143a <explode_bomb>
  400f6a: 83 7c 24 08 07               	cmpl	$7, 8(%rsp)
  400f6f: 77 3c                        	ja	0x400fad <phase_3+0x6a>
  400f71: 8b 44 24 08                  	movl	8(%rsp), %eax
  400f75: ff 24 c5 70 24 40 00         	jmpq	*4203632(,%rax,8)
  ...
```

`%esi` 에 할당한 상수 `$4203983` 은 `sscanf` 의 두 번째 argument, 즉 `format string` 일 것이다. `(gdb) x/s 4203983` 을 통해 그것이 "%d %d" 임을 알 수 있다. 즉 줄 `0x400f5b` 까지의 instruction 은 메모리 주소 8(%rsp) 에 첫 번째 입력(A)을, 12(%rsp) 에 두 번째 입력(B)을 적재한다. 이후에 일련의 비교를 통해, 입력된 숫자가 두개 보다 적거나 첫 번째 입력이 `0`에서 `7` 사이의 숫자가 아니라면 폭탄을 터뜨린다. `jmpq *4203632(,%rax,8)` 은 `Mem[4203632 + A * 8]` 에 적재되어 있는 값의 주소의 instruction 으로 `jump` 하라는 말이다. `A` 는 0부터 7까지의 값이므로, `(gdb) x/8gx 4203632` 를 통해 해당 주소로부터 적재되어 있는 연속된 8개의 값을 찾자.

```
0x402470:       0x0000000000400f7c      0x0000000000400fb9
0x402480:       0x0000000000400f83      0x0000000000400f8a
0x402490:       0x0000000000400f91      0x0000000000400f98
0x4024a0:       0x0000000000400f9f      0x0000000000400fa6
```

0부터 7까지의 `A` 애 대해 해당되는 위치로 `jump` 한다. 해당되는 위치로 가보면 각각

```asm
...
    movl  $xxx, %eax
    jmp   L1
...
L1: 
    cmpl  12(%rsp), %eax
    je    L2
    callq <explode_bomb>
L2:
    addq $24, %rsp
    ret
```

와 같은 구조를 볼 수 있다. 즉 `0` 부터 `7` 중에 하나 고른 `A` 에 대해 해당되는 위치에서 `%eax` 에 적재한 값(`$xxx`) 을 `B` 로 설정한게 우리의 defuse code 이다. 가능한 정답은 `A` 가 `0` 일때부터 `7` 일때까지, 8가지 일 것이다.


- phase_4

`phase_4` 자체는 매우 단순하다.

두 수를 입력받아 각각 `8(%rsp)`(A 라고 하자.), `12(%rsp)`(B 라고 하자.) 에 적재하고, `A` 가 0부터 14까지의 값이 아니라면 폭탄을 터뜨린다. 만약 `A` 가 0 부터 14까지의 값이라면 `func4(A, 0, 14)` 를 호출하며, 그 리턴값이 0이 아니라면 또 폭탄을 터뜨린다. `B` 는 `0` 이 아니라면 폭탄을 터뜨린다.


`func4` 가 꽤 복잡한 알고리즘을 가지고 있어서, pesudocode 를 만들었다.

```c
int func4(int x, int y, int z)
{
    int tmp = (z-y)+ (unsigned) (z-y) >> 31; /* logical shift */
    tmp >>= 1; /* arith shift */
    int ecx = tmp + y * 1;
    if (x < ecx) {
        tmp = func4(x, y, ecx - 1);
        tmp += tmp;
    } else {
        tmp = 0;
        if (x > ecx) {
            tmp = func4(x, ecx + 1, z);
            tmp = tmp + tmp + 1;
        }
    }
    return tmp;
}
```

만약에 `func4` 가 0이 아닌 값을 리턴해야 했다면 `A` 를 찾는 것은 매우 어려웠겠지만, 이 경우 `ecx` 와 `x` 가 같기만 하면 `func4` 는 0을 리턴하고, `y` 는 0, `z` 는 `14` 로 고정되어 있으므로 `A` 가 `14 >> 1 + 0`, 즉 7일 때 폭탄이 터지지 않는다.
