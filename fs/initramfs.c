__attribute__((section(".rodata")))
const char file_hello_txt[] =
"hello from ramfs!\n"
"you can 'cat /hello.txt'\n";

__attribute__((section(".rodata")))
const char file_info_txt[] =
"nano-os demo files\n"
"/hello.txt\n"
"/info.txt\n";
