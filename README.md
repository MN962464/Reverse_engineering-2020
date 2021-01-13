# Reverse_engineering-2020

## 大作业：结合IATHook实现DLL注入

- API hook技术可以精准的拦截到我们所需要的API，是代码注入和dll注入的一个应用实例（代码注入和dll注入技术的一个功能就是HOOK API），是逆向工程中最重要的一部分，经常用来破解网络验证和处理反调试。更深层次的逆向(系统内核的逆向都需要Hook API)

- 两个API的场景
    + 勾取user32.dll中的SetWindowText，换成执行MySetWindowText，将计算器中的阿拉伯数字转换为中文数字(也可以是汉字，或者修改对应代码转换为abcd等任何想要的形式)，同时也能重新调用user32.dll中的函数，恢复calc.exe的IAT值。
    [Hook_Caculator实验报告](Hook_Caculator/Readme.md)
    + 修改的是kernel32.dll中notepad.exe中的WriteFile API，在记事本调用WriteFile之前设置断点，将记事本内容的小写全部改为大写之后，再重新调用WriteFile函数完成保存操作，保存之后的txt文件打开后由小写变成了大写。
    [Hook_notepad实验报告](Hook_notepad/Readme.md)

+ 工程文件见对应文件夹