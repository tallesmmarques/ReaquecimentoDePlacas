#include <Windows.h>
#include <stdio.h>
#include <string>
#include <conio.h>
#include <stdlib.h>
#include <process.h>

// Constantes
const int ESC = 27;

// Cores e ferramentas para Console
#define CCRED   FOREGROUND_RED   | FOREGROUND_INTENSITY
#define CCBLUE  FOREGROUND_BLUE  | FOREGROUND_INTENSITY
#define CCGREEN FOREGROUND_GREEN | FOREGROUND_INTENSITY
#define CCWHITE FOREGROUND_RED   | FOREGROUND_GREEN     | FOREGROUND_BLUE
HANDLE hOut;
CRITICAL_SECTION csConsole;

// Funções e Threads
unsigned _stdcall ThreadLeituraDados(void*);

double RandReal(double min, double max) {
    return min + double(rand()) / RAND_MAX * (max - min);
}

// Globais
char key;

int main()
{
    HANDLE hThreads[1];
    unsigned dwThreadId;

    hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) {
        printf("Erro ao obter handle para escrita no console\n");
        exit(EXIT_FAILURE);
    }

    InitializeCriticalSection(&csConsole);

    hThreads[0] = (HANDLE)_beginthreadex(
        NULL,
        0,
        &ThreadLeituraDados,
        NULL,
        0,
        &dwThreadId
    );

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

    WaitForSingleObject(hThreads[0], INFINITE);
    CloseHandle(hThreads[0]);

    EnterCriticalSection(&csConsole);
    SetConsoleTextAttribute(hOut, CCRED);
    printf("Saindo da thread principal\n");
    LeaveCriticalSection(&csConsole);

	SetConsoleTextAttribute(hOut, CCWHITE);
    exit(EXIT_SUCCESS);
}

unsigned _stdcall ThreadLeituraDados(void* tArgs) {
	int NSEQ = 1;
    int TIPO = 55;
    double T_ZONA_P, T_ZONA_A, T_ZONA_E, PRESSAO;

	do {
        T_ZONA_P = RandReal(700, 900);
        T_ZONA_A = RandReal(901, 1200);
        T_ZONA_E = RandReal(1201, 1400);
        PRESSAO  = RandReal(10, 12);

		EnterCriticalSection(&csConsole);
		SetConsoleTextAttribute(hOut, CCGREEN);
		printf("%04d$%02d$%04.1f$%04.1f$%04.1f$%02.1f\n", 
            NSEQ, TIPO, T_ZONA_P, T_ZONA_A, T_ZONA_E, PRESSAO);
		LeaveCriticalSection(&csConsole);

        Sleep(1000);
        NSEQ = (NSEQ + 1) % 10000;
    } while (key != ESC);

    EnterCriticalSection(&csConsole);
    SetConsoleTextAttribute(hOut, CCRED);
    printf("Saindo da thread leitura de dados\n");
    LeaveCriticalSection(&csConsole);

    _endthreadex(0);
    return 0;
}
