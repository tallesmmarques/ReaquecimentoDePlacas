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

#define NO_MESSAGE 1
#define ERROR_MAILSLOT_INFO 2
#define ERROR_MAILSLOT_READ 3
int ReadMailSlot(HANDLE hSlot, char* msg, int* msgRestantes)
{
    BOOL status;
    DWORD tamanhoProximaMensagem, numMensagens;
    DWORD cAllMessages;
    OVERLAPPED ov;
    TCHAR buffer[MAX_MSG];

    tamanhoProximaMensagem = numMensagens = 0;

    status = GetMailslotInfo(hSlot, NULL, &tamanhoProximaMensagem, &numMensagens, NULL);
    if (!status)
    {
        cc_printf(CCRED, "Erro ao obter informacoes do mailslot, codigo = %d\n", GetLastError());
        return ERROR_MAILSLOT_INFO;
    }

    if (tamanhoProximaMensagem == MAILSLOT_NO_MESSAGE)
        return NO_MESSAGE;

    status = ReadFile(hSlot, msg, tamanhoProximaMensagem, 0, NULL);
    if (!status)
    {
        cc_printf(CCRED, "Erro ao ler mensagem na mailslot, codigo = %d\n", GetLastError());
        return ERROR_MAILSLOT_READ;
    }

    *msgRestantes = numMensagens - 1;

    return 0;
}

HANDLE hTerminateEvent;
HANDLE hBlockExOtimizacaoEvent;
HANDLE hClearConsoleEvent;
HANDLE hMailSlotOtimizacaoCreatedEvent;
HANDLE hMailSlot;
HANDLE hNovaMensagemOtimizacao;

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
    hMailSlotOtimizacaoCreatedEvent = OpenEvent(EVENT_ALL_ACCESS, TRUE, "MailSlotOtimizacaoCreatedEvent");
    CheckForError(hMailSlotOtimizacaoCreatedEvent);
    hNovaMensagemOtimizacao = OpenEvent(EVENT_ALL_ACCESS, TRUE, "NovaMensagemOtimizacao");
    CheckForError(hNovaMensagemOtimizacao);

    hMailSlot = CreateMailslot("\\\\.\\mailslot\\otimizacao", 0, MAILSLOT_WAIT_FOREVER, NULL);
    CheckForError(hMailSlot);
    SetEvent(hMailSlotOtimizacaoCreatedEvent);

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
                    status = ReadMailSlot(hMailSlot, msg, &msgRestantes);
                    if (status == ERROR_MAILSLOT_INFO || status == ERROR_MAILSLOT_READ)
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
            ResetEvent(hNovaMensagemOtimizacao);
        }
    } while (numEvent != 1); // ESC precionado no terminal principal

    CloseHandle(hBlockExOtimizacaoEvent);
    CloseHandle(hTerminateEvent);
    CloseHandle(hClearConsoleEvent);

    cc_printf(CCRED, "[S] Encerrando Processo de Exibicao de Dados de Otimizacao\n");
    exit(EXIT_SUCCESS);
}

