# 语义分析

- 我的位置信息还有偏差，我都想自己写词法分析了
- `break` 在`for` `while` 的检查没有做
- `for` 中的隐变量不应该被赋值，没有处理
- `semant.c` 文件的代码有点长了，600-700 行了，后面还有转中间代码的代码，
    这个还是抽象出`translate` 后的。
- 这个实现里，比较类型是否相等，尤其是自己声明的类型(都在外面套了一层`Ty_Name`)，
    是直接比较两个指针是否相等。对于内建类型，因为`transExp` 返回的都是经过`actual_ty` 处理，
    所以可以通过检查`kind` 来进行。
- 重新组织输出的错误信息，统一组织，放在一个模块里，用宏定义处理可能会简单
- 对于语义分析、类型检查报错的地方，因为需要继续执行，现在的实现是返回默认的
    `Ty_Int()` 类型，因此容易出现**错上加错** 的问题，导致报错信息增加。
    其实，我自己在调试的时候，如果报错太多，也就只看前几条报错，先改好了前面的报错，
    再编译一遍，所以感觉也还行😬 不过，之前学习了一下`Rust` ，那个报错信息可太详细了，
    详细的让人不想看了。
- 类似上一条，当类型在声明之前被使用的时候，这个类型经过`transTy` 的时候应该返回一个什么？
    需要考虑到被这个类型声明的变量在表达式中的类型检查。
- 对于`S_table` `TAB_table` 的优化，这个项目我肯定不会去做。什么时候我写自己的编译器了，
    就去把`Python` 的`Dict` 的抄过来，再抄个`AVL Tree` 就可以了


## NULL Pointer 

实现上遇到很多个`NULL Pointer` 问题，只有在运行时才出错，编译时可以通过。不过这也是常事了。

`NULL` 的发明者**Tony Hoare** 也称`NULL` 是一个`billion-dollar mistake`

> In 1965, I was designing the first comprehensive type system for references in an object oriented language (ALGOL W). My goal was to ensure that all use of references should be absolutely safe, with checking performed automatically by the compiler. But I couldn’t resist the temptation to put in a null reference, simply because it was so easy to implement. This has led to innumerable errors, vulnerabilities, and system crashes, which have probably caused a billion dollars of pain and damage in the last forty years.


`Rust` 中通过引入`Option` 来处理空值确实能提前避免这种问题。
但是引入的`unwrap` 依然会`panic` ，只不过这个责任在于程序员。
当然，提示信息更明显，错误定位更容易了。
不过，本质上和`c` 中的代码

```
if (p == NULL)
```

也没啥区别。

不过，引入了`unwrap`，代码里基本尾巴上都会接一个`unwrap()`，
有的代码就很长，感觉很不爽。

