#Директория fs/ (Файловая система)

Отвечает за работу с файлами.

 * `buffer.c` – кэширование данных с диска (буферизация).
 * `block_dev.c` – работа с блочными устройствами.
 * `open.c`, `read_write.c` – реализация системных вызовов open(), read(), write().
 * `super.c` – работа с суперблоком файловой системы (Minix FS).
 * `inode.c` – работа с индексными дескрипторами (inodes).

📌 Linux 0.11 использует Minix-файловую систему, так как на тот момент не было собственной.
