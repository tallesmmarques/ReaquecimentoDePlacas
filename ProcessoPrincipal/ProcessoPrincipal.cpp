#define WIN32_LEAN_AND_MEAN 

#include <Windows.h>
#include <stdio.h>
#include <string>
#include <conio.h>
#include <stdlib.h>
#include <process.h>

// Utilitários
#define _CHECKERROR    1        // Ativa função CheckForError
#include "CheckForError.h"
typedef unsigned (WINAPI* CAST_FUNCTION)(LPVOID);
typedef unsigned* CAST_LPDWORD;

// Constantes
static const int ESC = 27;
static const int MAX_MSG = 50;

// Cores e ferramentas para Console
#define CCRED   FOREGROUND_RED   | FOREGROUND_INTENSITY
#define CCBLUE  FOREGROUND_BLUE  | FOREGROUND_INTENSITY
#define CCGREEN FOREGROUND_GREEN | FOREGROUND_INTENSITY
#define CCWHITE FOREGROUND_RED   | FOREGROUND_GREEN     | FOREGROUND_BLUE
HANDLE hOut;
CRITICAL_SECTION csConsole;

// Funções e Threads
void WINAPI ThreadLeituraDados(void*);

double RandReal(double min, double max) {
    return min + double(rand()) / RAND_MAX * (max - min);
}

// Globais
char key;

int main()
{
    HANDLE hThreadLeituraDados;
    unsigned dwThreadId;

    hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) {
        printf("Erro ao obter handle para escrita no console\n");
        exit(EXIT_FAILURE);
    }

    InitializeCriticalSection(&csConsole);

    hThreadLeituraDados = (HANDLE)_beginthreadex(
        NULL,
        0,
        (CAST_FUNCTION) ThreadLeituraDados,
        (LPVOID) NULL,
        0,
        (CAST_LPDWORD) & dwThreadId
    );
    if (hThreadLeituraDados) {
		EnterCriticalSection(&csConsole);
		SetConsoleTextAttribute(hOut, CCWHITE);
		printf("Thread Leitura de Dados criada\n");
		LeaveCriticalSection(&csConsole);
    }
    else {
		EnterCriticalSection(&csConsole);
		SetConsoleTextAttribute(hOut, CCRED);
		printf("Erro na criação da thread Leitura de Dados!\n");
		LeaveCriticalSection(&csConsole);
        exit(0);
    }

    do {
        key = _getch();
        if (key == 'c')
            system("CLS");

        if (key != ESC) {
			EnterCriticalSection(&csConsole);
			SetConsoleTextAttribute(hOut, CCBLUE);
			printf("Tecla: %c\n", key);
			LeaveCriticalSection(&csConsole);
        }
    } while (key != ESC);

    WaitForSingleObject(hThreadLeituraDados, INFINITE);
    CloseHandle(hThreadLeituraDados);

    EnterCriticalSection(&csConsole);
    SetConsoleTextAttribute(hOut, CCRED);
    printf("Saindo da thread principal\n");
    LeaveCriticalSection(&csConsole);

	SetConsoleTextAttribute(hOut, CCWHITE);
    exit(EXIT_SUCCESS);
}

void WINAPI ThreadLeituraDados(void* tArgs) {
    char msg[MAX_MSG];

	int NSEQ = 1;
    int TIPO = 55;
    double T_ZONA_P, T_ZONA_A, T_ZONA_E, PRESSAO;
    SYSTEMTIME TIMESTAMP;

	do {
        T_ZONA_P = RandReal(700, 900);
        T_ZONA_A = RandReal(901, 1200);
        T_ZONA_E = RandReal(1201, 1400);
        PRESSAO  = RandReal(10, 12);
        GetLocalTime(&TIMESTAMP);

		sprintf_s(msg, MAX_MSG, "%04d$%02d$%04.1f$%04.1f$%04.1f$%02.1f$%02d:%02d:%02d\n", 
            NSEQ, TIPO, T_ZONA_P, T_ZONA_A, T_ZONA_E, PRESSAO,
            TIMESTAMP.wHour, TIMESTAMP.wMinute, TIMESTAMP.wSecond);

		EnterCriticalSection(&csConsole);
		SetConsoleTextAttribute(hOut, CCGREEN);
        printf(msg);
		LeaveCriticalSection(&csConsole);

        Sleep(1000);
        NSEQ = (NSEQ + 1) % 10000;
    } while (key != ESC);

    EnterCriticalSection(&csConsole);
    SetConsoleTextAttribute(hOut, CCRED);
    printf("Saindo da thread leitura de dados\n");
    LeaveCriticalSection(&csConsole);

    _endthreadex(0);
}
