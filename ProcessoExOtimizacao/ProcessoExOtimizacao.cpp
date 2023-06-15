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
#include "Mensagem.h"

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
HANDLE hBlockExOtimizacaoEvent;
HANDLE hClearConsoleEvent;
HANDLE hNovaMensagemOtimizacao;

HANDLE hArquivoMemoria;
HANDLE hMutexArquivo;
HANDLE hEspacoMemoriaArquivoEvent;
#define MAX_MSG_FILE 50
#define FILE_FULL 1
#define OTIMIZACAO_SIZE 38
const char HeaderInit[] = "00 00 00\n";
const int HeaderSize = strlen(HeaderInit);

#define NO_MESSAGE 1
#define ERROR_ARQUIVO 2
int ReadArquivoMemoria(char* msg, int* msgRestantes)
{
    TCHAR buffer[MAX_MSG];

    SetFilePointer(hArquivoMemoria, 0, 0, FILE_BEGIN);
    if (FALSE == ReadFile(hArquivoMemoria, buffer, HeaderSize, 0, NULL))
    {
		cc_printf(CCRED, "Erro ao ler cabecalho do arquivo\n");
        return ERROR_ARQUIVO;
    }
    buffer[HeaderSize] = '\0';

	std::istringstream iss(buffer);
	std::string token;
    int initLine, lastLine, numMsg;

    std::getline(iss, token, ' ');
    initLine = stoi(token);

    std::getline(iss, token, ' ');
    lastLine = stoi(token);

    std::getline(iss, token, ' ');
    numMsg = stoi(token);

    *msgRestantes = numMsg;
    if (numMsg == 0)
        return NO_MESSAGE;

    SetFilePointer(hArquivoMemoria, HeaderSize + initLine * OTIMIZACAO_SIZE, 0, FILE_BEGIN);
    if (FALSE == ReadFile(hArquivoMemoria, msg, OTIMIZACAO_SIZE, 0, NULL))
    {
		cc_printf(CCRED, "Erro ao ler dados de otimizacao do arquivo\n");
        return ERROR_ARQUIVO;
    }
    msg[OTIMIZACAO_SIZE - 1] = '\0';

    initLine = (initLine + 1) % MAX_MSG_FILE;
    numMsg--;
    sprintf_s(buffer, HeaderSize+1, "%02d %02d %02d\n", initLine, lastLine, numMsg);

    SetFilePointer(hArquivoMemoria, 0, 0, FILE_BEGIN);
    if (FALSE == WriteFile(hArquivoMemoria, buffer, HeaderSize, 0, NULL))
    {
		cc_printf(CCRED, "Erro ao escrever no cabecalho do arquivo\n");
        return ERROR_ARQUIVO;
    }

    return 0;
}

int main()
{
    setlocale(LC_ALL, "Portuguese");

    hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) {
        printf("Erro ao obter handle para escrita no console\n");
        exit(EXIT_FAILURE);
    }

    InitializeCriticalSection(&csConsole);

    cc_printf(CCWHITE, "[I] Processo Exibicao de Dados de Otimizacao inicializado\n");

    hTerminateEvent = OpenEvent(EVENT_ALL_ACCESS, TRUE, "TerminateEvent");
    CheckForError(hTerminateEvent);
    hBlockExOtimizacaoEvent = OpenEvent(EVENT_ALL_ACCESS, TRUE, "BlockExOtimizacaoEvent");
    CheckForError(hBlockExOtimizacaoEvent);
    hClearConsoleEvent = OpenEvent(EVENT_ALL_ACCESS, TRUE, "ClearConsoleEvent");
    CheckForError(hClearConsoleEvent);
    hNovaMensagemOtimizacao = OpenEvent(EVENT_ALL_ACCESS, TRUE, "NovaMensagemOtimizacao");
    CheckForError(hNovaMensagemOtimizacao);

    hArquivoMemoria = CreateFile("otimizacao.data", 
        GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    CheckForError(hArquivoMemoria != INVALID_HANDLE_VALUE);
    hMutexArquivo = OpenMutex(MUTEX_ALL_ACCESS, TRUE, "MutexArquivo");
    CheckForError(hMutexArquivo);
    hEspacoMemoriaArquivoEvent = OpenEvent(EVENT_ALL_ACCESS, TRUE, "EspacoMemoriaArquivoEvent");
    CheckForError(hEspacoMemoriaArquivoEvent);

	HANDLE hEvents[4] = { hBlockExOtimizacaoEvent, hTerminateEvent, hClearConsoleEvent, hNovaMensagemOtimizacao };
    DWORD dwRet, numEvent;
    int status;
	int msgRestantes = 0;
    char msg[MAX_MSG];
    
    BOOL bloqueada = FALSE;
    do {
		dwRet = WaitForMultipleObjects(4, hEvents, FALSE, INFINITE);
		numEvent = dwRet - WAIT_OBJECT_0;

        if (bloqueada && numEvent == 0) // desbloqueio
        {
			ResetEvent(hBlockExOtimizacaoEvent);
			bloqueada = FALSE;
			cc_printf(CCWHITE, "Tarefa de Exibicao de Dados de Otimizacao desbloqueada\n");
            SetEvent(hNovaMensagemOtimizacao);
        }
        else if (!bloqueada && numEvent == 0) // bloqueio
        {
			ResetEvent(hBlockExOtimizacaoEvent);
			bloqueada = TRUE;
			cc_printf(CCWHITE, "Tarefa de Exibicao de Dados de Otimizacao bloqueada\n");
        }

        if (numEvent == 2) // limpar terminal
		{
            ResetEvent(hClearConsoleEvent);
            system("cls");
		}

        if (numEvent == 3) // nova mensagem
        {
            if (!bloqueada)
            {
                do {
                    WaitForSingleObject(hMutexArquivo, INFINITE);
                    status = ReadArquivoMemoria(msg, &msgRestantes);
                    ReleaseMutex(hMutexArquivo);

                    if (status == ERROR_ARQUIVO)
                        SetEvent(hTerminateEvent);
                    if (status == NO_MESSAGE) break;

                    Mensagem mensagem(msg);
                    if (mensagem.TIPO == 01) // Otimizacao
                        cc_printf(CCBLUE, "%s\n", mensagem.getMensagemFormatada().c_str());
                    else
                        cc_printf(CCRED, "Tipo desconhecido de mensagem recebida - %s\n",
                            mensagem.getMensagemFormatada().c_str());
                } while (msgRestantes > 0);
            }
            PulseEvent(hEspacoMemoriaArquivoEvent);
            ResetEvent(hNovaMensagemOtimizacao);
        }
    } while (numEvent != 1); // ESC precionado no terminal principal

    CloseHandle(hBlockExOtimizacaoEvent);
    CloseHandle(hTerminateEvent);
    CloseHandle(hClearConsoleEvent);

    cc_printf(CCRED, "[S] Encerrando Processo de Exibicao de Dados de Otimizacao\n");
    exit(EXIT_SUCCESS);
}

