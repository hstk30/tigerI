# 指令选择


## 目前的成果

```
let
    function fib(t: int): int = 
        if (t < 2)
        then t
        else fib(t - 1) + fib(t - 2)
in
    fib(5)
end
```

的`aarch64` 汇编

```
L7:
    mov t134, #1
    mov t139, #2
    subs xzr, t133, t139
    b.lt L0
L1:
    mov t134, #0
L0:
    mov t140, #0
    subs xzr, t134, t140
    b.ne L2
L3:
    mov x0, x29
    mov t142, #1
    sub t141, t133, t142
    mov x1, t141
    bl fib
    mov t136, x0
    mov t138, t136
    mov x0, x29
    mov t144, #2
    sub t143, t133, t144
    mov x1, t143
    bl fib
    mov t137, x0
    mov t135, t145
L4:
    mov x0, t135
    b L6
L2:
    mov t135, t133
    b L4
L6:

L9:
    mov x0, x29
    mov t146, #5
    mov x1, t146
    bl fib
    mov x0, x0
    b L8
L8:
```

## 存在的问题

- 函数 **入口处理代码（prologue）** 还没有加入，所以栈帧分配，函数实参传递的代码没有生成
- 在代码生成`codegen` 中，没有处理函数的”逃逸“ 参数，暂时都放在参数寄存器中，
    还不确定要怎么处理，如果要在`munchArgs` 函数中处理，则每个函数需要一个`F_frame` 参数
- 应该看到，因为在 [翻译成中间代码](ch6-7.md) 中的条件表达式的处理，现在生成的条件分支的汇编
    很啰嗦，`if (t < 2)` 只需要一个分支指令即可，现在生成了两条。
- 因为使用了静态链，所以每次函数调用都有`mov x0, x29` 这条指令
- 生成的字符串汇编需要去重，甚至重新组织编码。
- 感觉应该把生成字符串、程序片段的代码也放到对应的`codegen` 文件，或者另起一个模块。
- 可优化：书上说树形匹配时使用类似`switch` 的快速匹配，我试了一下，发现这样代码更复杂了，
    总不可能写成状态机吧。

