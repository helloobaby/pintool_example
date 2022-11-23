### 如何编译
****

下载pin windows包  
https://www.intel.com/content/www/us/en/developer/articles/tool/pin-a-binary-instrumentation-tool-downloads.html  
然后把例子代码直接复制粘贴到[下载的pin根目录]\source\tools\MyPinTool\MyPinTool.cpp，然后用vs2019打开同目录下面的sln直接编译。

****
## example1.cpp
记录整个程序调用的ntdll里的api

## example2.cpp
记录模块加载情况

## example3.cpp
hook指定api 类似于微软的detours库
