#define WIN32_LEAN_AND_MEAN 
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1
#define __WIN32_WINNT 0x0500

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
HANDLE hBlockExProcessoEvent;
HANDLE hBlockExOtimizacaoEvent;

HANDLE hClearConsoleEvent;

CRITICAL_SECTION csListaCircularIO;
HANDLE hNovosDadosProcessoEvent;
HANDLE hNovosDadosOtimizacaoEvent;
HANDLE hMemoryFullEvent;
HANDLE hMemorySpaceEvent;

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
    InitializeCriticalSection(&csListaCircularIO);

    hTerminateEvent = CreateEvent(NULL, TRUE, FALSE, "TerminateEvent");
    CheckForError(hTerminateEvent);
    hBlockLeituraEvent = CreateEvent(NULL, TRUE, FALSE, "BlockLeituraEvent");
    CheckForError(hBlockLeituraEvent);
    hBlockCapProcessoEvent = CreateEvent(NULL, TRUE, FALSE, "BlockCapProcessoEvent");
    CheckForError(hBlockCapProcessoEvent);
    hBlockCapOtimizacaoEvent = CreateEvent(NULL, TRUE, FALSE, "BlockCapOtimizacaoEvent");
    CheckForError(hBlockCapOtimizacaoEvent);
    hBlockExProcessoEvent = CreateEvent(NULL, TRUE, FALSE, "BlockExProcessoEvent");
    CheckForError(hBlockExProcessoEvent);
    hBlockExOtimizacaoEvent = CreateEvent(NULL, TRUE, FALSE, "BlockExOtimizacaoEvent");
    CheckForError(hBlockExOtimizacaoEvent);

    hMemoryFullEvent = CreateEvent(NULL, TRUE, FALSE, "MemoryFullEvent");
    CheckForError(hMemoryFullEvent);
    hMemorySpaceEvent = CreateEvent(NULL, FALSE, FALSE, "MemorySpaceEvent");
    CheckForError(hMemorySpaceEvent);
    hNovosDadosProcessoEvent = CreateEvent(NULL, TRUE, FALSE, "NovosDadosProcessoEvent");
    CheckForError(hNovosDadosProcessoEvent);
    hNovosDadosOtimizacaoEvent = CreateEvent(NULL, TRUE, FALSE, "NovosDadosOtimizacaoEvent");
    CheckForError(hNovosDadosOtimizacaoEvent);

    hClearConsoleEvent = CreateEvent(NULL, TRUE, FALSE, "ClearConsoleEvent");
    CheckForError(hClearConsoleEvent);

    DWORD status;

	STARTUPINFO siExP;
    PROCESS_INFORMATION piExP;

    ZeroMemory(&siExP, sizeof(siExP));
    siExP.cb = sizeof(siExP);
    ZeroMemory(&piExP, sizeof(piExP));
    //status = CreateProcess(
    //    NULL,
    //    "ProcessoExProcesso.exe",
    //    NULL,
    //    NULL,
    //    TRUE,
    //    CREATE_NEW_CONSOLE,
    //    NULL,
    //    NULL,
    //    &siExP,
    //    &piExP
    //);

	STARTUPINFO siExO;
    PROCESS_INFORMATION piExO;

    ZeroMemory(&siExO, sizeof(siExO));
    siExO.cb = sizeof(siExO);
    ZeroMemory(&piExO, sizeof(piExO));
    //status = CreateProcess(
    //    NULL,
    //    "ProcessoExOtimizacao.exe",
    //    NULL,
    //    NULL,
    //    TRUE,
    //    CREATE_NEW_CONSOLE,
    //    NULL,
    //    NULL,
    //    &siExO,
    //    &piExO
    //);

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

    //const DWORD numProcess = 2;
    //HANDLE hProcess[numProcess];
    //hProcess[0] = piExP.hProcess;
    //hProcess[1] = piExO.hProcess;

    //dwRet = WaitForMultipleObjects(numProcess, hProcess, TRUE, INFINITE);
    //CheckForError((dwRet >= WAIT_OBJECT_0) && (dwRet < WAIT_OBJECT_0 + numProcess));
    //cc_printf(CCRED, "[S] Processo Exibicao de Dados de Processo encerrado \n");
    //cc_printf(CCRED, "[S] Processo Exibicao de Dados de Otimizacao encerrado \n");

    //CloseHandle(piExP.hProcess);
    //CloseHandle(piExP.hThread);
    //CloseHandle(piExO.hProcess);
    //CloseHandle(piExO.hThread);

    const DWORD numThreads = 4;
    HANDLE hThreads[numThreads];
    hThreads[0] = hThreadLeituraDados;
    hThreads[1] = hThreadLeituraTeclado;
    hThreads[2] = hThreadCapturaProcesso;
    hThreads[3] = hThreadCapturaOtimizacao;

    dwRet = WaitForMultipleObjects(numThreads, hThreads, TRUE, INFINITE);
    CheckForError((dwRet >= WAIT_OBJECT_0) && (dwRet < WAIT_OBJECT_0 + numThreads));

    CloseHandle(hThreadLeituraDados);
    CloseHandle(hThreadLeituraTeclado);
    CloseHandle(hThreadCapturaProcesso);
    CloseHandle(hThreadCapturaOtimizacao);

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
       case 'q':
           SetEvent(hBlockExProcessoEvent);
           break;
       case 's':
           SetEvent(hBlockExOtimizacaoEvent);
           break;
       case 'x':
           SetEvent(hClearConsoleEvent);
           break;
       case ESC:
           SetEvent(hTerminateEvent);
           break;
       }
   } while (key != ESC);

    cc_printf(CCRED, "[S] Saindo da thread Leitura do Teclado\n");

    _endthreadex(0);
}

int NSEQ_Processo = 1;
int NSEQ_Otimizacao = 1;
int NSEQ_Alarme = 1;

void genDadosProcesso(char*);
void genAlarme(char*);
void genDadosOtimizacao(char*);

void CALLBACK LerProcesso(PVOID, BOOLEAN);
void CALLBACK LerOtimizacao(PVOID, BOOLEAN);
void CALLBACK LerAlarme(PVOID, BOOLEAN);

HANDLE hTimerQueue;
HANDLE hTimerProcesso, hTimerOtimizacao, hTimerAlarme;
const int msPeriodProcesso      = 1000;
const int msPeriodOtimizacao    = 1000;
const int msPeriodAlarme        = 1000;

// Impede que Timer sejam apagados novamente ou criados novamente
BOOL ProcessoRodando    = FALSE;
BOOL OtimizacaoRodando  = FALSE;
BOOL AlarmeRodando      = FALSE;

void InitializeTimers(BOOL apenasAlarme = FALSE)
{
	BOOL status;

    if (!ProcessoRodando && !apenasAlarme) 
    {
        ProcessoRodando = TRUE;
		status = CreateTimerQueueTimer(&hTimerProcesso, hTimerQueue, (WAITORTIMERCALLBACK) LerProcesso,
									   NULL, msPeriodProcesso, msPeriodProcesso, WT_EXECUTEDEFAULT);
		if (!status){
			cc_printf(CCRED, "[Leitura] Erro em TimerProcesso! Codigo = %d)\n", GetLastError());
			exit(EXIT_FAILURE);
		}
    }

    if (!OtimizacaoRodando && !apenasAlarme)
    {
        OtimizacaoRodando = TRUE;
		status = CreateTimerQueueTimer(&hTimerOtimizacao, hTimerQueue, (WAITORTIMERCALLBACK) LerOtimizacao,
									   NULL, msPeriodOtimizacao, msPeriodOtimizacao, WT_EXECUTEDEFAULT);
		if (!status){
			cc_printf(CCRED, "[Leitura] Erro em TimerOtimizacao! Codigo = %d)\n", GetLastError());
			exit(EXIT_FAILURE);
		}
    }

    if (!AlarmeRodando)
    {
        AlarmeRodando = TRUE;
		status = CreateTimerQueueTimer(&hTimerAlarme, hTimerQueue, (WAITORTIMERCALLBACK) LerAlarme,
									   NULL, msPeriodAlarme, msPeriodAlarme, WT_EXECUTEDEFAULT);
		if (!status){
			cc_printf(CCRED, "[Leitura] Erro em TimerAlarme! Codigo = %d)\n", GetLastError());
			exit(EXIT_FAILURE);
		}
    }
}
void StopTimers(BOOL apenasDados = FALSE)
{
	BOOL status;

    if (ProcessoRodando)
    {
        ProcessoRodando = FALSE;
		status = DeleteTimerQueueTimer(hTimerQueue, hTimerProcesso, NULL); 
		if (!status && GetLastError() != ERROR_IO_PENDING)
        {
			cc_printf(CCRED, "[Leitura] Erro ao deletar TimerProcesso! Codigo = %d)\n", GetLastError());
			exit(EXIT_FAILURE);
		}
    }

    if (OtimizacaoRodando)
    {
        OtimizacaoRodando = FALSE;
		status = DeleteTimerQueueTimer(hTimerQueue, hTimerOtimizacao, NULL); 
		if (!status && GetLastError() != ERROR_IO_PENDING)
        {
			cc_printf(CCRED, "[Leitura] Erro ao deletar TimerOtimizacao! Codigo = %d)\n", GetLastError());
			exit(EXIT_FAILURE);
		}
    }

    if (AlarmeRodando && !apenasDados)
    {
        AlarmeRodando = FALSE;
		status = DeleteTimerQueueTimer(hTimerQueue, hTimerAlarme, NULL); 
		if (!status && GetLastError() != ERROR_IO_PENDING)
        {
			cc_printf(CCRED, "[Leitura] Erro ao deletar TimerAlarme! Codigo = %d)\n", GetLastError());
			exit(EXIT_FAILURE);
		}
    }
}

void WINAPI ThreadLeituraDados(LPVOID tArgs)
{
	cc_printf(CCWHITE, "[I] Thread Leitura de Dados inicializada\n");

	BOOL status;
    BOOL blockAll = FALSE;
    BOOL blockDados = FALSE;

	hTimerQueue = CreateTimerQueue();
	if (hTimerQueue == NULL){
        cc_printf(CCRED, "[Leitura] Falha em CreateTimerQueue! Codigo =%d)\n", GetLastError());
        exit(EXIT_FAILURE);
    }
    InitializeTimers();
    
    HANDLE hEvents[3] = { hBlockLeituraEvent, hTerminateEvent, hMemoryFullEvent };
	HANDLE hMemoriaEvents[3] = { hBlockLeituraEvent, hTerminateEvent, hMemorySpaceEvent };
    DWORD dwRet;
    DWORD numEvent;
    do {
		dwRet = WaitForMultipleObjects(3, hEvents, FALSE, INFINITE);
		numEvent = dwRet - WAIT_OBJECT_0;

        if (numEvent == 2) // hMemoryFullEvent - bloqueio por memória cheia
        {
            if (blockDados == FALSE)
            {
                blockDados = TRUE;
                StopTimers(TRUE);
				cc_printf(CCPURPLE, "[Leitura] Lista circular cheia\n");
            }
            if (blockDados == TRUE)
            {
                dwRet = WaitForMultipleObjects(3, hMemoriaEvents, FALSE, INFINITE);
                numEvent = dwRet - WAIT_OBJECT_0;

                if (numEvent == 2) // hMemorySpaceEvent - espaço liberado na memória
                {
					ResetEvent(hMemoryFullEvent);
					blockDados = FALSE;
					if (blockAll != TRUE) InitializeTimers(); // liberado para leitura mas thread está bloqueada
                }
            }
        }

        if (numEvent == 0) // hBlockLeituraEvent - bloqueio da thread pelo usuário
        {
			ResetEvent(hBlockLeituraEvent);

            if (blockAll == FALSE)
            {
                blockAll = TRUE;
				StopTimers();
				cc_printf(CCWHITE, "Tarefa de leitura de dados bloqueada\n");
            }
            else
            {
                blockAll = FALSE;
                if (blockDados) InitializeTimers(true); // apenas alarme pois a memória está cheia
                else InitializeTimers();
				cc_printf(CCWHITE, "Tarefa de leitura de dados desbloqueada\n");
            }
        }
    } while (numEvent != 1); // hTerminateEvent - ESC pressionado
 
	if (!DeleteTimerQueueEx(hTimerQueue, NULL))
        cc_printf(CCRED, "[Leitura] Falha em DeleteTimerQueue! Codigo = %d\n", GetLastError());

    cc_printf(CCRED, "[S] Saindo da thread Leitura de Dados\n");
    _endthreadex(0);
}

void CALLBACK LerProcesso(PVOID nTimerID, BOOLEAN TimerOrWaitFired)
{
    int memoria_ret;
    char msg[MAX_MSG];

	genDadosProcesso(msg);

	EnterCriticalSection(&csListaCircularIO);
	memoria_ret = listaCircular.guardarDadoProcesso(msg);
	LeaveCriticalSection(&csListaCircularIO);

    if (memoria_ret == MEMORY_FULL)
    {
        if (ProcessoRodando == FALSE) return; // já foi avisado que a memória está cheia
        SetEvent(hMemoryFullEvent);
    }

    if (memoria_ret != MEMORY_FULL) 
    {
        NSEQ_Processo = 1 + (NSEQ_Processo % 9999);
		SetEvent(hNovosDadosProcessoEvent);
		//cc_printf(CCGREEN, "[Leitura] Dado de processo armazenado\n");
    }
}
void CALLBACK LerOtimizacao(PVOID nTimerID, BOOLEAN TimerOrWaitFired)
{
    int memoria_ret;
    char msg[MAX_MSG];

	genDadosOtimizacao(msg);

	EnterCriticalSection(&csListaCircularIO);
	memoria_ret = listaCircular.guardarDadoOtimizacao(msg);
	LeaveCriticalSection(&csListaCircularIO);

    if (memoria_ret == MEMORY_FULL)
    {
        if (OtimizacaoRodando == FALSE) return; // já foi avisado que a memória está cheia
        SetEvent(hMemoryFullEvent);
    }

    if (memoria_ret != MEMORY_FULL) 
    {
        NSEQ_Otimizacao = 1 + (NSEQ_Otimizacao % 9999);
		SetEvent(hNovosDadosOtimizacaoEvent);
		//cc_printf(CCBLUE, "[Leitura] Dado de otimizacao armazenado\n");
    }
}
void CALLBACK LerAlarme(PVOID nTimerID, BOOLEAN TimerOrWaitFired)
{
    HANDLE hEvents[2] = { hBlockLeituraEvent, hTerminateEvent };
    DWORD dwRet;
    DWORD numEvent;

    char msg[MAX_MSG];

	genAlarme(msg);
	cc_printf(CCREDI, "[Leitura] %s\n", msg);
}

void genDadosProcesso(char* msg)
{
    int TIPO = 55;
    double T_ZONA_P, T_ZONA_A, T_ZONA_E, PRESSAO;
    SYSTEMTIME TIMESTAMP;

	T_ZONA_P = RandReal(700, 900);
	T_ZONA_A = RandReal(901, 1200);
	T_ZONA_E = RandReal(1201, 1400);
	PRESSAO  = RandReal(10, 12);
	GetLocalTime(&TIMESTAMP);

	sprintf_s(msg, MAX_MSG, "%04d$%02d$%06.1f$%06.1f$%06.1f$%04.1f$%02d:%02d:%02d", 
		NSEQ_Processo, TIPO, T_ZONA_P, T_ZONA_A, T_ZONA_E, PRESSAO,
		TIMESTAMP.wHour, TIMESTAMP.wMinute, TIMESTAMP.wSecond);
}
void genDadosOtimizacao(char* msg) 
{
    int TIPO = 01;
    double T_ZONA_P, T_ZONA_A, T_ZONA_E;
    SYSTEMTIME TIMESTAMP;

	T_ZONA_P = RandReal(700, 900);
	T_ZONA_A = RandReal(901, 1200);
	T_ZONA_E = RandReal(1201, 1400);
	GetLocalTime(&TIMESTAMP);

	sprintf_s(msg, MAX_MSG, "%04d$%02d$%06.1f$%06.1f$%06.1f$%02d:%02d:%02d", 
		NSEQ_Otimizacao, TIPO, T_ZONA_P, T_ZONA_A, T_ZONA_E,
		TIMESTAMP.wHour, TIMESTAMP.wMinute, TIMESTAMP.wSecond);
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
				EnterCriticalSection(&csListaCircularIO);
			    ret = listaCircular.lerDadoProcesso(msg);
				LeaveCriticalSection(&csListaCircularIO);
                if (ret == MEMORY_EMPTY) break;

				cc_printf(CCGREEN, "[Processo] %s\n", msg);
                PulseEvent(hMemorySpaceEvent);
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
				EnterCriticalSection(&csListaCircularIO);
			    ret = listaCircular.lerDadoOtimizacao(msg);
				LeaveCriticalSection(&csListaCircularIO);
                if (ret == MEMORY_EMPTY) break;

				cc_printf(CCBLUE, "[Otimizacao] %s\n", msg);
                PulseEvent(hMemorySpaceEvent);
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

