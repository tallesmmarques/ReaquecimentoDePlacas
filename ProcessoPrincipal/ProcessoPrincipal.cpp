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
#define CCRED       FOREGROUND_RED
#define CCREDI      FOREGROUND_RED   | FOREGROUND_INTENSITY
#define CCBLUE      FOREGROUND_BLUE  | FOREGROUND_INTENSITY
#define CCGREEN     FOREGROUND_GREEN | FOREGROUND_INTENSITY
#define CCWHITE     FOREGROUND_RED   | FOREGROUND_GREEN     | FOREGROUND_BLUE
#define CCPURPLE    FOREGROUND_RED   | FOREGROUND_BLUE      | FOREGROUND_INTENSITY
HANDLE hOut;
CRITICAL_SECTION csConsole;

// Funções, Threads e Eventos
void WINAPI ThreadLeituraTeclado(LPVOID);
void WINAPI ThreadLeituraDados(LPVOID);
void WINAPI ThreadLeituraDadosAlarmes(LPVOID);
void WINAPI ThreadCapturaProcesso(LPVOID);
void WINAPI ThreadCapturaOtimizacao(LPVOID);

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

HANDLE hTerminateEvent;

HANDLE hBlockLeituraEvent;
HANDLE hBlockCapProcessoEvent;
HANDLE hBlockCapOtimizacaoEvent;

HANDLE hSemMemoriaLeitura;
HANDLE hNovosDadosProcessoEvent;
HANDLE hNovosDadosOtimizacaoEvent;

// Globais
ListaCircular listaCircular;

int main()
{
    HANDLE hThreadLeituraDados;
    HANDLE hThreadLeituraTeclado;
    HANDLE hThreadCapturaProcesso;
    HANDLE hThreadCapturaOtimizacao;

    setlocale(LC_ALL, "Portuguese");

    hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) {
        printf("Erro ao obter handle para escrita no console\n");
        exit(EXIT_FAILURE);
    }

    InitializeCriticalSection(&csConsole);

    hTerminateEvent = CreateEvent(NULL, TRUE, FALSE, "TerminateEvent");
    CheckForError(hTerminateEvent);
    hBlockLeituraEvent = CreateEvent(NULL, TRUE, FALSE, "BlockLeituraEvent");
    CheckForError(hBlockLeituraEvent);
    hBlockCapProcessoEvent = CreateEvent(NULL, TRUE, FALSE, "BlockCapProcessoEvent");
    CheckForError(hBlockCapProcessoEvent);
    hBlockCapOtimizacaoEvent = CreateEvent(NULL, TRUE, FALSE, "BlockCapOtimizacaoEvent");
    CheckForError(hBlockCapOtimizacaoEvent);

    hSemMemoriaLeitura = CreateSemaphore(NULL, 0, 1, "MemoriaLeitura");
    CheckForError(hSemMemoriaLeitura);
    hNovosDadosProcessoEvent = CreateEvent(NULL, TRUE, FALSE, "NovosDadosProcessoEvent");
    CheckForError(hNovosDadosProcessoEvent);
    hNovosDadosOtimizacaoEvent = CreateEvent(NULL, TRUE, FALSE, "NovosDadosOtimizacaoEvent");
    CheckForError(hNovosDadosOtimizacaoEvent);

    hThreadLeituraDados = (HANDLE)_beginthreadex(
        NULL,
        0,
        (CAST_FUNCTION) ThreadLeituraDados,
        (LPVOID) NULL,
        0,
        (CAST_LPDWORD) NULL
    );
    if (hThreadLeituraDados) {
		cc_printf(CCWHITE, "[C] Thread Leitura de Dados criada\n");
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
        (CAST_LPDWORD) NULL
    );
    if (hThreadLeituraTeclado) {
		cc_printf(CCWHITE, "[C] Thread Leitura do Teclado criada\n");
    } else {
		cc_printf(CCRED, "Erro na criação da thread Leitura do Teclado!\n");
        exit(0);
    }

    hThreadCapturaProcesso = (HANDLE)_beginthreadex(
        NULL,
        0,
        (CAST_FUNCTION) ThreadCapturaProcesso,
        (LPVOID) NULL,
        0,
        (CAST_LPDWORD) NULL
    );
    if (hThreadCapturaProcesso) {
		cc_printf(CCWHITE, "[C] Thread Captura de Dados de Processo criada\n");
    } else {
		cc_printf(CCRED, "Erro na criação da thread Captura de Dados de Processo!\n");
        exit(0);
    }

    hThreadCapturaOtimizacao = (HANDLE)_beginthreadex(
        NULL,
        0,
        (CAST_FUNCTION) ThreadCapturaOtimizacao,
        (LPVOID) NULL,
        0,
        (CAST_LPDWORD) NULL
    );
    if (hThreadCapturaOtimizacao) {
		cc_printf(CCWHITE, "[C] Thread Captura de Dados de Otimizacao criada\n");
    } else {
		cc_printf(CCRED, "Erro na criação da thread Captura de Dados de Otimizacao!\n");
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

    cc_printf(CCRED, "[S] Saindo da thread Principal\n");
    exit(EXIT_SUCCESS);
}

void WINAPI ThreadLeituraTeclado(LPVOID tArgs) 
{
	cc_printf(CCWHITE, "[I] Thread Leitura do Teclado inicializada\n");

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
       case 'o':
           SetEvent(hBlockCapProcessoEvent);
           break;
       case 'p':
           SetEvent(hBlockCapOtimizacaoEvent);
           break;
       case ESC:
           SetEvent(hTerminateEvent);
           break;
       }
   } while (key != ESC);

    cc_printf(CCRED, "[S] Saindo da thread Leitura do Teclado\n");

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
	cc_printf(CCWHITE, "[I] Thread Leitura de Dados inicializada\n");

    HANDLE hThreadLeituraDadosAlarme = (HANDLE)_beginthreadex(
        NULL,
        0,
        (CAST_FUNCTION) ThreadLeituraDadosAlarmes,
        (LPVOID) NULL,
        0,
        (CAST_LPDWORD) NULL
    );
    if (hThreadLeituraDadosAlarme) {
		cc_printf(CCWHITE, "[C] Thread Leitura de Dados para Alarmes criada\n");
    } else {
		cc_printf(CCRED, "Erro na criação da thread Leitura de Dados para Alarmes!\n");
        exit(0);
    }

    char msg[MAX_MSG];
    int timePeriod = 1; // período de 1 segundo

    HANDLE hEvents[3] = { hBlockLeituraEvent, hTerminateEvent, hSemMemoriaLeitura };
    DWORD dwRet;
    DWORD numEvent;
    int memoria_ret;

	do {
        dwRet = WaitForMultipleObjects(2, hEvents, FALSE, 1000*timePeriod);
		numEvent = dwRet - WAIT_OBJECT_0;

        if (dwRet == WAIT_TIMEOUT) 
        {
            // Mensagem de Dados de Processos -------------------------------------------
		    genDadosProcesso(msg);
			memoria_ret = listaCircular.guardarDadoProcesso(msg);

            if (memoria_ret == MEMORY_FULL)
            {
                cc_printf(CCPURPLE, "[Leitura] Lista circular cheia\n");

                do {
					dwRet = WaitForMultipleObjects(3, hEvents, FALSE, INFINITE);
					numEvent = dwRet - WAIT_OBJECT_0;

					if (numEvent == 1) break;
					else if (numEvent == 0) // evento de bloqueio
					{
						ResetEvent(hBlockLeituraEvent);

						cc_printf(CCWHITE, "Tarefa de leitura de dados bloqueada\n");
						dwRet = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);
						numEvent = dwRet - WAIT_OBJECT_0;
						if (numEvent == 0)
						{
							ResetEvent(hBlockLeituraEvent);
							cc_printf(CCWHITE, "Tarefa de leitura de dados desbloqueada\n");
						}
					}
                    else if (numEvent == 2) // vaga livre
                    {
						memoria_ret = listaCircular.guardarDadoProcesso(msg);
                    }
                } while (memoria_ret == MEMORY_FULL);

                if (numEvent == 1) break;
            }
			//cc_printf(CCBLUE, "[Leitura] Dado de processo armazenado\n");
            SetEvent(hNovosDadosProcessoEvent);

            // Mensagem de Dados de Otimização ------------------------------------------
		    genDadosOtimizacao(msg);
			memoria_ret = listaCircular.guardarDadoOtimizacao(msg);

            if (memoria_ret == MEMORY_FULL)
            {
                cc_printf(CCPURPLE, "[Leitura] Lista circular cheia\n");

                do {
					dwRet = WaitForMultipleObjects(3, hEvents, FALSE, INFINITE);
					numEvent = dwRet - WAIT_OBJECT_0;

					if (numEvent == 1) break;
					else if (numEvent == 0) // evento de bloqueio
					{
						ResetEvent(hBlockLeituraEvent);

						cc_printf(CCWHITE, "Tarefa de leitura de dados bloqueada\n");
						dwRet = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);
						numEvent = dwRet - WAIT_OBJECT_0;
						if (numEvent == 0)
						{
							ResetEvent(hBlockLeituraEvent);
							cc_printf(CCWHITE, "Tarefa de leitura de dados desbloqueada\n");
						}
					}
                    else if (numEvent == 2) // vaga livre
                    {
						memoria_ret = listaCircular.guardarDadoOtimizacao(msg);
                    }
                } while (memoria_ret == MEMORY_FULL);

                if (numEvent == 1) break;
            }
			//cc_printf(CCBLUE, "[Leitura] Dado de otimizacao armazenado\n");
            SetEvent(hNovosDadosOtimizacaoEvent);
        }

        if (numEvent == 0) // evento de bloqueio
        {
			ResetEvent(hBlockLeituraEvent);

			cc_printf(CCWHITE, "Tarefa de leitura de dados bloqueada\n");
			dwRet = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);
			numEvent = dwRet - WAIT_OBJECT_0;
			if (numEvent == 0)
			{
				ResetEvent(hBlockLeituraEvent);
				cc_printf(CCWHITE, "Tarefa de leitura de dados desbloqueada\n");
			}
        }
    } while (numEvent != 1); // loop até evento TermLeitura 

    WaitForSingleObject(hThreadLeituraDadosAlarme, INFINITE);

    cc_printf(CCRED, "[S] Saindo da thread Leitura de Dados\n");

    _endthreadex(0);
}

void WINAPI ThreadLeituraDadosAlarmes(LPVOID)
{
	cc_printf(CCWHITE, "[I] Thread Leitura de Dados de Alarme inicializada\n");

    char msg[MAX_MSG];
    int timePeriod = 1; // período de 1 segundo

    HANDLE hEvents[2] = { hBlockLeituraEvent, hTerminateEvent };
    DWORD dwRet;
    DWORD numEvent;

    do {
        dwRet = WaitForMultipleObjects(2, hEvents, FALSE, 1000*timePeriod);
        numEvent = dwRet - WAIT_OBJECT_0;

        if (dwRet == WAIT_TIMEOUT)
        {
            genAlarme(msg);
            cc_printf(CCREDI, "[Alarme] %s\n", msg);
        }
        else if (numEvent == 0) // evento de bloqueio
        {
			ResetEvent(hBlockLeituraEvent);

			//cc_printf(CCBLUE, "Tarefa de leitura de dados bloqueada\n");
			dwRet = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);
			numEvent = dwRet - WAIT_OBJECT_0;
			if (numEvent == 0)
			{
				ResetEvent(hBlockLeituraEvent);
				//cc_printf(CCBLUE, "Tarefa de leitura de dados desbloqueada\n");
			}
        }
    } while (numEvent != 1);

    cc_printf(CCRED, "[S] Saindo da thread Leitura de Dados de Alarme\n");
    _endthreadex(0);
}

void WINAPI ThreadCapturaProcesso(LPVOID)
{
	cc_printf(CCWHITE, "[I] Thread Captura de Dados de Processo inicializada\n");

    HANDLE hEvents[3] = { hBlockCapProcessoEvent, hTerminateEvent, hNovosDadosProcessoEvent };
    DWORD dwRet;
    DWORD numEvent;

    int ret;
    char msg[MAX_MSG];

    do {
        dwRet = WaitForMultipleObjects(3, hEvents, FALSE, INFINITE);
        numEvent = dwRet - WAIT_OBJECT_0;

        if (numEvent == 2)
        {
            ResetEvent(hNovosDadosProcessoEvent);
            do {
			    ret = listaCircular.lerDadoProcesso(msg);
                if (ret == MEMORY_EMPTY) break;

				cc_printf(CCGREEN, "[Processo] %s\n", msg);
				ReleaseSemaphore(hSemMemoriaLeitura, 1, NULL);
            } while (ret != MEMORY_EMPTY);
        }
        else if (numEvent == 0)
        {
			ResetEvent(hBlockCapProcessoEvent);

			cc_printf(CCWHITE, "Tarefa de Captura de Dados de Processo bloqueada\n");
			dwRet = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);
			numEvent = dwRet - WAIT_OBJECT_0;
			if (numEvent == 0)
			{
				ResetEvent(hBlockCapProcessoEvent);
				cc_printf(CCWHITE, "Tarefa de Captura de Dados de Processo desbloqueada\n");
                SetEvent(hNovosDadosProcessoEvent);
			}
        }
    } while (numEvent != 1);

    cc_printf(CCRED, "[S] Saindo da thread Captura de Dados de Processo\n");
    _endthreadex(0);
}

void WINAPI ThreadCapturaOtimizacao(LPVOID)
{
	cc_printf(CCWHITE, "[I] Thread Captura de Dados de Otimizacao inicializada\n");

    HANDLE hEvents[3] = { hBlockCapOtimizacaoEvent, hTerminateEvent, hNovosDadosOtimizacaoEvent };
    DWORD dwRet;
    DWORD numEvent;

    int ret;
    char msg[MAX_MSG];

    do {
        dwRet = WaitForMultipleObjects(3, hEvents, FALSE, INFINITE);
        numEvent = dwRet - WAIT_OBJECT_0;

        if (numEvent == 2)
        {
            ResetEvent(hNovosDadosOtimizacaoEvent);
            do {
			    ret = listaCircular.lerDadoOtimizacao(msg);
                if (ret == MEMORY_EMPTY) break;

				cc_printf(CCBLUE, "[Otimizacao] %s\n", msg);
				ReleaseSemaphore(hSemMemoriaLeitura, 1, NULL);
            } while (ret != MEMORY_EMPTY);
        }
        else if (numEvent == 0)
        {
			ResetEvent(hBlockCapOtimizacaoEvent);

			cc_printf(CCWHITE, "Tarefa de Captura de Dados de Otimizacao bloqueada\n");
			dwRet = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);
			numEvent = dwRet - WAIT_OBJECT_0;
			if (numEvent == 0)
			{
				ResetEvent(hBlockCapOtimizacaoEvent);
				cc_printf(CCWHITE, "Tarefa de Captura de Dados de Otimizacao desbloqueada\n");
                SetEvent(hNovosDadosOtimizacaoEvent);
			}
        }
    } while (numEvent != 1);

    cc_printf(CCRED, "[S] Saindo da thread Captura de Dados de Otimizacao\n");
    _endthreadex(0);
}
