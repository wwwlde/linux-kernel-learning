# Директория boot/ (Загрузчик ядра)

Этот каталог содержит код, который загружает ядро в память при старте системы.

 * `boot.s` – написан на ассемблере x86 (Real Mode, 16 бит). Он загружает загрузчик второго этапа.
 * `setup.s` – тоже на ассемблере, выполняет инициализацию оборудования (видео, память, диски).
 * `video.s` – инициализация видеорежима.

📌 Эти файлы работают в 16-битном режиме процессора и выполняют начальную настройку перед переходом в 32-битный режим.
