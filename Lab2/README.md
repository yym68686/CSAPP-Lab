## GDB 常用命令

| 命令                  | 功能                                           |
| --------------------- | ---------------------------------------------- |
| gdb filename          | 开始调试                                       |
| run                   | 开始运行/运行后重新运行                        |
| run 1 2 3             | 开始运行,并且传入参数1，2，3                   |
| kill                  | 停止运行                                       |
| quit                  | 退出gdb                                        |
| break sum             | 在sum函数的开头设置断点                        |
| break *0x8048c3       | 在0x8048c3的地址处设置断点                     |
| delete 1              | 删除断点1                                      |
| clear sum             | 删除在sum函数入口的断点                        |
| stepi                 | 运行一条指令                                   |
| stepi 4               | 运行4条指令                                    |
| continue              | 运行到下一个断点                               |
| disas sum             | 反汇编sum函数                                  |
| disas 0X12345         | 反汇编入口在0x12345的函数                      |
| print /x /d /t $rax   | 将rax里的内容以16进制，10进制，2进制的形式输出 |
| print 0x888           | 输出0x888的十进制形式                          |
| print (int)0x123456   | 将0x123456地址所存储的内容以数字形式输出       |
| print (char*)0x123456 | 输出存储在0x123456的字符串                     |
| x/w $rsp              | 解析在rsp所指向位置的word                      |
| x/2w $rsp             | 解析在rsp所指向位置的两个word                  |
| x/2wd $rsp            | 解析在rsp所指向位置的word，以十进制形式输出    |
| info registers        | 寄存器信息                                     |
| info functions        | 函数信息                                       |
| info stack            | 栈信息                                         |

## **1. phase_1**

显示汇编代码：

```bash
disas phase_1
```

汇编代码：

```go
Dump of assembler code for function phase_1:
   0x00001662 <+0>:     endbr32
   0x00001666 <+4>:     push   %ebp
   0x00001667 <+5>:     mov    %esp,%ebp
   0x00001669 <+7>:     push   %ebx
   0x0000166a <+8>:     sub    $0xc,%esp
   0x0000166d <+11>:    call   0x13d0 <__x86.get_pc_thunk.bx>
   0x00001672 <+16>:    add    $0x38f2,%ebx
   0x00001678 <+22>:    lea    -0x1e20(%ebx),%eax
   0x0000167e <+28>:    push   %eax
   0x0000167f <+29>:    pushl  0x8(%ebp)
   0x00001682 <+32>:    call   0x1c10 <strings_not_equal>
   0x00001687 <+37>:    add    $0x10,%esp
   0x0000168a <+40>:    test   %eax,%eax
   0x0000168c <+42>:    jne    0x1693 <phase_1+49>
   0x0000168e <+44>:    mov    -0x4(%ebp),%ebx
   0x00001691 <+47>:    leave
   0x00001692 <+48>:    ret
   0x00001693 <+49>:    call   0x1e95 <explode_bomb>
   0x00001698 <+54>:    jmp    0x168e <phase_1+44>
End of assembler dump.
```

- 如果系统的CPU版本比较新(如在十代酷睿), 在Ubuntu20.04上会默认编译出`endbr32`指令。请参考[这里](https://github.com/NJU-ProjectN/abstract-machine/commit/0f6f91dee305ad0b143bf13becc2882eda4d977a)添加相应的编译选项来去掉这条新指令。不过这里没有源代码所以没有办法重新编译。

调用了strings_not_equal函数直接判断两个字符串是否相等，判断用户输入的字符串是否跟系统预设的相等，首先要找到data段里面的内置字符串，需要知道字符串的地址，然后打印出来。首先字符串肯定不是存在栈里面的，所以涉及栈操作的指令一定跟那个字符串无关，根据汇编大概可以知道下面这条指令：

```go
0x00001678 <+22>:    lea    -0x1e20(%ebx),%eax
```

把字符串地址赋值给了eax，所以我们要知道`%ebx-0x1e20`处的地址是什么。要知道ebx是什么，需要先运行到这里先在主函数处设置断点`b main`，按r运行后，再使用`disas phase_1`输出汇编代码：

```go
Dump of assembler code for function phase_1:
   0x00401662 <+0>:     endbr32
   0x00401666 <+4>:     push   %ebp
   0x00401667 <+5>:     mov    %esp,%ebp
   0x00401669 <+7>:     push   %ebx
   0x0040166a <+8>:     sub    $0xc,%esp
   0x0040166d <+11>:    call   0x4013d0 <__x86.get_pc_thunk.bx>
   0x00401672 <+16>:    add    $0x38f2,%ebx
   0x00401678 <+22>:    lea    -0x1e20(%ebx),%eax
   0x0040167e <+28>:    push   %eax
   0x0040167f <+29>:    pushl  0x8(%ebp)
   0x00401682 <+32>:    call   0x401c10 <strings_not_equal>
   0x00401687 <+37>:    add    $0x10,%esp
   0x0040168a <+40>:    test   %eax,%eax
   0x0040168c <+42>:    jne    0x401693 <phase_1+49>
   0x0040168e <+44>:    mov    -0x4(%ebp),%ebx
   0x00401691 <+47>:    leave
   0x00401692 <+48>:    ret
   0x00401693 <+49>:    call   0x401e95 <explode_bomb>
   0x00401698 <+54>:    jmp    0x40168e <phase_1+44>
End of assembler dump.
```

发现地址有所变化，以此时的地址为准。查看`%ebx-0x1e20`处的值：

```bash
x/x $ebx-0x1e20
```

输出：

```go
0x403144:       0x69206548
```

所以内置字符串的地址就是`0x403144`，输出字符串：

```bash
x/s 0x403144
```

回显：

```gherkin
0x403144:       "He is evil and fits easily into most overhead storage bins."
```

所以第一题的答案就是`He is evil and fits easily into most overhead storage bins.`

## **2. phase_2**

先查看汇编代码`disas phase_2`：

```go
Dump of assembler code for function phase_2:
   0x0040169a <+0>:     endbr32
   0x0040169e <+4>:     push   %ebp
   0x0040169f <+5>:     mov    %esp,%ebp
   0x004016a1 <+7>:     push   %edi
   0x004016a2 <+8>:     push   %esi
   0x004016a3 <+9>:     push   %ebx
   0x004016a4 <+10>:    sub    $0x34,%esp
   0x004016a7 <+13>:    call   0x4013d0 <__x86.get_pc_thunk.bx>
   0x004016ac <+18>:    add    $0x38b8,%ebx
   0x004016b2 <+24>:    mov    %gs:0x14,%eax
   0x004016b8 <+30>:    mov    %eax,-0x1c(%ebp)
   0x004016bb <+33>:    xor    %eax,%eax
   0x004016bd <+35>:    lea    -0x34(%ebp),%eax
   0x004016c0 <+38>:    push   %eax
   0x004016c1 <+39>:    pushl  0x8(%ebp)
   0x004016c4 <+42>:    call   0x401eeb <read_six_numbers> ; 读取六个数
   0x004016c9 <+47>:    add    $0x10,%esp
   0x004016cc <+50>:    cmpl   $0x0,-0x34(%ebp)      ; 判断输入的第一个数是否小于零
   0x004016d0 <+54>:    js     0x4016dc <phase_2+66> ; 如果小于零就爆炸
   0x004016d2 <+56>:    mov    $0x1,%esi             ; for 循环迭代器 i 赋初值为 1
   0x004016d7 <+61>:    lea    -0x34(%ebp),%edi      ; 读取第一个数字的地址到寄存器edi
   0x004016da <+64>:    jmp    0x4016f0 <phase_2+86> ; 跳转到for循环开始
   0x004016dc <+66>:    call   0x401e95 <explode_bomb>
   0x004016e1 <+71>:    jmp    0x4016d2 <phase_2+56>
   0x004016e3 <+73>:    call   0x401e95 <explode_bomb>
   0x004016e8 <+78>:    add    $0x1,%esi
   0x004016eb <+81>:    cmp    $0x6,%esi
   0x004016ee <+84>:    je     0x4016fd <phase_2+99>
   0x004016f0 <+86>:    mov    %esi,%eax              ; eax存放i的值，初值为一
   0x004016f2 <+88>:    add    -0x4(%edi,%esi,4),%eax ; eax = eax + (edi + esi * 4 - 0x4)
   0x004016f6 <+92>:    cmp    %eax,(%edi,%esi,4)     ;判断array[i - 1] + i与array[i]的大小关系
   0x004016f9 <+95>:    je     0x4016e8 <phase_2+78>  ;若相等则继续for循环
   0x004016fb <+97>:    jmp    0x4016e3 <phase_2+73>  ; 若不等，则爆炸
   0x004016fd <+99>:    mov    -0x1c(%ebp),%eax       ;把第六个数字放到eax中，数组范围%edp-0x34~%edp-0x1c
   0x00401700 <+102>:   xor    %gs:0x14,%eax
   0x00401707 <+109>:   jne    0x401711 <phase_2+119>
   0x00401709 <+111>:   lea    -0xc(%ebp),%esp
   0x0040170c <+114>:   pop    %ebx
   0x0040170d <+115>:   pop    %esi
   0x0040170e <+116>:   pop    %edi
   0x0040170f <+117>:   pop    %ebp
   0x00401710 <+118>:   ret
   0x00401711 <+119>:   call   0x402e30 <__stack_chk_fail_local>
End of assembler dump.
```

注意到：

```go
0x004016f2 <+88>:    add    -0x4(%edi,%esi,4),%eax
```

这句话的意思是`eax = eax + (edi + esi * 4 - 0x4)`，翻译一下就是`sum = array[i - 1] + i`。因此我们需要构造一个当前数组下标加前一个愿元素等于当前数组元素的数组：

```
    0      1      2      3      4      5
┌──────┬──────┬──────┬──────┬──────┬──────┐
│   0  │   1  │   3  │   6  │  10  │  15  │
└──────┴──────┴──────┴──────┴──────┴──────┘
```

所以目前的答案是：

```
He is evil and fits easily into most overhead storage bins.
0 1 3 6 10 15
```

## **3. phase_3**

先看汇编代码`disas phase_3`：

```go
Dump of assembler code for function phase_3:
   0x00401716 <+0>:     endbr32
   0x0040171a <+4>:     push   %ebp
   0x0040171b <+5>:     mov    %esp,%ebp
   0x0040171d <+7>:     push   %ebx
   0x0040171e <+8>:     sub    $0x14,%esp
   0x00401721 <+11>:    call   0x4013d0 <__x86.get_pc_thunk.bx>
   0x00401726 <+16>:    add    $0x383e,%ebx
   0x0040172c <+22>:    mov    %gs:0x14,%eax
   0x00401732 <+28>:    mov    %eax,-0xc(%ebp)
   0x00401735 <+31>:    xor    %eax,%eax
   0x00401737 <+33>:    lea    -0x10(%ebp),%eax
   0x0040173a <+36>:    push   %eax
   0x0040173b <+37>:    lea    -0x14(%ebp),%eax
   0x0040173e <+40>:    push   %eax
   0x0040173f <+41>:    lea    -0x1b97(%ebx),%eax
   0x00401745 <+47>:    push   %eax
   0x00401746 <+48>:    pushl  0x8(%ebp)
   0x00401749 <+51>:    call   0x4012d0 <__isoc99_sscanf@plt> ; 需要输入一些东西
   0x0040174e <+56>:    add    $0x10,%esp
   0x00401751 <+59>:    cmp    $0x1,%eax              ; scanf的返回值即不能为空
   0x00401754 <+62>:    jle    0x40176f <phase_3+89>
   0x00401756 <+64>:    cmpl   $0x7,-0x14(%ebp)       ; 输入的第一个数字与7比较
   0x0040175a <+68>:    ja     0x4017ee <phase_3+216> ; 如果大于7则爆炸
   0x00401760 <+74>:    mov    -0x14(%ebp),%eax
   0x00401763 <+77>:    mov    %ebx,%edx
   0x00401765 <+79>:    add    -0x1da4(%ebx,%eax,4),%edx
   0x0040176c <+86>:    notrack jmp *%edx
   0x0040176f <+89>:    call   0x401e95 <explode_bomb>
   0x00401774 <+94>:    jmp    0x401756 <phase_3+64>
   0x00401776 <+96>:    mov    $0x12b,%eax             ; eax = 0x12b
   0x0040177b <+101>:   sub    $0xfc,%eax              ; eax = 0x12b - 0xfc = 0x2f
   0x00401780 <+106>:   add    $0x38,%eax              ; eax = 0x2f + 0x38 = 0x67
   0x00401783 <+109>:   sub    $0x230,%eax             ; eax = 0x67 - 0x230 = 0xfffffe37
   0x00401788 <+114>:   add    $0x230,%eax             ; 从 0x004017d2 跳转到这里 此时eax = 0
   0x0040178d <+119>:   sub    $0x230,%eax
   0x00401792 <+124>:   add    $0x230,%eax
   0x00401797 <+129>:   sub    $0x230,%eax             ; eax没有变依然是0
   0x0040179c <+134>:   cmpl   $0x5,-0x14(%ebp)        ; 输入的第一个数字与5比较
   0x004017a0 <+138>:   jg     0x4017a7 <phase_3+145>  ; 如果大于5则爆炸
   0x004017a2 <+140>:   cmp    %eax,-0x10(%ebp)        ; 比较第二个数字与eax，此时eax = 0
   0x004017a5 <+143>:   je     0x4017ac <phase_3+150>  ; 等于就通过
   0x004017a7 <+145>:   call   0x401e95 <explode_bomb> ; 不等于就爆炸
   0x004017ac <+150>:   mov    -0xc(%ebp),%eax ; 第二个数字存到eax
   0x004017af <+153>:   xor    %gs:0x14,%eax
   0x004017b6 <+160>:   jne    0x4017fa <phase_3+228>
   0x004017b8 <+162>:   mov    -0x4(%ebp),%ebx
   0x004017bb <+165>:   leave
   0x004017bc <+166>:   ret
   0x004017bd <+167>:   mov    $0x0,%eax
   0x004017c2 <+172>:   jmp    0x40177b <phase_3+101>
   0x004017c4 <+174>:   mov    $0x0,%eax
   0x004017c9 <+179>:   jmp    0x401780 <phase_3+106>
   0x004017cb <+181>:   mov    $0x0,%eax
   0x004017d0 <+186>:   jmp    0x401783 <phase_3+109>
   0x004017d2 <+188>:   mov    $0x0,%eax ; eax = 0
   0x004017d7 <+193>:   jmp    0x401788 <phase_3+114>
   0x004017d9 <+195>:   mov    $0x0,%eax
   0x004017de <+200>:   jmp    0x40178d <phase_3+119>
   0x004017e0 <+202>:   mov    $0x0,%eax
   0x004017e5 <+207>:   jmp    0x401792 <phase_3+124>
   0x004017e7 <+209>:   mov    $0x0,%eax
   0x004017ec <+214>:   jmp    0x401797 <phase_3+129>
   0x004017ee <+216>:   call   0x401e95 <explode_bomb>
   0x004017f3 <+221>:   mov    $0x0,%eax
   0x004017f8 <+226>:   jmp    0x40179c <phase_3+134>
   0x004017fa <+228>:   call   0x402e30 <__stack_chk_fail_local>
End of assembler dump.
```

发现：

```go
   0x00401737 <+33>:    lea    -0x10(%ebp),%eax
   0x0040173a <+36>:    push   %eax
   0x0040173b <+37>:    lea    -0x14(%ebp),%eax
   0x0040173e <+40>:    push   %eax
   0x0040173f <+41>:    lea    -0x1b97(%ebx),%eax
   0x00401745 <+47>:    push   %eax
   0x00401746 <+48>:    pushl  0x8(%ebp)
   0x00401749 <+51>:    call   0x4012d0 <__isoc99_sscanf@plt>
```

说明需要输入什么东西，很明显连续三个lea push应该是scanf的三个参数，说明需要输入两个变量，参数是从最后一个开始push的，所以`-0x1b97(%ebx)`是输入格式的字符串，尝试显示输入格式。先打断点：

```bash
b *0x00401745
```

运行后，查看输入格式字符串：

```bash
x/s $eax
```

显示：

```go
0x4033cd:       "%d %d"
```

说明我们要输入两个数字，接下来看这两个数字需要满足什么要求。从汇编指令看得出来`-0x14(%ebp)`就是我们输入的第一个数字必须小于等于5，可以取的数有`0 1 2 3 4 5`，`-0x10(%ebp)`是第二个数字。第二个数字需要在0x004017a2与eax进行比较，相等就不会爆炸，将程序运行到这里，发现eax在0x004017d2被赋值为0，跳转到0x00401788，最后eax还是0，所以第二个数字就是零。

## **4. phase_4**

先看汇编代码：

```go
Dump of assembler code for function phase_4:
   0x0040185e <+0>:     endbr32
   0x00401862 <+4>:     push   %ebp
   0x00401863 <+5>:     mov    %esp,%ebp
   0x00401865 <+7>:     push   %ebx
   0x00401866 <+8>:     sub    $0x14,%esp
   0x00401869 <+11>:    call   0x4013d0 <__x86.get_pc_thunk.bx>
   0x0040186e <+16>:    add    $0x36f6,%ebx
   0x00401874 <+22>:    mov    %gs:0x14,%eax
   0x0040187a <+28>:    mov    %eax,-0xc(%ebp)
   0x0040187d <+31>:    xor    %eax,%eax
   0x0040187f <+33>:    lea    -0x10(%ebp),%eax
   0x00401882 <+36>:    push   %eax
   0x00401883 <+37>:    lea    -0x14(%ebp),%eax
   0x00401886 <+40>:    push   %eax
   0x00401887 <+41>:    lea    -0x1b97(%ebx),%eax
   0x0040188d <+47>:    push   %eax
   0x0040188e <+48>:    pushl  0x8(%ebp)
   0x00401891 <+51>:    call   0x4012d0 <__isoc99_sscanf@plt>
   0x00401896 <+56>:    add    $0x10,%esp
   0x00401899 <+59>:    cmp    $0x2,%eax ; 必须输入两个参数，否则爆炸
   0x0040189c <+62>:    jne    0x4018a4 <phase_4+70>
   0x0040189e <+64>:    cmpl   $0xe,-0x14(%ebp) ; 第一个数字必须小于等于0xe
   0x004018a2 <+68>:    jbe    0x4018a9 <phase_4+75>
   0x004018a4 <+70>:    call   0x401e95 <explode_bomb>
   0x004018a9 <+75>:    sub    $0x4,%esp
   0x004018ac <+78>:    push   $0xe        ;func4第三个参数14
   0x004018ae <+80>:    push   $0x0        ;func4第二个参数0
   0x004018b0 <+82>:    pushl  -0x14(%ebp) ;func4第一个参数是我们输入的第一个数字
   0x004018b3 <+85>:    call   0x4017ff <func4> ;调用func4
   0x004018b8 <+90>:    add    $0x10,%esp
   0x004018bb <+93>:    or     -0x10(%ebp),%eax ; 比较第二个数字与返回值 eax 的大小
   0x004018be <+96>:    je     0x4018c5 <phase_4+103> ; eax = 第二个数字 = 0 则不爆炸
   0x004018c0 <+98>:    call   0x401e95 <explode_bomb>
   0x004018c5 <+103>:   mov    -0xc(%ebp),%eax
   0x004018c8 <+106>:   xor    %gs:0x14,%eax
   0x004018cf <+113>:   jne    0x4018d6 <phase_4+120>
   0x004018d1 <+115>:   mov    -0x4(%ebp),%ebx
   0x004018d4 <+118>:   leave
   0x004018d5 <+119>:   ret
   0x004018d6 <+120>:   call   0x402e30 <__stack_chk_fail_local>
End of assembler dump.
```

跟上一题一样，依旧是输入两个数字：

```go
   0x0040187f <+33>:    lea    -0x10(%ebp),%eax   ; 第二个数字
   0x00401882 <+36>:    push   %eax
   0x00401883 <+37>:    lea    -0x14(%ebp),%eax   ; 第一个数字
   0x00401886 <+40>:    push   %eax
   0x00401887 <+41>:    lea    -0x1b97(%ebx),%eax ; 输入格式
   0x0040188d <+47>:    push   %eax
   0x0040188e <+48>:    pushl  0x8(%ebp)
   0x00401891 <+51>:    call   0x4012d0 <__isoc99_sscanf@plt>
```

查看输入格式，先打断点：

```bash
b *0x0040188d
```

运行后，查看输入格式字符串：

```bash
x/s $eax
```

显示：

```go
0x4033cd:       "%d %d"
```

说明我们要输入两个数字，接下来看这两个数字需要满足什么要求。从汇编指令看得出来`-0x14(%ebp)`就是我们输入的第一个数字必须小于等于0xe，`-0x10(%ebp)`是第二个数字，通过：

```go
0x004018be <+96>:    je     0x4018c5 <phase_4+103> ; eax = 第二个数字 = 0 则不爆炸
```

可知，所以第二个数字必须等于零。接下来判断怎么让第一个数字经过func4之后返回值为0。func4的汇编代码：

```go
Dump of assembler code for function func4:
   0x004017ff <+0>:     endbr32
   0x00401803 <+4>:     push   %ebp
   0x00401804 <+5>:     mov    %esp,%ebp
   0x00401806 <+7>:     push   %esi
   0x00401807 <+8>:     push   %ebx
   0x00401808 <+9>:     mov    0x8(%ebp),%ecx  ; ebp+0x8是第一个参数a1
   0x0040180b <+12>:    mov    0xc(%ebp),%eax  ; ebp+0xc是第二个参数a2
   0x0040180e <+15>:    mov    0x10(%ebp),%ebx ; ebp+0x10是第三个参数a3
   0x00401811 <+18>:    mov    %ebx,%esi  ; esi = a3
   0x00401813 <+20>:    sub    %eax,%esi  ; esi = a3 - a2
   0x00401815 <+22>:    mov    %esi,%edx  ; edx = a3 - a2
   0x00401817 <+24>:    shr    $0x1f,%edx ; 逻辑右移31位 edx = (a3 - a2) >> 31，只保留符号位
   0x0040181a <+27>:    add    %esi,%edx
   0x0040181c <+29>:    sar    %edx       ;算术右移1位，即edx / 2 = (a3 - a2) / 2
   0x0040181e <+31>:    add    %eax,%edx  ; 中间值 mid = edx = a2 + (a3 - a2) / 2
   0x00401820 <+33>:    cmp    %ecx,%edx  ; 比较 a2 + (a3 - a2) / 2 与 a1 的大小
   0x00401822 <+35>:    jg     0x401832 <func4+51> ; 若 a2 + (a3 - a2) / 2 > a1
   0x00401824 <+37>:    mov    $0x0,%eax           ; eax = 0，当 mid == a1 时，直接返回0
   0x00401829 <+42>:    jl     0x401847 <func4+72> ; 若 a2 + (a3 - a2) / 2 < a1
   0x0040182b <+44>:    lea    -0x8(%ebp),%esp
   0x0040182e <+47>:    pop    %ebx
   0x0040182f <+48>:    pop    %esi
   0x00401830 <+49>:    pop    %ebp
   0x00401831 <+50>:    ret
   0x00401832 <+51>:    sub    $0x4,%esp ; 此时 a2 + (a3 - a2) / 2 > a1
   0x00401835 <+54>:    sub    $0x1,%edx ; mid - 1
   0x00401838 <+57>:    push   %edx      ;func4第三个参数mid - 1
   0x00401839 <+58>:    push   %eax      ;func4第二个参数a2
   0x0040183a <+59>:    push   %ecx      ;func4第一个参数a1
   0x0040183b <+60>:    call   0x4017ff <func4> ; func4(a1, a2, mid - 1)
   0x00401840 <+65>:    add    $0x10,%esp
   0x00401843 <+68>:    add    %eax,%eax ; eax = eax * 2
   0x00401845 <+70>:    jmp    0x40182b <func4+44>
   0x00401847 <+72>:    sub    $0x4,%esp ; 此时 a2 + (a3 - a2) / 2 < a1
   0x0040184a <+75>:    push   %ebx      ;func4第三个参数a3
   0x0040184b <+76>:    add    $0x1,%edx
   0x0040184e <+79>:    push   %edx      ;func4第二个参数mid + 1
   0x0040184f <+80>:    push   %ecx      ;func4第一个参数a1
   0x00401850 <+81>:    call   0x4017ff <func4>      ; func4(a1, mid + 1, a3)
   0x00401855 <+86>:    add    $0x10,%esp
   0x00401858 <+89>:    lea    0x1(%eax,%eax,1),%eax ; eax = eax * 2 + 1
   0x0040185c <+93>:    jmp    0x40182b <func4+44>
End of assembler dump.
```

这个函数是一个二分函数。a3 加 a2 有可能超过范围，所以计算两个数的中间值可以使用`a2 + (a3 - a2) / 2`。

- 逻辑右移与算术右移：逻辑右移就是不考虑符号位，右移一位，左边补零即可。算术右移需要考虑符号位，右移一位，若符号位为1，就在左边补1,；否则，就补0。所以算术右移也可以进行有符号位的除法，右移,n位就等于除2的n次方。

注意到：

```go
   0x00401822 <+35>:    jg     0x401832 <func4+51> ; 若 a2 + (a3 - a2) / 2 > a1
   0x00401824 <+37>:    mov    $0x0,%eax           ; eax = 0，当 mid == a1 时，直接返回0
   0x00401829 <+42>:    jl     0x401847 <func4+72> ; 若 a2 + (a3 - a2) / 2 < a1
```

只要不满足`jg`和`jl`的条件，func4就可以直接返回0，只有`mid == a1`时，func4直接返回，而第一次`mid = 7`，所以第一个数应该是7。本题答案为：`7 0`

## **5. phase_5**

先看汇编代码：

```go
Dump of assembler code for function phase_5:
   0x004018db <+0>:     endbr32
   0x004018df <+4>:     push   %ebp
   0x004018e0 <+5>:     mov    %esp,%ebp
   0x004018e2 <+7>:     push   %edi
   0x004018e3 <+8>:     push   %esi
   0x004018e4 <+9>:     push   %ebx
   0x004018e5 <+10>:    sub    $0x18,%esp
   0x004018e8 <+13>:    call   0x4013d0 <__x86.get_pc_thunk.bx>
   0x004018ed <+18>:    add    $0x3677,%ebx
   0x004018f3 <+24>:    mov    0x8(%ebp),%esi ; ebp+0x8 是字符串的开始字节
   0x004018f6 <+27>:    push   %esi
   0x004018f7 <+28>:    call   0x401bea <string_length>
   0x004018fc <+33>:    add    $0x10,%esp
   0x004018ff <+36>:    cmp    $0x6,%eax ; 字符串长度必须等于6
   0x00401902 <+39>:    jne    0x401931 <phase_5+86>
   0x00401904 <+41>:    mov    %esi,%eax          ; eax = esi 字符串开始字节
   0x00401906 <+43>:    add    $0x6,%esi          ; esi + esi + 6 字符串结束字节
   0x00401909 <+46>:    mov    $0x0,%ecx          ; ecx = 0，记录结果
   0x0040190e <+51>:    lea    -0x1d84(%ebx),%edi ; 将数组起始地址放到 edi 中
   0x00401914 <+57>:    movzbl (%eax),%edx        ; 32 位 0 扩展
   0x00401917 <+60>:    and    $0xf,%edx          ; 取字符的asscii码的第四位作为数组的索引edx
   0x0040191a <+63>:    add    (%edi,%edx,4),%ecx ; ecx  = ecx + edi[edx]
   0x0040191d <+66>:    add    $0x1,%eax          ; 计数器，记录当前在第几个字符
   0x00401920 <+69>:    cmp    %esi,%eax
   0x00401922 <+71>:    jne    0x401914 <phase_5+57> ; 若 还没到末尾，则继续循环
   0x00401924 <+73>:    cmp    $0x2f,%ecx ; ecx 与 47 比较
   0x00401927 <+76>:    jne    0x401938 <phase_5+93> ; ecx 必须等于 47
   0x00401929 <+78>:    lea    -0xc(%ebp),%esp
   0x0040192c <+81>:    pop    %ebx
   0x0040192d <+82>:    pop    %esi
   0x0040192e <+83>:    pop    %edi
   0x0040192f <+84>:    pop    %ebp
   0x00401930 <+85>:    ret
   0x00401931 <+86>:    call   0x401e95 <explode_bomb>
   0x00401936 <+91>:    jmp    0x401904 <phase_5+41>
   0x00401938 <+93>:    call   0x401e95 <explode_bomb>
   0x0040193d <+98>:    jmp    0x401929 <phase_5+78>
End of assembler dump.
```

注意到：

```go
0x0040190e <+51>:    lea    -0x1d84(%ebx),%edi ; 将数组起始地址放到 edi 中
```

看看`-0x1d84(%ebx)`是啥，添加断点：`b *0x0040190e`，输出`-0x1d84(%ebx)`内容：

```bash
x/16x $ebx - 0x1d84
```

回显：

```go
0x4031e0 <array.3068>:     0x02    0x00    0x00    0x00    0x0a    0x00    0x00    0x00
0x4031e8 <array.3068+8>:   0x06    0x00    0x00    0x00    0x01    0x00    0x00    0x00
```

提示是数组，每四个字节一个元素，换一个格式输出：

```bash
x/6xg $ebx - 0x1d84
```

回显：

```go
0x4031e0 <array.3068>:          0x0000000a00000002      0x0000000100000006
0x4031f0 <array.3068+16>:       0x000000100000000c      0x0000000300000009
0x403200 <array.3068+32>:       0x0000000700000004      0x000000050000000e
```

我们的目标是需要找到一组字符序列，他们的asscii码的第四位形成的数组索引下的元素相加为47，发现这个数组前六个相加刚好是47，查询一下asscii码表，找到六个字符其低四位分别是`0 1 2 3 4 5`，我找的是`pqrstu`，当然还有其他答案。

## **6. phase_6**

先看汇编代码：

```go
Dump of assembler code for function phase_6:
   0x0040193f <+0>:     endbr32
   0x00401943 <+4>:     push   %ebp
   0x00401944 <+5>:     mov    %esp,%ebp
   0x00401946 <+7>:     push   %edi
   0x00401947 <+8>:     push   %esi
   0x00401948 <+9>:     push   %ebx
   0x00401949 <+10>:    sub    $0x64,%esp
   0x0040194c <+13>:    call   0x4013d0 <__x86.get_pc_thunk.bx>
   0x00401951 <+18>:    add    $0x3613,%ebx
   0x00401957 <+24>:    mov    %gs:0x14,%eax
   0x0040195d <+30>:    mov    %eax,-0x1c(%ebp)
   0x00401960 <+33>:    xor    %eax,%eax
   0x00401962 <+35>:    lea    -0x4c(%ebp),%eax
   0x00401965 <+38>:    push   %eax
   0x00401966 <+39>:    pushl  0x8(%ebp)
   0x00401969 <+42>:    call   0x401eeb <read_six_numbers> ; 读取六个数字
   0x0040196e <+47>:    lea    -0x48(%ebp),%eax            ; eax = array[1]
   0x00401971 <+50>:    mov    %eax,-0x64(%ebp)            ; ebp-0x64 = array[1]
   0x00401974 <+53>:    add    $0x10,%esp
   0x00401977 <+56>:    movl   $0x0,-0x60(%ebp)            ; v16 = ebp - 0x60 = 0
   0x0040197e <+63>:    lea    -0x34(%ebp),%eax            ; v19 = ebp - 0x34 = eax = array[2]
   0x00401981 <+66>:    mov    %eax,-0x5c(%ebp)            ; ebp - 0x5c = v19 = ebp - 0x34
   0x00401984 <+69>:    jmp    0x4019a7 <phase_6+104>

下面这一段是一个循环，伪代码：
 while ( 1 )
  {
    if ( (unsigned int)(*((_DWORD *)v15 - 1) - 1) > 5 )
      explode_bomb();
    if ( ++v16 > 5 )
      break;
    v1 = (int *)v15;
    do
    {
      if ( *((_DWORD *)v15 - 1) == *v1 )
        explode_bomb();
      ++v1;
    }
    while ( &v19 != v1 );
    v15 += 4;
  }

   0x00401986 <+71>:    call   0x401e95 <explode_bomb>
   0x0040198b <+76>:    jmp    0x4019ba <phase_6+123>
   0x0040198d <+78>:    call   0x401e95 <explode_bomb>
   0x00401992 <+83>:    add    $0x4,%esi              ; v1++ = esi + 0x4
   0x00401995 <+86>:    cmp    %esi,-0x5c(%ebp)       ;
   0x00401998 <+89>:    je     0x4019a3 <phase_6+100> ; while (v19 != v1)
   0x0040199a <+91>:    mov    (%esi),%eax            ; eax = *v15
   0x0040199c <+93>:    cmp    %eax,-0x4(%edi)
   0x0040199f <+96>:    jne    0x401992 <phase_6+83>
   0x004019a1 <+98>:    jmp    0x40198d <phase_6+78>
   0x004019a3 <+100>:   addl   $0x4,-0x64(%ebp)       ; v15 = v15 + 0x4 = *(ebp - 0x64) + 4
   0x004019a7 <+104>:   mov    -0x64(%ebp),%eax       ; eax = *(ebp - 0x64) = array[1]
   0x004019aa <+107>:   mov    %eax,%edi              ; edi = eax = *(ebp - 0x64) = array[1]
   0x004019ac <+109>:   mov    -0x4(%eax),%eax        ; eax = array[0]
   0x004019af <+112>:   mov    %eax,-0x68(%ebp)
   0x004019b2 <+115>:   sub    $0x1,%eax              ; eax = eax - 1
   0x004019b5 <+118>:   cmp    $0x5,%eax              ; 跟5比较
   0x004019b8 <+121>:   ja     0x401986 <phase_6+71>  ; 不能大于5
   0x004019ba <+123>:   addl   $0x1,-0x60(%ebp)       ; v16++ = *(ebp - 0x60) + 1
   0x004019be <+127>:   mov    -0x60(%ebp),%eax       ; eax = v16
   0x004019c1 <+130>:   cmp    $0x5,%eax              ; v16 与 5 比较
   0x004019c4 <+133>:   jg     0x4019cb <phase_6+140> ; v16 不能大于 5
   0x004019c6 <+135>:   mov    -0x64(%ebp),%esi       ; esi = v15
   0x004019c9 <+138>:   jmp    0x40199a <phase_6+91>

下面这一段是一个循环，伪代码：
  do
  {
    *v2 = 7 - *v2;
    ++v2;
  }
  while ( &v19 != v2 );

   0x004019cb <+140>:   lea    -0x4c(%ebp),%eax       ; v2 = eax = ebp - 0x4c
   0x004019ce <+143>:   lea    -0x34(%ebp),%esi       ; v19 = esi = ebp - 0x34
   0x004019d1 <+146>:   mov    $0x7,%ecx              ; ecx = 7
   0x004019d6 <+151>:   mov    %ecx,%edx              ; edx = 7
   0x004019d8 <+153>:   sub    (%eax),%edx            ; v2 = edx = 7 - *eax
   0x004019da <+155>:   mov    %edx,(%eax)            ; *v2 = *eax = (ebp - 0x4c) = edx
   0x004019dc <+157>:   add    $0x4,%eax              ; v2 = eax = eax + 4 = (ebp - 0x4c)++
   0x004019df <+160>:   cmp    %eax,%esi              ; v19 = esi
   0x004019e1 <+162>:   jne    0x4019d6 <phase_6+151> ; while v19 != v2

下面这一段是一个循环，伪代码：
  for ( i = 0; i != 6; ++i )
  {
    v4 = *(&v17 + i);
    v5 = 1;
    v6 = &node1;
    if ( v4 > 1 )
    {
      do
      {
        v6 = (_DWORD *)v6[2];
        ++v5;
      }
      while ( v5 != v4 );
    }
    *(&v19 + i) = (int)v6;
  }

   0x004019e3 <+164>:   mov    $0x0,%esi               ; 循环计数器赋初值，一共循环六次
   0x004019e8 <+169>:   mov    %esi,%edi               ; i = edi = esi = 0
   0x004019ea <+171>:   mov    -0x4c(%ebp,%esi,4),%ecx ; v4 = ecx = (ebp - 0x4c + esi * 4)
   0x004019ee <+175>:   mov    $0x1,%eax               ; v5 = eax = 1
   0x004019f3 <+180>:   lea    0x594(%ebx),%edx        ;
   0x004019f9 <+186>:   cmp    $0x1,%ecx
   0x004019fc <+189>:   jle    0x401a08 <phase_6+201>  ; if ecx <= 1
   0x004019fe <+191>:   mov    0x8(%edx),%edx
   0x00401a01 <+194>:   add    $0x1,%eax               ; v5++
   0x00401a04 <+197>:   cmp    %ecx,%eax               ; v5 与 v4 比较
   0x00401a06 <+199>:   jne    0x4019fe <phase_6+191>  ; while v5 != v4
   0x00401a08 <+201>:   mov    %edx,-0x34(%ebp,%edi,4)
   0x00401a0c <+205>:   add    $0x1,%esi               ; i = esi++
   0x00401a0f <+208>:   cmp    $0x6,%esi               ; 循环六次
   0x00401a12 <+211>:   jne    0x4019e8 <phase_6+169>

下一段也有一个循环，伪代码：
  v7 = v19;
  v8 = v20;
  *(_DWORD *)(v19 + 8) = v20;
  v9 = v21;
  *(_DWORD *)(v8 + 8) = v21;
  v10 = v22;
  *(v9 + 8) = v22;
  v11 = v23;
  *(v10 + 8) = v23;
  v12 = v24;
  *(v11 + 8) = v24;
  *(v12 + 8) = 0;
  v13 = 5;
  do
  {
    if ( *v7 < *(v7 + 8) )
      explode_bomb();
    v7 = *(v7 + 8);
    --v13;
  }
  while ( v13 );

   0x00401a14 <+213>:   mov    -0x34(%ebp),%esi ; v7 = esi = v19
   0x00401a17 <+216>:   mov    -0x30(%ebp),%eax ; v8 = eax = v20
   0x00401a1a <+219>:   mov    %eax,0x8(%esi)   ; 这一段是将node首尾相连
   0x00401a1d <+222>:   mov    -0x2c(%ebp),%edx
   0x00401a20 <+225>:   mov    %edx,0x8(%eax)
   0x00401a23 <+228>:   mov    -0x28(%ebp),%eax
   0x00401a26 <+231>:   mov    %eax,0x8(%edx)
   0x00401a29 <+234>:   mov    -0x24(%ebp),%edx
   0x00401a2c <+237>:   mov    %edx,0x8(%eax)
   0x00401a2f <+240>:   mov    -0x20(%ebp),%eax
   0x00401a32 <+243>:   mov    %eax,0x8(%edx)
   0x00401a35 <+246>:   movl   $0x0,0x8(%eax)
   0x00401a3c <+253>:   mov    $0x5,%edi
   0x00401a41 <+258>:   jmp    0x401a4b <phase_6+268>

   0x00401a43 <+260>:   mov    0x8(%esi),%esi ; 下一个元素
   0x00401a46 <+263>:   sub    $0x1,%edi ; 计数器
   0x00401a49 <+266>:   je     0x401a5b <phase_6+284> ; 计数器等于0，退出循环
   0x00401a4b <+268>:   mov    0x8(%esi),%eax ; eax 指向 esi 下一个元素
   0x00401a4e <+271>:   mov    (%eax),%eax    ; 将 eax 的内容赋值给 eax
   0x00401a50 <+273>:   cmp    %eax,(%esi)    ; 比较 两个元素的元素值
   0x00401a52 <+275>:   jge    0x401a43 <phase_6+260> ; 前一个元素必须小于后一个元素
   0x00401a54 <+277>:   call   0x401e95 <explode_bomb>
   0x00401a59 <+282>:   jmp    0x401a43 <phase_6+260>

   0x00401a5b <+284>:   mov    -0x1c(%ebp),%eax
   0x00401a5e <+287>:   xor    %gs:0x14,%eax
   0x00401a65 <+294>:   jne    0x401a6f <phase_6+304>
   0x00401a67 <+296>:   lea    -0xc(%ebp),%esp
   0x00401a6a <+299>:   pop    %ebx
   0x00401a6b <+300>:   pop    %esi
   0x00401a6c <+301>:   pop    %edi
   0x00401a6d <+302>:   pop    %ebp
   0x00401a6e <+303>:   ret
   0x00401a6f <+304>:   call   0x402e30 <__stack_chk_fail_local>
End of assembler dump.
```

注意到

```go
0x004019f3 <+180>:   lea    0x594(%ebx),%edx
```

不知道是啥，先输出一下，先运行到此处，输出`ebx+0x594`，发现等于`0x4054f8`，输出此处的信息，发现是个`node`，可能是结构体，第三个元素是下一个元素的指针，说明是个链表，输出所有元素：

```apl
(gdb) x/3x 0x4054f8
0x4054f8 <node1>:       0x000003c2      0x00000001      0x00405504
(gdb) x/3x 0x00405504
0x405504 <node2>:       0x000002ca      0x00000002      0x00405080
(gdb) x/3x 0x00405080
0x405080 <node6>:       0x0000029c      0x00000006      0x0040551c
(gdb) x/3x 0x0040551c
0x40551c <node4>:       0x0000018a      0x00000004      0x00405528
(gdb) x/3x 0x00405528
0x405528 <node5>:       0x00000046      0x00000005      0x00405510
(gdb) x/3x 0x00405510
0x405510 <node3>:       0x00000042      0x00000003      0x00000000
```

把数字按元素顺序排列好，第三行是按从大到小排序：

| 1     | 2     | 3    | 4     | 5    | 6     |
| ----- | ----- | ---- | ----- | ---- | ----- |
| 0x3c2 | 0x2ca | 0x42 | 0x18a | 0x46 | 0x29c |
| 1     | 2     | 5    | 6     | 4    | 3     |

查看伪代码，应该是从大到小排序，顺序应该是`1 2 6 4 5 3`，因为经过`7 - array[i]`的处理，所以应该输入的是`6 5 1 3 2 4`。

## **7. 最终结果**

```
Welcome to my fiendish little bomb. You have 6 phases with
which to blow yourself up. Have a nice day!
He is evil and fits easily into most overhead storage bins.
Phase 1 defused. How about the next one?
0 1 3 6 10 15
That's number 2.  Keep going!
4 0
Halfway there!
7 0 DrEvil
So you got that one.  Try this one.
pqrstu
Good work!  On to the next...
6 5 1 3 2 4
Congratulations! You've defused the bomb!
Your instructor has been notified and will verify your solution.
```

## **8.隐藏关卡**

在`bomb.c`发现`phase_6`之后还有一个`phase_defused`函数，查看汇编代码：

```go
Dump of assembler code for function phase_defused:
   0x0040206d <+0>:     endbr32
   0x00402071 <+4>:     push   %ebp
   0x00402072 <+5>:     mov    %esp,%ebp
   0x00402074 <+7>:     push   %ebx
   0x00402075 <+8>:     sub    $0x70,%esp
   0x00402078 <+11>:    call   0x4013d0 <__x86.get_pc_thunk.bx>
   0x0040207d <+16>:    add    $0x2ee7,%ebx
   0x00402083 <+22>:    mov    %gs:0x14,%eax
   0x00402089 <+28>:    mov    %eax,-0xc(%ebp)
   0x0040208c <+31>:    xor    %eax,%eax
   0x0040208e <+33>:    push   $0x1
   0x00402090 <+35>:    call   0x401dc1 <send_msg>
   0x00402095 <+40>:    add    $0x10,%esp
   0x00402098 <+43>:    cmpl   $0x6,0x7e0(%ebx)
   0x0040209f <+50>:    je     0x4020b6 <phase_defused+73>
   0x004020a1 <+52>:    mov    -0xc(%ebp),%eax
   0x004020a4 <+55>:    xor    %gs:0x14,%eax
   0x004020ab <+62>:    jne    0x402149 <phase_defused+220>
   0x004020b1 <+68>:    mov    -0x4(%ebp),%ebx
   0x004020b4 <+71>:    leave
   0x004020b5 <+72>:    ret
   0x004020b6 <+73>:    sub    $0xc,%esp
   0x004020b9 <+76>:    lea    -0x5c(%ebp),%eax
   0x004020bc <+79>:    push   %eax
   0x004020bd <+80>:    lea    -0x60(%ebp),%eax
   0x004020c0 <+83>:    push   %eax
   0x004020c1 <+84>:    lea    -0x64(%ebp),%eax   ; 这里是我们输入的 7 0
   0x004020c4 <+87>:    push   %eax
   0x004020c5 <+88>:    lea    -0x1b3d(%ebx),%eax ; 输入格式
   0x004020cb <+94>:    push   %eax
   0x004020cc <+95>:    lea    0x7fc(%ebx),%eax   ; 第一题输入的字符串
   0x004020d2 <+101>:   lea    0xf0(%eax),%eax
   0x004020d8 <+107>:   push   %eax
   0x004020d9 <+108>:   call   0x4012d0 <__isoc99_sscanf@plt>
   0x004020de <+113>:   add    $0x20,%esp
   0x004020e1 <+116>:   cmp    $0x3,%eax ; 第四题输入参数为三，才能跳转到secret_phase
   0x004020e4 <+119>:   je     0x402108 <phase_defused+155>
   0x004020e6 <+121>:   sub    $0xc,%esp
   0x004020e9 <+124>:   lea    -0x1c88(%ebx),%eax
   0x004020ef <+130>:   push   %eax
   0x004020f0 <+131>:   call   0x401280 <puts@plt>
   0x004020f5 <+136>:   lea    -0x1c5c(%ebx),%eax
   0x004020fb <+142>:   mov    %eax,(%esp)
   0x004020fe <+145>:   call   0x401280 <puts@plt>
   0x00402103 <+150>:   add    $0x10,%esp
   0x00402106 <+153>:   jmp    0x4020a1 <phase_defused+52>
   0x00402108 <+155>:   sub    $0x8,%esp
   0x0040210b <+158>:   lea    -0x1b34(%ebx),%eax ; 第四题第三个参数需要是 DrEvil
   0x00402111 <+164>:   push   %eax
   0x00402112 <+165>:   lea    -0x5c(%ebp),%eax
   0x00402115 <+168>:   push   %eax
   0x00402116 <+169>:   call   0x401c10 <strings_not_equal> ; 判断是不是DrEvil
   0x0040211b <+174>:   add    $0x10,%esp
   0x0040211e <+177>:   test   %eax,%eax ; 第四题第三个参数必须是 DrEvil 才能跳转到secret_phase
   0x00402120 <+179>:   jne    0x4020e6 <phase_defused+121>
   0x00402122 <+181>:   sub    $0xc,%esp
   0x00402125 <+184>:   lea    -0x1ce8(%ebx),%eax
   0x0040212b <+190>:   push   %eax
   0x0040212c <+191>:   call   0x401280 <puts@plt>
   0x00402131 <+196>:   lea    -0x1cc0(%ebx),%eax
   0x00402137 <+202>:   mov    %eax,(%esp)
   0x0040213a <+205>:   call   0x401280 <puts@plt>
   0x0040213f <+210>:   call   0x401aca <secret_phase>
   0x00402144 <+215>:   add    $0x10,%esp
   0x00402147 <+218>:   jmp    0x4020e6 <phase_defused+121>
   0x00402149 <+220>:   call   0x402e30 <__stack_chk_fail_local>
End of assembler dump.
```

把断点打在0x004020e1，运行到此处后看看：

```bash
x/s $ebx-0x1b3d
```

显示：

```go
0x403427:       "%d %d %s"
```

查看$ebp-0x64是啥：

```bash
x/xg $ebp-0x64
```

显示：

```
0xbffff494:     0x0000000000000007
```

`$ebp-0x64`对应第一个参数7，`$ebp-0x60`对应第二个参数0，这就是我们第四题输入的`7 0`，他需要三个参数，查看未知地址：

```bash
x/s $ebx-0x1b34
```

发现回显是`DrEvil`。根据

```go
  0x00402116 <+169>:   call   0x401c10 <strings_not_equal> ; 判断是不是DrEvil
```

可知，第四题的第三个参数必须是DrEvil才能跳转到secret_phase。继续往下看汇编发现：

```go
0x0040213f <+210>:   call   0x401aca <secret_phase>
```

看看`secret_phase`汇编代码：

```go
Dump of assembler code for function secret_phase:
   0x00401aca <+0>:     endbr32
   0x00401ace <+4>:     push   %ebp
   0x00401acf <+5>:     mov    %esp,%ebp
   0x00401ad1 <+7>:     push   %esi
   0x00401ad2 <+8>:     push   %ebx
   0x00401ad3 <+9>:     call   0x4013d0 <__x86.get_pc_thunk.bx>
   0x00401ad8 <+14>:    add    $0x348c,%ebx
   0x00401ade <+20>:    call   0x401f3a <read_line>
   0x00401ae3 <+25>:    sub    $0x4,%esp
   0x00401ae6 <+28>:    push   $0xa
   0x00401ae8 <+30>:    push   $0x0
   0x00401aea <+32>:    push   %eax
   0x00401aeb <+33>:    call   0x401340 <strtol@plt> ;strtol将eax存的字符串转化为10进制整数，使结果没有""
   0x00401af0 <+38>:    mov    %eax,%esi
   0x00401af2 <+40>:    lea    -0x1(%eax),%eax
   0x00401af5 <+43>:    add    $0x10,%esp
   0x00401af8 <+46>:    cmp    $0x3e8,%eax
   0x00401afd <+51>:    ja     0x401b34 <secret_phase+106>
   0x00401aff <+53>:    sub    $0x8,%esp
   0x00401b02 <+56>:    push   %esi
   0x00401b03 <+57>:    lea    0x540(%ebx),%eax
   0x00401b09 <+63>:    push   %eax
   0x00401b0a <+64>:    call   0x401a74 <fun7>
   0x00401b0f <+69>:    add    $0x10,%esp
   0x00401b12 <+72>:    test   %eax,%eax ; 检查 fun7 返回值是不是 0
   0x00401b14 <+74>:    jne    0x401b3b <secret_phase+113> ; 不是0，就爆炸
   0x00401b16 <+76>:    sub    $0xc,%esp
   0x00401b19 <+79>:    lea    -0x1de4(%ebx),%eax
   0x00401b1f <+85>:    push   %eax
   0x00401b20 <+86>:    call   0x401280 <puts@plt>
   0x00401b25 <+91>:    call   0x40206d <phase_defused>
   0x00401b2a <+96>:    add    $0x10,%esp
   0x00401b2d <+99>:    lea    -0x8(%ebp),%esp
   0x00401b30 <+102>:   pop    %ebx
   0x00401b31 <+103>:   pop    %esi
   0x00401b32 <+104>:   pop    %ebp
   0x00401b33 <+105>:   ret
   0x00401b34 <+106>:   call   0x401e95 <explode_bomb>
   0x00401b39 <+111>:   jmp    0x401aff <secret_phase+53>
   0x00401b3b <+113>:   call   0x401e95 <explode_bomb>
   0x00401b40 <+118>:   jmp    0x401b16 <secret_phase+76>
End of assembler dump.
```

重点关注：

```go
   0x00401b0a <+64>:    call   0x401a74 <fun7>
   0x00401b0f <+69>:    add    $0x10,%esp
   0x00401b12 <+72>:    test   %eax,%eax ; 检查 fun7 返回值是不是 0
   0x00401b14 <+74>:    jne    0x401b3b <secret_phase+113> ; 不是0，就爆炸
```

只要fun7的返回值是0就行了。fun7函数汇编代码：

```go
Dump of assembler code for function fun7:
   0x00401a74 <+0>:     endbr32
   0x00401a78 <+4>:     push   %ebp
   0x00401a79 <+5>:     mov    %esp,%ebp
   0x00401a7b <+7>:     push   %ebx
   0x00401a7c <+8>:     sub    $0x4,%esp
   0x00401a7f <+11>:    mov    0x8(%ebp),%edx
   0x00401a82 <+14>:    mov    0xc(%ebp),%ecx
   0x00401a85 <+17>:    test   %edx,%edx
   0x00401a87 <+19>:    je     0x401ac3 <fun7+79>
   0x00401a89 <+21>:    mov    (%edx),%ebx
   0x00401a8b <+23>:    cmp    %ecx,%ebx
   0x00401a8d <+25>:    jg     0x401a9b <fun7+39> ; 调用左孩子
   0x00401a8f <+27>:    mov    $0x0,%eax ; 给result复制为0
   0x00401a94 <+32>:    jne    0x401aae <fun7+58> ; 调用右孩子
   0x00401a96 <+34>:    mov    -0x4(%ebp),%ebx
   0x00401a99 <+37>:    leave
   0x00401a9a <+38>:    ret
   0x00401a9b <+39>:    sub    $0x8,%esp
   0x00401a9e <+42>:    push   %ecx
   0x00401a9f <+43>:    pushl  0x4(%edx) ; 调用左孩子
   0x00401aa2 <+46>:    call   0x401a74 <fun7>
   0x00401aa7 <+51>:    add    $0x10,%esp
   0x00401aaa <+54>:    add    %eax,%eax
   0x00401aac <+56>:    jmp    0x401a96 <fun7+34>
   0x00401aae <+58>:    sub    $0x8,%esp
   0x00401ab1 <+61>:    push   %ecx
   0x00401ab2 <+62>:    pushl  0x8(%edx) ; 调用右孩子
   0x00401ab5 <+65>:    call   0x401a74 <fun7>
   0x00401aba <+70>:    add    $0x10,%esp
   0x00401abd <+73>:    lea    0x1(%eax,%eax,1),%eax
   0x00401ac1 <+77>:    jmp    0x401a96 <fun7+34>
   0x00401ac3 <+79>:    mov    $0xffffffff,%eax ; 返回-1
   0x00401ac8 <+84>:    jmp    0x401a96 <fun7+34>
End of assembler dump.
```

fun7汇编等价于：

```c
int __cdecl fun7(_DWORD *a1, int a2)
{
  int result; // eax

  if ( !a1 )
    return -1;
  if ( *a1 > a2 )
    return 2 * fun7(a1[1], a2);
  result = 0;
  if ( *a1 != a2 )
    result = 2 * fun7(a1[2], a2) + 1;
  return result;
}
```

a1是当前搜索到的值，a2是输入的值，当输入值等于当前搜索值时，返回0。调用fun7之前，有个指令：

```go
 0x00401b03 <+57>:    lea    0x540(%ebx),%eax
```

输出此处的内容：

```go
(gdb) x/24wx $ebx+0x540
0x4054a4 <n1>:          0x00000024      0x004054b0      0x004054bc      0x00000008
0x4054b4 <n21+4>:       0x004054e0      0x004054c8      0x00000032      0x004054d4
0x4054c4 <n22+8>:       0x004054ec      0x00000016      0x0040505c      0x00405044
0x4054d4 <n33>:         0x0000002d      0x00405020      0x00405068      0x00000006
0x4054e4 <n31+4>:       0x0040502c      0x00405050      0x0000006b      0x00405038
0x4054f4 <n34+8>:       0x00405074      0x000003c2      0x00000001      0x00405504
```

发现这又是一个结构体，第一个字是节点值，后面两个都是地址。一个一个输出看看：

```go
(gdb) x/3wx $ebx+0x540
0x4054a4 <n1>:  0x00000024      0x004054b0      0x004054bc
(gdb) x/3wx 0x004054b0
0x4054b0 <n21>: 0x00000008      0x004054e0      0x004054c8
(gdb) x/3wx 0x004054bc
0x4054bc <n22>: 0x00000032      0x004054d4      0x004054ec
(gdb) x/3wx 0x004054e0
0x4054e0 <n31>: 0x00000006      0x0040502c      0x00405050
(gdb) x/3wx 0x004054c8
0x4054c8 <n32>: 0x00000016      0x0040505c      0x00405044
(gdb) x/3wx 0x004054d4
0x4054d4 <n33>: 0x0000002d      0x00405020      0x00405068
(gdb) x/3wx 0x004054ec
0x4054ec <n34>: 0x0000006b      0x00405038      0x00405074
(gdb) x/3wx 0x0040502c
0x40502c <n41>: 0x00000001      0x00000000      0x00000000
(gdb) x/3wx 0x00405050
0x405050 <n42>: 0x00000007      0x00000000      0x00000000
(gdb) x/3wx 0x0040505c
0x40505c <n43>: 0x00000014      0x00000000      0x00000000
(gdb) x/3wx 0x00405044
0x405044 <n44>: 0x00000023      0x00000000      0x00000000
(gdb) x/3wx 0x00405020
0x405020 <n45>: 0x00000028      0x00000000      0x00000000
(gdb) x/3wx 0x00405068
0x405068 <n46>: 0x0000002f      0x00000000      0x00000000
(gdb) x/3wx 0x00405038
0x405038 <n47>: 0x00000063      0x00000000      0x00000000
(gdb) x/3wx 0x00405074
0x405074 <n48>: 0x000003e9      0x00000000      0x00000000
```

整理一下，这是一个二叉树：

```
          ┌─────────0x24─────────┐
          ▼                      ▼
    ┌───0x08────┐          ┌────0x32────┐
    ▼           ▼          ▼            ▼
 ┌─0x6─┐    ┌─0x16─┐    ┌─0x2d─┐    ┌─0x6b─┐
 ▼     ▼    ▼      ▼    ▼      ▼    ▼      ▼
0x1   0x7  0x14  0x23  0x28  0x2f  0x63  0x3e9
```

注意到：

```go
0x00401a89 <+21>:    mov    (%edx),%ebx
0x00401a8b <+23>:    cmp    %ecx,%ebx
```

这里ecx是二叉树根节点36，ebx是我们输入的数字，因此只要我们的输入也是36，则两个if条件都不会执行，最后直接返回0，结果：

```
Welcome to my fiendish little bomb. You have 6 phases with
which to blow yourself up. Have a nice day!
He is evil and fits easily into most overhead storage bins.
Phase 1 defused. How about the next one?
0 1 3 6 10 15
That's number 2.  Keep going!
4 0
Halfway there!
7 0 DrEvil
So you got that one.  Try this one.
pqrstu
Good work!  On to the next...
6 5 1 3 2 4
Curses, you've found the secret phase!
But finding it and solving it are quite different...
36
Wow! You've defused the secret stage!
Congratulations! You've defused the bomb!
Your instructor has been notified and will verify your solution.
```

# References

[CSAPP实验之bomb lab（上）](https://zhuanlan.zhihu.com/p/48759303)

[《深入理解计算机系统》Bomb Lab实验解析](https://earthaa.github.io/2020/01/12/CSAPP-Bomblab/)

[《深入理解计算机系统/CSAPP》Bomb Lab](https://zhuanlan.zhihu.com/p/57977157)

[CSAPP-bomblab](https://yuhan2001.github.io/2021/04/01/bomblab/)

[Introduction to CSAPP（十九）：这可能是你能找到的分析最全的Bomblab了](https://zhuanlan.zhihu.com/p/104130161)

[深入理解计算机系统（CSAPP）实验二 Bomb Lab](http://xzjqx.github.io/2018/04/26/bomblab/)