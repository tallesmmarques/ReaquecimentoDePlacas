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
HANDLE hBlockExProcessoEvent;

int main()
{
    setlocale(LC_ALL, "Portuguese");

    hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) {
        printf("Erro ao obter handle para escrita no console\n");
        exit(EXIT_FAILURE);
    }

    InitializeCriticalSection(&csConsole);

    cc_printf(CCWHITE, "[I] Processo Exibicao de Dados de Processo inicializado\n");

    hTerminateEvent = OpenEvent(EVENT_ALL_ACCESS, TRUE, "TerminateEvent");
    CheckForError(hTerminateEvent);
    hBlockExProcessoEvent = OpenEvent(EVENT_ALL_ACCESS, TRUE, "BlockExProcessoEvent");
    CheckForError(hBlockExProcessoEvent);

    HANDLE hEvents[2] = { hBlockExProcessoEvent, hTerminateEvent };
    DWORD dwRet, numEvent;
    
    do {
        dwRet = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);
        ResetEvent(hBlockExProcessoEvent);
		CheckForError((dwRet >= WAIT_OBJECT_0) && (dwRet < WAIT_OBJECT_0 + 1));
        numEvent = dwRet - WAIT_OBJECT_0;

        if (numEvent == 0) // bloqueio
        {
			ResetEvent(hBlockExProcessoEvent);

			cc_printf(CCWHITE, "Tarefa de Exibicao de Dados de Processo bloqueada\n");
			dwRet = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);
			numEvent = dwRet - WAIT_OBJECT_0;
			if (numEvent == 0)
			{
				ResetEvent(hBlockExProcessoEvent);
				cc_printf(CCWHITE, "Tarefa de Exibicao de Dados de Processo desbloqueada\n");
			}
        }
    } while (numEvent != 1); // ESC precionado no terminal principal

    CloseHandle(hBlockExProcessoEvent);
    CloseHandle(hTerminateEvent);

    cc_printf(CCRED, "[S] Encerrando Processo de Exibição de Dados de Processo\n");
    exit(EXIT_SUCCESS);
}

