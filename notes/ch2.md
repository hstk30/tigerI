## 词法分析

### 处理字符串和注释

#### 方案一

一个正则强上，按这种模式构造一个

```
{START} ( {SIMPLE} | {COMPLEX}  ) * {END}
```

形式的正则，不过可能不好处理转义字符，考虑的东西多了正则会非常复杂，后期加新东西会变的很麻烦。
匹配到字符串后走一个`read_string(char *s)` 的函数集中处理字符串。


#### 方案二

加入状态(state), 比较推荐，也简单


#### 方案三

直接手写词法分析，感觉也不是很难。


### 参考

- [Start Conditions](https://westes.github.io/flex/manual/Start-Conditions.html)
- [Recognising strings and/or comments](https://www.cs.man.ac.uk/~pjj/cs212/ex2_str_comm.html)
- [The Fundamentals of lex Rules](https://docs.oracle.com/cd/E19504-01/802-5880/6i9k05dgk/index.html#lex-36741)
- [regular-expression-for-a-string-literal-in-flex-lex](https://stackoverflow.com/questions/2039795/regular-expression-for-a-string-literal-in-flex-lex)


