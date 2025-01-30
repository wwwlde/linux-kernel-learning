/*
 *  linux/kernel/sched.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 * 'sched.c' is the main kernel file. It contains scheduling primitives
 * (sleep_on, wakeup, schedule etc) as well as a number of simple system
 * call functions (type getpid(), which just extracts a field from
 * current-task
 */

//
// Планировщик процессов
//
/*
 * Этот заголовочный файл содержит определения и структуры, связанные с процессами и планированием (scheduler).
 *
 * Основные элементы:
 *
 * struct task_struct – структура процесса.
 * NR_TASKS – максимальное число процессов (64 в Linux 0.11).
 * current – макрос, который указывает на текущий процесс.
 * schedule() – функция планировщика процессов.
 */
#include <linux/sched.h>

//
// Общие функции ядра
//
/*
 * Этот файл содержит вспомогательные макросы и функции для ядра.
 * 
 * Основные элементы:
 * 
 * printk() – аналог printf(), но для вывода сообщений ядром.
 * panic() – завершение системы при критической ошибке.
 * extern int printf(const char *fmt, ...); – отладочный вывод.
 */
#include <linux/kernel.h>

// Системные вызовы
/*
 * Определяет системные вызовы (syscalls), такие как sys_fork(), sys_exit(), sys_open().
 *
 * Основные элементы:
 * 
 * sys_fork() – создание нового процесса.
 * sys_exit() – завершение процесса.
 * sys_open() – открытие файла.
 */
#include <linux/sys.h>

//
// Регистры дисковода :'-)
//
/*
 * Этот заголовочный файл содержит определения регистров флоппи-дисковода (FDC, Floppy Disk Controller).
 * 
 * Основные элементы:
 * 
 * FD_STATUS – статусный регистр FDC.
 * FD_DATA – регистр данных FDC.
 * FD_DOR – регистр управления приводом.
 *
 * Пример использования в коде драйвера дисковода:
 * 
 * outb(FD_DOR, 0x1C); // Включаем флоппи-дисковод
 *
 */
#include <linux/fdreg.h>

//
// Системные макросы и функции ASM
//
/*
 * Этот файл содержит низкоуровневый код, связанный с переключением задач, атомарными операциями и прерываниями.
 *
 * Основные элементы:
 *
 * sti() / cli() – включение/выключение прерываний.
 * switch_to(n) – макрос переключения задач.
 * set_trap_gate(n, addr) – установка обработчиков прерываний.
 *
 * Пример макроса sti():
 * 
 * #define sti() __asm__("sti") // Включение прерываний
 *
 */
#include <asm/system.h>

//
// Ввод-вывод на низком уровне
//
/*
 * Содержит макросы и функции для работы с портами ввода-вывода.
 *
 * Основные элементы:
 * 
 * outb(value, port) – записывает байт в порт.
 * inb(port) – читает байт из порта.
 * outb_p(value, port) – задержка после записи (для старых устройств).
 * Пример использования:
 * 
 * outb(0x1F, 0x3F8); // Запись в порт COM1 (последовательный порт)
 *
 */
#include <asm/io.h>

//
// Работа с сегментами памяти
//
/*
 * Этот файл содержит макросы для работы с сегментами памяти в x86 (Protected Mode).
 *
 * Основные элементы:
 *
 * get_fs_byte(ptr) – чтение байта из пользовательского пространства.
 * put_fs_byte(byte, ptr) – запись байта в пользовательское пространство.
 * set_fs(seg) – установка сегмента данных.
 *
 * Пример макроса get_fs_byte() в segment.h:
 *
 * #define get_fs_byte(x) ({ \
 *    register char __res; \
 *    __asm__("movb %%fs:%1,%0" : "=q" (__res) : "m" (*(x))); \
 *    __res; \
 * })
 *
 */
#include <asm/segment.h>

//
// Обработчик сигналов
//
/*
 * Содержит определения сигналов (SIGKILL, SIGTERM) и функций для работы с ними.
 *
 * Основные элементы:
 *
 * SIGKILL – завершение процесса.
 * SIGTERM – мягкое завершение.
 * SIGSTOP – остановка процесса.
 * Пример использования:
 *
 * kill(pid, SIGKILL); // Завершить процесс
 *
 */
#include <signal.h>

//
// _S(nr) используется для представления сигналов в виде битов (битовых флагов).
//
/*
 * Пример работы _S(nr)
 *
 * Сигнал	        _S(nr)
 * _S(1) (SIGHUP)	1 << (1 - 1) = 1 << 0 = 0x00000001
 * _S(2) (SIGINT)	1 << (2 - 1) = 1 << 1 = 0x00000002
 * _S(3) (SIGQUIT)	1 << (3 - 1) = 1 << 2 = 0x00000004
 *
 */
#define _S(nr) (1<<((nr)-1))

//
// Этот макрос создаёт битовую маску для всех сигналов, которые можно блокировать.
//
/*
 * Разбор по шагам:
 * _S(SIGKILL) создаёт битовую маску для SIGKILL (обычно 9).
 * _S(SIGSTOP) создаёт битовую маску для SIGSTOP (обычно 19).
 * Их побитовое ИЛИ (|) объединяет маски этих двух сигналов.
 * Инверсия (~) создаёт маску, в которой все сигналы, кроме SIGKILL и SIGSTOP, можно блокировать.
 *
 * Пример расчёта _BLOCKABLE
 * 
 * #define SIGKILL 9
 * #define SIGSTOP 19
 *
 * // Вычисляем _S(SIGKILL) и _S(SIGSTOP)
 * _S(9)  = (1 << (9 - 1))  = 0x00000100  // 0000 0000 0000 0000 0000 0001 0000 0000
 * _S(19) = (1 << (19 - 1)) = 0x00040000  // 0000 0000 0000 0100 0000 0000 0000 0000
 *
 * // Побитовое ИЛИ:
 * _S(9) | _S(19) = 0x00040100  // 0000 0000 0000 0100 0000 0001 0000 0000
 *
 * // Инверсия:
 * ~0x00040100 = 0xFFFBFEFF   // 1111 1111 1111 1011 1111 1110 1111 1111
 * 
 * _BLOCKABLE — это битовая маска всех сигналов, которые можно блокировать, за исключением SIGKILL и SIGSTOP (их блокировать нельзя).
 * 
 */
#define _BLOCKABLE (~(_S(SIGKILL) | _S(SIGSTOP)))

//
// Эта функция show_task() в Linux 0.11 выполняет отладочный вывод информации о процессе (task_struct) и анализирует использование стека процесса. 
//
void show_task(int nr,struct task_struct * p) // nr – номер задачи (процесса) в таблице, p – указатель на структуру процесса task_struct.
{
	/*
	 * В Linux 0.11 каждый процесс получает 4 КБ (4096 байт) памяти.
     * struct task_struct хранится в нижней части этих 4 КБ.
     * j = 4096 - sizeof(struct task_struct) — это количество байтов, доступных для стека процесса.
	 * Переменная j получит значение 4096 - sizeof(struct task_struct), а переменная i останется неинициализированной.
	*/
	int i,j = 4096-sizeof(struct task_struct);
    // Выводит в консоль ядра номер задачи nr, PID процесса (p->pid) и его состояние (p->state).
	// Вывод - 2: pid=134, state=0,
	printk("%d: pid=%d, state=%d, ",nr,p->pid,p->state);
	i=0;
	/*
	* Что здесь происходит?
	*
	* p+1 – это указатель на конец структуры task_struct, то есть на начало стека процесса.
	* (char *)(p+1) – преобразует указатель к байтовому представлению.
	* Цикл идёт по всей доступной памяти стека (j байтов).
	* Проверяется, сколько из этих байтов ещё равны нулю (0).
	* i++ увеличивается, пока находится 0 (неиспользованные байты).
	* 
	* Идея: ядро проверяет, сколько памяти в стеке осталось свободной.
	*
	*/
	while (i<j && !((char *)(p+1))[i])
		i++;
	// Вывод - 2: pid=134, state=0, 3500 (of 4072) chars free in kernel stack
	printk("%d (of %d) chars free in kernel stack\n\r",i,j);
}

//
// Эта функция show_stat() предназначена для вывода информации о всех активных процессах в системе.
// Она проходит по массиву task[], в котором хранятся все процессы, и вызывает show_task() для каждого существующего процесса.
//
void show_stat(void)
{
	int i;
	/*
	* task[] — это массив указателей на структуры task_struct. Он определён в sched.h:
	*
	* #define NR_TASKS 64  // NR_TASKS — это максимальное количество процессов. Максимум 64 процесса
	* struct task_struct *task[NR_TASKS] = {NULL, }; // Первым идёт процесс init
	* task[0] — это init (первый процесс, от которого создаются все остальные).
	* task[1], task[2] и т. д. — это другие процессы.
	*/
	for (i=0;i<NR_TASKS;i++)      // Проходим по всем слотам задач
		if (task[i])              // Если процесс существует
			show_task(i,task[i]); // Выводим информацию о нём
	/*
	* Пример вывода show_stat()
	*
	* Если в системе есть несколько процессов, функция может вывести что-то вроде:
	*
	* 0: pid=1, state=0, 3800 (of 4072) chars free in kernel stack
	* 1: pid=2, state=1, 3900 (of 4072) chars free in kernel stack
	* 2: pid=3, state=0, 3600 (of 4072) chars free in kernel stack
	* ...
	*
	* pid=1 — это init, основной процесс.
	* state=0 — процесс выполняется (TASK_RUNNING).
	* 3800 (of 4072) chars free in kernel stack — процесс использовал 272 байта стека.
	*
	*/
}

/*
 * Этот макрос определяет значение для программирования таймера PIT (Programmable Interval Timer) в Linux 0.11.
 * 1193180 — это частота стандартного таймера PIT (Programmable Interval Timer) в Герцах (1.19318 МГц).
 * HZ — это количество прерываний таймера в секунду (в Linux 0.11 это 100, т.е. HZ = 100).
 * LATCH — это количество тактов таймера между прерываниями.
 *
 * Этот макрос вычисляет, через сколько тактов таймера должно произойти одно прерывание, если система работает с частотой HZ (100 прерываний в секунду).
 * Вычислим LATCH для Linux 0.11:
 *
 * LATCH = 1193180 / 100 = 11931
 *
 * Таким образом, прерывания таймера будут происходить каждые 11 931 тактов.
 *
 * Как используется LATCH?
 *
 * Этот макрос используется для программирования канала 0 таймера PIT (порт 0x40), чтобы генерировать HZ прерываний в секунду.
 *
 * Пример кода настройки PIT:
 *
 * outb(0x36, 0x43);                // Устанавливаем режим 3 (Square Wave Mode)
 * outb(LATCH & 0xFF, 0x40);        // Отправляем младший байт
 * outb(LATCH >> 8, 0x40);          // Отправляем старший байт
 *
 * Как это работает?
 *
 * outb(0x36, 0x43); — устанавливает режим периодического прерывания.
 * outb(LATCH & 0xFF, 0x40); — отправляет младший байт LATCH в PIT.
 * outb(LATCH >> 8, 0x40); — отправляет старший байт LATCH в PIT.
 * 
 * Таким образом, таймер начинает генерировать 100 прерываний в секунду, что нужно для работы планировщика процессов (sched.c).
 * 
 * Используется в планировщике процессов, чтобы переключать задачи каждые 10 мс (1 / 100 сек).
 *
 */
#define LATCH (1193180/HZ)

/*
 * extern говорит компилятору, что mem_use() реализована в другом файле, но может использоваться в этом файле.
 * Фактическая реализация mem_use() находится где-то ещё (например, в mm/memory.c или kernel/mem.c).
 * mem_use() - выводит статистику использования памяти.
 */
extern void mem_use(void);

/*
 * timer_interrupt() — обработчик прерываний от таймера
 * 
 * Что делает timer_interrupt()?
 *
 * Это обработчик прерывания IRQ0, который вызывается каждые 10 мс (HZ = 100).
 * Он переключает задачи (schedule()).
 * Обновляет системное время.
 *
 * Где реализована timer_interrupt()?
 *
 * Она написана на ассемблере в kernel/system_call.s:
 * 
 * _timer_interrupt:
 *   push %ds
 *   push %es
 *   push %fs
 *   pushl %eax
 *   pushl %ebx
 *   pushl %ecx
 *   pushl %edx
 *   pushl %esi
 *   pushl %edi
 *   pushl %ebp
 *   call do_timer
 *   jmp ret_from_sys_call
 *
 * Что происходит?
 *
 * Сохраняются все регистры.
 * Вызывается do_timer() (в sched.c), который переключает задачи.
 * Затем происходит возврат (ret_from_sys_call).
 *
 * Где вызывается timer_interrupt()?
 * 
 * В boot/setup.S устанавливается обработчик для IRQ0 (таймер):
 *
 * movl timer_interrupt,0x08(%ebx)  // Прерывание 0x08 -> timer_interrupt
 *
 */
extern int timer_interrupt(void);

/*
 * system_call() — обработчик системных вызовов
 *
 * Что делает system_call()?
 *
 * Это точка входа для всех системных вызовов (sys_*).
 * В eax передаётся номер системного вызова.
 * Вызывается соответствующая функция (например, sys_write(), sys_fork()).
 * Возвращает результат в eax.
 *
 * Где реализована system_call()?
 *
 * В kernel/system_call.s на ассемблере:
 *
 * _system_call:
 *    push %ds
 *    push %es
 *    push %fs
 *    pushl %eax
 *    ...
 *    cmpl $NR_syscalls-1,%eax
 *    jae bad_sys_call
 *    call *sys_call_table(,%eax,4)
 *    ...
 *
 * Что происходит?
 *
 * Сохраняются регистры.
 * Проверяется, является ли eax допустимым номером системного вызова.
 * Вызывается функция из таблицы sys_call_table[].
 *
 * Пример таблицы системных вызовов (sys_call_table[])
 *
 * fn_ptr sys_call_table[] = {
 *    sys_setup,
 *    sys_exit,
 *    sys_fork,
 *    sys_read,
 *    sys_write,
 *    ...
 * };
 *
 * Если в eax передано 4, вызывается sys_write().
 *
 * Где вызывается system_call()?
 *
 * В entry.S прерывание int 0x80 направляет поток исполнения в system_call:
 * 
 * .int80_handler:
 *     call system_call
 *
 * Так пользовательские программы могут делать вызовы ядра, например:
 * 
 * write(1, "Hello, world!\n", 13);
 * Это вызовет sys_write() через int 0x80.
 * 
 * Почему int 0x80?
 *
 * В x86 архитектуре прерывания с 0x00 по 0x1F зарезервированы для процессора (исключения и ошибки).
 * Прерывания 0x20–0xFF можно использовать для программных вызовов.
 * Линус выбрал 0x80 для системных вызовов, потому что:
 * Оно не конфликтует с аппаратными прерываниями.
 * Его легко запомнить.
 * В DOS использовалось int 0x21, но Linux — не DOS.
 *
 *
 * Как работает int 0x80?
 * В пользовательском коде
 * Если программа вызывает системную функцию, например:
 * 
 * write(1, "Hello, world!\n", 13);
 *
 * Где-то внутри стандартной библиотеки (libc) она преобразуется в:
 * movl $4, %eax       # Код системного вызова (sys_write)
 * movl $1, %ebx       # Файл дескриптор (stdout)
 * movl $message, %ecx # Адрес строки
 * movl $13, %edx      # Длина строки
 * int $0x80           # Вызов ядра
 * 
 * Здесь:
 *
 * eax содержит номер системного вызова (например, 4 = sys_write).
 * Остальные регистры передают аргументы (ebx, ecx, edx).
 * int 0x80 переключает управление на ядро.
 *
 * Как ядро обрабатывает int 0x80?
 *
 * Прерывание int 0x80 вызывает обработчик system_call()
 * Код в kernel/system_call.s:
 *
 * .int80_handler:
 *   call system_call
 *
 * Функция system_call() смотрит номер вызова в eax
 *
 * cmpl $NR_syscalls-1,%eax
 * jae bad_sys_call
 * call *sys_call_table(,%eax,4)
 * 
 * Если eax = 4, вызывается sys_write().
 * Ядро выполняет нужную функцию (sys_write, sys_fork, sys_open).
 * Результат записывается обратно в eax и возвращается пользователю.
 * 
 */
extern int system_call(void);

union task_union {
	struct task_struct task;
	char stack[PAGE_SIZE];
};

static union task_union init_task = {INIT_TASK,};

long volatile jiffies=0;
long startup_time=0;
struct task_struct *current = &(init_task.task);
struct task_struct *last_task_used_math = NULL;

struct task_struct * task[NR_TASKS] = {&(init_task.task), };

long user_stack [ PAGE_SIZE>>2 ] ;

struct {
	long * a;
	short b;
	} stack_start = { & user_stack [PAGE_SIZE>>2] , 0x10 };
/*
 *  'math_state_restore()' saves the current math information in the
 * old math state array, and gets the new ones from the current task
 */
void math_state_restore()
{
	if (last_task_used_math == current)
		return;
	__asm__("fwait");
	if (last_task_used_math) {
		__asm__("fnsave %0"::"m" (last_task_used_math->tss.i387));
	}
	last_task_used_math=current;
	if (current->used_math) {
		__asm__("frstor %0"::"m" (current->tss.i387));
	} else {
		__asm__("fninit"::);
		current->used_math=1;
	}
}

/*
 *  'schedule()' is the scheduler function. This is GOOD CODE! There
 * probably won't be any reason to change this, as it should work well
 * in all circumstances (ie gives IO-bound processes good response etc).
 * The one thing you might take a look at is the signal-handler code here.
 *
 *   NOTE!!  Task 0 is the 'idle' task, which gets called when no other
 * tasks can run. It can not be killed, and it cannot sleep. The 'state'
 * information in task[0] is never used.
 */
void schedule(void)
{
	int i,next,c;
	struct task_struct ** p;

/* check alarm, wake up any interruptible tasks that have got a signal */

	for(p = &LAST_TASK ; p > &FIRST_TASK ; --p)
		if (*p) {
			if ((*p)->alarm && (*p)->alarm < jiffies) {
					(*p)->signal |= (1<<(SIGALRM-1));
					(*p)->alarm = 0;
				}
			if (((*p)->signal & ~(_BLOCKABLE & (*p)->blocked)) &&
			(*p)->state==TASK_INTERRUPTIBLE)
				(*p)->state=TASK_RUNNING;
		}

/* this is the scheduler proper: */

	while (1) {
		c = -1;
		next = 0;
		i = NR_TASKS;
		p = &task[NR_TASKS];
		while (--i) {
			if (!*--p)
				continue;
			if ((*p)->state == TASK_RUNNING && (*p)->counter > c)
				c = (*p)->counter, next = i;
		}
		if (c) break;
		for(p = &LAST_TASK ; p > &FIRST_TASK ; --p)
			if (*p)
				(*p)->counter = ((*p)->counter >> 1) +
						(*p)->priority;
	}
	switch_to(next);
}

int sys_pause(void)
{
	current->state = TASK_INTERRUPTIBLE;
	schedule();
	return 0;
}

void sleep_on(struct task_struct **p)
{
	struct task_struct *tmp;

	if (!p)
		return;
	if (current == &(init_task.task))
		panic("task[0] trying to sleep");
	tmp = *p;
	*p = current;
	current->state = TASK_UNINTERRUPTIBLE;
	schedule();
	if (tmp)
		tmp->state=0;
}

void interruptible_sleep_on(struct task_struct **p)
{
	struct task_struct *tmp;

	if (!p)
		return;
	if (current == &(init_task.task))
		panic("task[0] trying to sleep");
	tmp=*p;
	*p=current;
repeat:	current->state = TASK_INTERRUPTIBLE;
	schedule();
	if (*p && *p != current) {
		(**p).state=0;
		goto repeat;
	}
	*p=NULL;
	if (tmp)
		tmp->state=0;
}

void wake_up(struct task_struct **p)
{
	if (p && *p) {
		(**p).state=0;
		*p=NULL;
	}
}

/*
 * OK, here are some floppy things that shouldn't be in the kernel
 * proper. They are here because the floppy needs a timer, and this
 * was the easiest way of doing it.
 */
static struct task_struct * wait_motor[4] = {NULL,NULL,NULL,NULL};
static int  mon_timer[4]={0,0,0,0};
static int moff_timer[4]={0,0,0,0};
unsigned char current_DOR = 0x0C;

int ticks_to_floppy_on(unsigned int nr)
{
	extern unsigned char selected;
	unsigned char mask = 0x10 << nr;

	if (nr>3)
		panic("floppy_on: nr>3");
	moff_timer[nr]=10000;		/* 100 s = very big :-) */
	cli();				/* use floppy_off to turn it off */
	mask |= current_DOR;
	if (!selected) {
		mask &= 0xFC;
		mask |= nr;
	}
	if (mask != current_DOR) {
		outb(mask,FD_DOR);
		if ((mask ^ current_DOR) & 0xf0)
			mon_timer[nr] = HZ/2;
		else if (mon_timer[nr] < 2)
			mon_timer[nr] = 2;
		current_DOR = mask;
	}
	sti();
	return mon_timer[nr];
}

void floppy_on(unsigned int nr)
{
	cli();
	while (ticks_to_floppy_on(nr))
		sleep_on(nr+wait_motor);
	sti();
}

void floppy_off(unsigned int nr)
{
	moff_timer[nr]=3*HZ;
}

void do_floppy_timer(void)
{
	int i;
	unsigned char mask = 0x10;

	for (i=0 ; i<4 ; i++,mask <<= 1) {
		if (!(mask & current_DOR))
			continue;
		if (mon_timer[i]) {
			if (!--mon_timer[i])
				wake_up(i+wait_motor);
		} else if (!moff_timer[i]) {
			current_DOR &= ~mask;
			outb(current_DOR,FD_DOR);
		} else
			moff_timer[i]--;
	}
}

#define TIME_REQUESTS 64

static struct timer_list {
	long jiffies;
	void (*fn)();
	struct timer_list * next;
} timer_list[TIME_REQUESTS], * next_timer = NULL;

void add_timer(long jiffies, void (*fn)(void))
{
	struct timer_list * p;

	if (!fn)
		return;
	cli();
	if (jiffies <= 0)
		(fn)();
	else {
		for (p = timer_list ; p < timer_list + TIME_REQUESTS ; p++)
			if (!p->fn)
				break;
		if (p >= timer_list + TIME_REQUESTS)
			panic("No more time requests free");
		p->fn = fn;
		p->jiffies = jiffies;
		p->next = next_timer;
		next_timer = p;
		while (p->next && p->next->jiffies < p->jiffies) {
			p->jiffies -= p->next->jiffies;
			fn = p->fn;
			p->fn = p->next->fn;
			p->next->fn = fn;
			jiffies = p->jiffies;
			p->jiffies = p->next->jiffies;
			p->next->jiffies = jiffies;
			p = p->next;
		}
	}
	sti();
}

void do_timer(long cpl)
{
	extern int beepcount;
	extern void sysbeepstop(void);

	if (beepcount)
		if (!--beepcount)
			sysbeepstop();

	if (cpl)
		current->utime++;
	else
		current->stime++;

	if (next_timer) {
		next_timer->jiffies--;
		while (next_timer && next_timer->jiffies <= 0) {
			void (*fn)(void);
			
			fn = next_timer->fn;
			next_timer->fn = NULL;
			next_timer = next_timer->next;
			(fn)();
		}
	}
	if (current_DOR & 0xf0)
		do_floppy_timer();
	if ((--current->counter)>0) return;
	current->counter=0;
	if (!cpl) return;
	schedule();
}

int sys_alarm(long seconds)
{
	int old = current->alarm;

	if (old)
		old = (old - jiffies) / HZ;
	current->alarm = (seconds>0)?(jiffies+HZ*seconds):0;
	return (old);
}

int sys_getpid(void)
{
	return current->pid;
}

int sys_getppid(void)
{
	return current->father;
}

int sys_getuid(void)
{
	return current->uid;
}

int sys_geteuid(void)
{
	return current->euid;
}

int sys_getgid(void)
{
	return current->gid;
}

int sys_getegid(void)
{
	return current->egid;
}

int sys_nice(long increment)
{
	if (current->priority-increment>0)
		current->priority -= increment;
	return 0;
}

void sched_init(void)
{
	int i;
	struct desc_struct * p;

	if (sizeof(struct sigaction) != 16)
		panic("Struct sigaction MUST be 16 bytes");
	set_tss_desc(gdt+FIRST_TSS_ENTRY,&(init_task.task.tss));
	set_ldt_desc(gdt+FIRST_LDT_ENTRY,&(init_task.task.ldt));
	p = gdt+2+FIRST_TSS_ENTRY;
	for(i=1;i<NR_TASKS;i++) {
		task[i] = NULL;
		p->a=p->b=0;
		p++;
		p->a=p->b=0;
		p++;
	}
/* Clear NT, so that we won't have troubles with that later on */
	__asm__("pushfl ; andl $0xffffbfff,(%esp) ; popfl");
	ltr(0);
	lldt(0);
	outb_p(0x36,0x43);		/* binary, mode 3, LSB/MSB, ch 0 */
	outb_p(LATCH & 0xff , 0x40);	/* LSB */
	outb(LATCH >> 8 , 0x40);	/* MSB */
	set_intr_gate(0x20,&timer_interrupt);
	outb(inb_p(0x21)&~0x01,0x21);
	set_system_gate(0x80,&system_call);
}
