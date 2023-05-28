#define WIN32_LEAN_AND_MEAN 

#include <windows.h>
#include <stdio.h>
#include <string>
#include <conio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <process.h>
#include <math.h>
#include <locale.h>

#include "ListaCircular.h"

// Constantes
#define ESC 27
#define MAX_MSG 50

// Utilitários
#define _CHECKERROR    1        // Ativa função CheckForError
#include "CheckForError.h"

typedef unsigned (WINAPI* CAST_FUNCTION)(LPVOID);
typedef unsigned* CAST_LPDWORD;

// Cores e ferramentas para Console
#define CCRED   FOREGROUND_RED   | FOREGROUND_INTENSITY
#define CCBLUE  FOREGROUND_BLUE  | FOREGROUND_INTENSITY
#define CCGREEN FOREGROUND_GREEN | FOREGROUND_INTENSITY
#define CCWHITE FOREGROUND_RED   | FOREGROUND_GREEN     | FOREGROUND_BLUE
HANDLE hOut;
CRITICAL_SECTION csConsole;

// Funções, Threads e Eventos
void WINAPI ThreadLeituraTeclado(LPVOID);
void WINAPI ThreadLeituraDados(LPVOID);

double RandReal(double min, double max) 
{
    return min + double(rand()) / RAND_MAX * (max - min);
}

// printf colorido, e com exclusão mútua
void cc_printf(const int color, const char* format, ...)
{
	char buf[512];
	va_list args;
	va_start(args, format);
	vsprintf_s(buf, 512, format, args);
	va_end(args);

	EnterCriticalSection(&csConsole);
	SetConsoleTextAttribute(hOut, color);
	printf(buf);
	LeaveCriticalSection(&csConsole);
}

HANDLE hBlockLeituraEvent;
HANDLE hTermLeituraEvent;

// Globais
ListaCircular listaCircular;

int main()
{
    HANDLE hThreadLeituraDados;
    HANDLE hThreadLeituraTeclado;
    unsigned dwThreadId;

    setlocale(LC_ALL, "Portuguese");

    hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) {
        printf("Erro ao obter handle para escrita no console\n");
        exit(EXIT_FAILURE);
    }

    InitializeCriticalSection(&csConsole);

    hBlockLeituraEvent = CreateEvent(NULL, TRUE, FALSE, "BlockLeituraEvent");
    CheckForError(hBlockLeituraEvent);
    hTermLeituraEvent = CreateEvent(NULL, TRUE, FALSE, "TermLeituraEvent");
    CheckForError(hTermLeituraEvent);

    hThreadLeituraDados = (HANDLE)_beginthreadex(
        NULL,
        0,
        (CAST_FUNCTION) ThreadLeituraDados,
        (LPVOID) NULL,
        0,
        (CAST_LPDWORD) & dwThreadId
    );
    if (hThreadLeituraDados) {
		cc_printf(CCWHITE, "Thread Leitura de Dados criada\n");
    } else {
		cc_printf(CCRED, "Erro na criação da thread Leitura de Dados!\n");
        exit(0);
    }

    hThreadLeituraTeclado = (HANDLE)_beginthreadex(
        NULL,
        0,
        (CAST_FUNCTION) ThreadLeituraTeclado,
        (LPVOID) NULL,
        0,
        (CAST_LPDWORD) & dwThreadId
    );
    if (hThreadLeituraTeclado) {
		cc_printf(CCWHITE, "Thread Leitura do Teclado criada\n");
    } else {
		cc_printf(CCRED, "Erro na criação da thread Leitura do Teclado!\n");
        exit(0);
    }

    DWORD dwRet;
    const DWORD numThreads = 2;
    HANDLE hThreads[numThreads];
    hThreads[0] = hThreadLeituraDados;
    hThreads[1] = hThreadLeituraTeclado;

    dwRet = WaitForMultipleObjects(numThreads, hThreads, TRUE, INFINITE);
    CheckForError((dwRet >= WAIT_OBJECT_0) && (dwRet < WAIT_OBJECT_0 + numThreads));

    CloseHandle(hThreadLeituraDados);
    CloseHandle(hThreadLeituraTeclado);

    cc_printf(CCRED, "Saindo da thread Principal\n");
    exit(EXIT_SUCCESS);
}

void WINAPI ThreadLeituraTeclado(LPVOID tArgs) 
{
	cc_printf(CCWHITE, "Thread Leitura do Teclado inicializada\n");

    char key;

    int ret;
    char buf[MAX_MSG];

   do {
       key = _getch();

       switch (key)
       {
       case 'd':
           SetEvent(hBlockLeituraEvent);
           break;
       case 'l':
           ret = listaCircular.lerDadoProcesso(buf);
           if (ret == MEMORY_EMPTY)
               cc_printf(CCBLUE, "Nao ha dados de processos disponiveis\n");
           else 
               cc_printf(CCBLUE, "%s\n", buf);

           break;
       case 'k':
           ret = listaCircular.lerDadoOtimizacao(buf);
           if (ret == MEMORY_EMPTY)
           {
               cc_printf(CCBLUE, "Nao ha dados de otimizacao disponiveis\n");
           }
           else 
               cc_printf(CCBLUE, "%s\n", buf);

           break;
       case ESC:
           SetEvent(hTermLeituraEvent);
           break;
       }
   } while (key != ESC);

    cc_printf(CCRED, "Saindo da thread Leitura do Teclado\n");

    _endthreadex(0);
}

void genDadosProcesso(char* msg)
{
	static int NSEQ = 1;

    int TIPO = 55;
    double T_ZONA_P, T_ZONA_A, T_ZONA_E, PRESSAO;
    SYSTEMTIME TIMESTAMP;

	T_ZONA_P = RandReal(700, 900);
	T_ZONA_A = RandReal(901, 1200);
	T_ZONA_E = RandReal(1201, 1400);
	PRESSAO  = RandReal(10, 12);
	GetLocalTime(&TIMESTAMP);

    if (T_ZONA_A < 1000) // mantém a mensagem fixa em 42 caracteres
    {
		sprintf_s(msg, MAX_MSG, "%04d$%02d$%04.1f$0%04.1f$%04.1f$%02.1f$%02d:%02d:%02d", 
			NSEQ, TIPO, T_ZONA_P, T_ZONA_A, T_ZONA_E, PRESSAO,
			TIMESTAMP.wHour, TIMESTAMP.wMinute, TIMESTAMP.wSecond);
    }
    else
    {
		sprintf_s(msg, MAX_MSG, "%04d$%02d$%04.1f$%04.1f$%04.1f$%02.1f$%02d:%02d:%02d", 
			NSEQ, TIPO, T_ZONA_P, T_ZONA_A, T_ZONA_E, PRESSAO,
			TIMESTAMP.wHour, TIMESTAMP.wMinute, TIMESTAMP.wSecond);
    }

	NSEQ = 1 + (NSEQ % 9999);
}
void genAlarme(char* msg)
{
	static int NSEQ = 1;

    int TIPO = 99;
    int CODIGO;
    SYSTEMTIME TIMESTAMP;

    CODIGO = round(RandReal(0, 99));
	GetLocalTime(&TIMESTAMP);

	sprintf_s(msg, MAX_MSG, "%04d$%02d$%02d$%02d:%02d:%02d", 
		NSEQ, TIPO, CODIGO,
		TIMESTAMP.wHour, TIMESTAMP.wMinute, TIMESTAMP.wSecond);

	NSEQ = 1 + (NSEQ % 9999);
}
void genDadosOtimizacao(char* msg) {
	static int NSEQ = 1;

    int TIPO = 01;
    double T_ZONA_P, T_ZONA_A, T_ZONA_E;
    SYSTEMTIME TIMESTAMP;

	T_ZONA_P = RandReal(700, 900);
	T_ZONA_A = RandReal(901, 1200);
	T_ZONA_E = RandReal(1201, 1400);
	GetLocalTime(&TIMESTAMP);

    if (T_ZONA_A < 1000) // mantém a mensagem fixa em 42 caracteres
    {
		sprintf_s(msg, MAX_MSG, "%04d$%02d$%04.1f$0%04.1f$%04.1f$%02d:%02d:%02d", 
			NSEQ, TIPO, T_ZONA_P, T_ZONA_A, T_ZONA_E,
			TIMESTAMP.wHour, TIMESTAMP.wMinute, TIMESTAMP.wSecond);
    }
    else
    {
		sprintf_s(msg, MAX_MSG, "%04d$%02d$%04.1f$%04.1f$%04.1f$%02d:%02d:%02d", 
			NSEQ, TIPO, T_ZONA_P, T_ZONA_A, T_ZONA_E,
			TIMESTAMP.wHour, TIMESTAMP.wMinute, TIMESTAMP.wSecond);
    }

	NSEQ = 1 + (NSEQ % 9999);
}
void WINAPI ThreadLeituraDados(LPVOID tArgs)
{
	cc_printf(CCWHITE, "Thread Leitura de Dados inicializada\n");

    char msg[MAX_MSG];
    int timePeriod = 2; // período de 1 segundo

    HANDLE hEvents[2] = { hBlockLeituraEvent, hTermLeituraEvent };
    DWORD dwRet;
    DWORD numEvent;
    int memoria_ret;

	do {
        dwRet = WaitForMultipleObjects(2, hEvents, FALSE, 1000*timePeriod);
		numEvent = dwRet - WAIT_OBJECT_0;

        if (dwRet == WAIT_TIMEOUT) 
        {
            genDadosProcesso(msg);
            memoria_ret = listaCircular.guardarDadoProcesso(msg);
            cc_printf(CCBLUE, "Dado de processo gerado - %d\n", memoria_ret);

            genAlarme(msg);
            memoria_ret = listaCircular.guardarDadoOtimizacao(msg);
            cc_printf(CCBLUE, "Dado de otimizacao gerado - %d\n", memoria_ret);

            genDadosOtimizacao(msg);
			//cc_printf(CCGREEN, msg);
        }
        else if (numEvent == 0) // evento de bloqueio
        {
			ResetEvent(hBlockLeituraEvent);

			cc_printf(CCBLUE, "Tarefa de leitura de dados bloqueada\n");
			dwRet = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);
			numEvent = dwRet - WAIT_OBJECT_0;
			if (numEvent == 0)
			{
				ResetEvent(hBlockLeituraEvent);
				cc_printf(CCBLUE, "Tarefa de leitura de dados desbloqueada\n");
			}
        }
    } while (numEvent != 1); // loop até evento TermLeitura 
	ResetEvent(hTermLeituraEvent);

    cc_printf(CCRED, "Saindo da thread Leitura de Dados\n");

    _endthreadex(0);
}
