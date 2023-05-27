#include <Windows.h>
#include <stdio.h>
#include <string>
#include <conio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <process.h>

// Cores e ferramentas para Console
#define CCRED   FOREGROUND_RED   | FOREGROUND_INTENSITY
#define CCBLUE  FOREGROUND_BLUE  | FOREGROUND_INTENSITY
#define CCGREEN FOREGROUND_GREEN | FOREGROUND_INTENSITY
#define CCWHITE FOREGROUND_RED   | FOREGROUND_GREEN     | FOREGROUND_BLUE

// Utilitários
typedef unsigned (WINAPI* CAST_FUNCTION)(LPVOID);
typedef unsigned* CAST_LPDWORD;

// Constantes
static const int ESC = 27;
static const int MAX_MSG = 50;

void WINAPI ThreadLeituraTeclado(void*);


