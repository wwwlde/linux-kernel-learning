# Mac mini 2024 with M4

## How to Run x86 containers on an Mac mini 2024 with M4

1. Включаем QEMU в Docker:

```bash
docker run --rm --privileged multiarch/qemu-user-static --reset -p yes
```

2. Перезапускаем Docker.

3. Запускаем контейнер с эмуляцией i386:

```bash
docker run --platform linux/386 -it i386/debian:oldstable /bin/bash
```

# Modified version of the linux kernel 0.11, which can be compiled on 64-bit Ubuntu

To apply the patch, assume the source codes are decompressed to the directory `linux-0.11`, then

```bash
cd linux-0.11/
patch -p1 < linux-0.11-deb64.patch
```

After applying the patch above, I also made the following changes:

[kernel/panic.c](kernel/panic.c)

```c
void __stack_chk_fail(void) {
    panic("Stack smashing detected!");
}
```

I am also providing the complete patch for [include/string.h](include/string.h):

```patch
 diff -Naur string.h.orig string.h
--- string.h.orig       1991-09-17 15:04:09.000000000 +0000
+++ string.h    2025-02-01 19:32:50.993273012 +0000
@@ -10,6 +10,8 @@
 typedef unsigned int size_t;
 #endif
 
+__attribute__((gnu_inline))
+
 extern char * strerror(int errno);
 
 /*
@@ -26,18 +28,24 @@
  
 extern inline char * strcpy(char * dest,const char *src)
 {
-__asm__("cld\n"
+__asm__("pushl %%esi\n\t"
+        "pushl %%edi\n\t"
+        "cld\n"
        "1:\tlodsb\n\t"
        "stosb\n\t"
        "testb %%al,%%al\n\t"
-       "jne 1b"
-       ::"S" (src),"D" (dest):"si","di","ax");
+       "jne 1b\n\t"
+        "popl %%edi\n\t"
+        "popl %%esi"
+       ::"S" (src),"D" (dest):"ax");
 return dest;
 }
 
 extern inline char * strncpy(char * dest,const char *src,int count)
 {
-__asm__("cld\n"
+__asm__("pushl %%esi\n\t"
+        "pushl %%edi\n\t"
+        "cld\n"
        "1:\tdecl %2\n\t"
        "js 2f\n\t"
        "lodsb\n\t"
@@ -46,28 +54,36 @@
        "jne 1b\n\t"
        "rep\n\t"
        "stosb\n"
-       "2:"
-       ::"S" (src),"D" (dest),"c" (count):"si","di","ax","cx");
+       "2:\n\t"
+        "popl %%edi\n\t"
+        "popl %%esi"
+       ::"S" (src),"D" (dest),"c" (count):"ax");
 return dest;
 }
 
 extern inline char * strcat(char * dest,const char * src)
 {
-__asm__("cld\n\t"
+__asm__("pushl %%esi\n\t"
+        "pushl %%edi\n\t"
+        "cld\n\t"
        "repne\n\t"
        "scasb\n\t"
        "decl %1\n"
        "1:\tlodsb\n\t"
        "stosb\n\t"
        "testb %%al,%%al\n\t"
-       "jne 1b"
-       ::"S" (src),"D" (dest),"a" (0),"c" (0xffffffff):"si","di","ax","cx");
+       "jne 1b\n\t"
+        "popl %%edi\n\t"
+        "popl %%esi"
+       ::"S" (src),"D" (dest),"a" (0),"c" (0xffffffff));
 return dest;
 }
 
 extern inline char * strncat(char * dest,const char * src,int count)
 {
-__asm__("cld\n\t"
+__asm__("pushl %%esi\n\t"
+        "pushl %%edi\n\t"
+        "cld\n\t"
        "repne\n\t"
        "scasb\n\t"
        "decl %1\n\t"
@@ -79,16 +95,19 @@
        "testb %%al,%%al\n\t"
        "jne 1b\n"
        "2:\txorl %2,%2\n\t"
-       "stosb"
-       ::"S" (src),"D" (dest),"a" (0),"c" (0xffffffff),"g" (count)
-       :"si","di","ax","cx");
+       "stosb\n\t"
+        "popl %%edi\n\t"
+        "popl %%esi"
+       ::"S" (src),"D" (dest),"a" (0),"c" (0xffffffff),"g" (count));
 return dest;
 }
 
 extern inline int strcmp(const char * cs,const char * ct)
 {
-register int __res __asm__("ax");
-__asm__("cld\n"
+  register int __res __asm__("ax");
+__asm__("pushl %%esi\n\t"
+        "pushl %%edi\n\t"
+        "cld\n"
        "1:\tlodsb\n\t"
        "scasb\n\t"
        "jne 2f\n\t"
@@ -99,15 +118,19 @@
        "2:\tmovl $1,%%eax\n\t"
        "jl 3f\n\t"
        "negl %%eax\n"
-       "3:"
-       :"=a" (__res):"D" (cs),"S" (ct):"si","di");
+       "3:\n\t"
+        "popl %%edi\n\t"
+        "popl %%esi"
+       :"=a" (__res):"D" (cs),"S" (ct));
 return __res;
 }
 
 extern inline int strncmp(const char * cs,const char * ct,int count)
 {
 register int __res __asm__("ax");
-__asm__("cld\n"
+__asm__("pushl %%esi\n\t"
+        "pushl %%edi\n\t"
+        "cld\n"
        "1:\tdecl %3\n\t"
        "js 2f\n\t"
        "lodsb\n\t"
@@ -120,15 +143,18 @@
        "3:\tmovl $1,%%eax\n\t"
        "jl 4f\n\t"
        "negl %%eax\n"
-       "4:"
-       :"=a" (__res):"D" (cs),"S" (ct),"c" (count):"si","di","cx");
+       "4:\n\t"
+        "popl %%edi\n\t"
+        "popl %%esi"
+       :"=a" (__res):"D" (cs),"S" (ct),"c" (count));
 return __res;
 }
 
 extern inline char * strchr(const char * s,char c)
 {
 register char * __res __asm__("ax");
-__asm__("cld\n\t"
+__asm__("pushl %%esi\n\t"
+        "cld\n\t"
        "movb %%al,%%ah\n"
        "1:\tlodsb\n\t"
        "cmpb %%ah,%%al\n\t"
@@ -137,15 +163,17 @@
        "jne 1b\n\t"
        "movl $1,%1\n"
        "2:\tmovl %1,%0\n\t"
-       "decl %0"
-       :"=a" (__res):"S" (s),"0" (c):"si");
+       "decl %0\n\t"
+        "popl %%esi"
+       :"=a" (__res):"S" (s),"0" (c));
 return __res;
 }
 
 extern inline char * strrchr(const char * s,char c)
 {
 register char * __res __asm__("dx");
-__asm__("cld\n\t"
+__asm__("pushl %%esi\n\t"
+        "cld\n\t"
        "movb %%al,%%ah\n"
        "1:\tlodsb\n\t"
        "cmpb %%ah,%%al\n\t"
@@ -153,8 +181,9 @@
        "movl %%esi,%0\n\t"
        "decl %0\n"
        "2:\ttestb %%al,%%al\n\t"
-       "jne 1b"
-       :"=d" (__res):"0" (0),"S" (s),"a" (c):"ax","si");
+       "jne 1b\n\t"
+        "popl %%esi"
+       :"=d" (__res):"0" (0),"S" (s),"a" (c));
 return __res;
 }
 
@@ -178,7 +207,7 @@
        "je 1b\n"
        "2:\tdecl %0"
        :"=S" (__res):"a" (0),"c" (0xffffffff),"0" (cs),"g" (ct)
-       :"ax","cx","dx","di");
+       :"dx","di");
 return __res-cs;
 }
 
@@ -202,7 +231,7 @@
        "jne 1b\n"
        "2:\tdecl %0"
        :"=S" (__res):"a" (0),"c" (0xffffffff),"0" (cs),"g" (ct)
-       :"ax","cx","dx","di");
+       :"dx","di");
 return __res-cs;
 }
 
@@ -229,7 +258,7 @@
        "2:\txorl %0,%0\n"
        "3:"
        :"=S" (__res):"a" (0),"c" (0xffffffff),"0" (cs),"g" (ct)
-       :"ax","cx","dx","di");
+       :"dx","di");
 return __res;
 }
 
@@ -256,19 +285,21 @@
        "xorl %%eax,%%eax\n\t"
        "2:"
        :"=a" (__res):"0" (0),"c" (0xffffffff),"S" (cs),"g" (ct)
-       :"cx","dx","di","si");
+       :"dx","di");
 return __res;
 }
 
 extern inline int strlen(const char * s)
 {
 register int __res __asm__("cx");
-__asm__("cld\n\t"
+__asm__("pushl %%edi\n\t"
+        "cld\n\t"
        "repne\n\t"
        "scasb\n\t"
        "notl %0\n\t"
-       "decl %0"
-       :"=c" (__res):"D" (s),"a" (0),"0" (0xffffffff):"di");
+       "decl %0\n\t"
+        "popl %%edi"
+       :"=c" (__res):"D" (s),"a" (0),"0" (0xffffffff));
 return __res;
 }
 
@@ -327,7 +358,7 @@
        "jne 8f\n\t"
        "movl %0,%1\n"
        "8:"
-       :"=b" (__res),"=S" (___strtok)
+       :"=b" (__res),"=r" (___strtok)
        :"0" (___strtok),"1" (s),"g" (ct)
        :"ax","cx","dx","di");
 return __res;
@@ -335,35 +366,47 @@
 
 extern inline void * memcpy(void * dest,const void * src, int n)
 {
-__asm__("cld\n\t"
+__asm__("pushl %%esi\n\t"
+        "pushl %%edi\n\t"
+        "cld\n\t"
        "rep\n\t"
-       "movsb"
-       ::"c" (n),"S" (src),"D" (dest)
-       :"cx","si","di");
+       "movsb\n\t"
+        "popl %%edi\n\t"
+        "popl %%esi"
+       ::"c" (n),"S" (src),"D" (dest));
 return dest;
 }
 
 extern inline void * memmove(void * dest,const void * src, int n)
 {
 if (dest<src)
-__asm__("cld\n\t"
+__asm__("pushl %%esi\n\t"
+        "pushl %%edi\n\t"
+        "cld\n\t"
        "rep\n\t"
-       "movsb"
-       ::"c" (n),"S" (src),"D" (dest)
-       :"cx","si","di");
+       "movsb\n\t"
+        "popl %%edi\n\t"
+        "popl %%esi"
+       ::"c" (n),"S" (src),"D" (dest));
 else
-__asm__("std\n\t"
+__asm__("pushl %%esi\n\t"
+        "pushl %%edi\n\t"
+        "std\n\t"
        "rep\n\t"
-       "movsb"
-       ::"c" (n),"S" (src+n-1),"D" (dest+n-1)
-       :"cx","si","di");
+       "movsb\n\t"
+        "cld\n\t"
+        "popl %%edi\n\t"
+        "popl %%esi"
+       ::"c" (n),"S" (src+n-1),"D" (dest+n-1));
 return dest;
 }
 
 extern inline int memcmp(const void * cs,const void * ct,int count)
 {
 register int __res __asm__("ax");
-__asm__("cld\n\t"
+__asm__("pushl %%esi\n\t"
+        "pushl %%edi\n\t"
+        "cld\n\t"
        "repe\n\t"
        "cmpsb\n\t"
        "je 1f\n\t"
@@ -371,8 +414,7 @@
        "jl 1f\n\t"
        "negl %%eax\n"
        "1:"
-       :"=a" (__res):"0" (0),"D" (cs),"S" (ct),"c" (count)
-       :"si","di","cx");
+       :"=a" (__res):"0" (0),"D" (cs),"S" (ct),"c" (count));
 return __res;
 }
 
@@ -387,18 +429,18 @@
        "je 1f\n\t"
        "movl $1,%0\n"
        "1:\tdecl %0"
-       :"=D" (__res):"a" (c),"D" (cs),"c" (count)
-       :"cx");
+       :"=D" (__res):"a" (c),"D" (cs),"c" (count));
 return __res;
 }
 
 extern inline void * memset(void * s,char c,int count)
 {
-__asm__("cld\n\t"
+__asm__("pushl %%edi\n\t"
+        "cld\n\t"
        "rep\n\t"
-       "stosb"
-       ::"a" (c),"D" (s),"c" (count)
-       :"cx","di");
+       "stosb\n\t"
+        "popl %%edi"
+       ::"a" (c),"D" (s),"c" (count));
 return s;
 }
 ```

Link: [Linux kernel v0.11](https://github.com/ellipse/linux-0.11-deb)

# Finally!

```bash
...
make[1]: Leaving directory `/opt/linux-0.11/lib'
ld -m elf_i386 -Ttext 0 -e startup_32 -M -x boot/head.o init/main.o \
        kernel/kernel.o mm/mm.o fs/fs.o \
        kernel/blk_drv/blk_drv.a kernel/chr_drv/chr_drv.a \
        kernel/math/math.a \
        lib/lib.a \
        -o tools/system > System.map
objcopy --only-keep-debug tools/system tools/system.dbg
objcopy --add-gnu-debuglink=tools/system.dbg tools/system
objcopy -g tools/system
gcc -DRAMDISK=512 -m32 -Wall -O1 -g -fstrength-reduce -fomit-frame-pointer -mtune=i386 -fno-stack-protector \
        -o tools/build tools/build.c
tools/build boot/bootsect boot/setup tools/system FLOPPY > Image
Root device is (0, 0)
Boot sector 512 bytes.
Setup is 312 bytes.
System is 117201 bytes.
sync
```

```bash
qemu-system-i386 -L pc-bios -fda bootimage-fda.img -hda rootfs.img -boot a -no-reboot -m 16 -k en-us
```

![Linux 0.11](./2025-02-01%2023.55.52.jpg)


🎉🎉🎉
