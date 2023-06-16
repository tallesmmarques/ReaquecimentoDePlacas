#define WIN32_LEAN_AND_MEAN 

#include <windows.h>
#include <stdio.h>
#include <string>
#include <conio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <process.h>
#include <math.h>
#include "Mensagem.h"

// Constantes
#define ESC 27
#define SIZE_MSG 50

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
    TCHAR buffer[SIZE_MSG];

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
HANDLE hBlockExProcessoEvent;
HANDLE hMailSlotProcessoCriadoEvent;
HANDLE hMailSlot;
HANDLE hNovaMensagemProcesso;

int main()
{
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
    hMailSlotProcessoCriadoEvent = OpenEvent(EVENT_ALL_ACCESS, TRUE, "MailSlotProcessoCriadoEvent");
    CheckForError(hMailSlotProcessoCriadoEvent);
    hNovaMensagemProcesso = OpenEvent(EVENT_ALL_ACCESS, TRUE, "NovaMensagemProcesso");
    CheckForError(hNovaMensagemProcesso);

    hMailSlot = CreateMailslot("\\\\.\\mailslot\\processo", 0, MAILSLOT_WAIT_FOREVER, NULL);
    CheckForError(hMailSlot);
    SetEvent(hMailSlotProcessoCriadoEvent);

    HANDLE hEvents[3] = { hBlockExProcessoEvent, hTerminateEvent, hNovaMensagemProcesso };
    DWORD dwRet, numEvent;
    int status;
	int msgRestantes = 0;
    char msg[SIZE_MSG];
    
    do {
        dwRet = WaitForMultipleObjects(3, hEvents, FALSE, INFINITE);
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
                SetEvent(hNovaMensagemProcesso);
			}
        }

        if (numEvent == 2) // nova mensagem
        {
            do {
                status = ReadMailSlot(hMailSlot, msg, &msgRestantes);
                if (status == ERROR_MAILSLOT_INFO || status == ERROR_MAILSLOT_READ)
                    SetEvent(hTerminateEvent);
                if (status == NO_MESSAGE) break;

                Mensagem mensagem(msg);
                if (mensagem.TIPO == 99) // Alarme
                    cc_printf(CCREDI, "%s\n", mensagem.getMensagemFormatada().c_str());
                else if (mensagem.TIPO == 55) // Processo
                    cc_printf(CCGREEN, "%s\n", mensagem.getMensagemFormatada().c_str());
                else
                    cc_printf(CCRED, "Tipo desconhecido de mensagem recebida - %s\n",
                        mensagem.getMensagemFormatada().c_str());
            } while (msgRestantes > 0);
            ResetEvent(hNovaMensagemProcesso);
        }
    } while (numEvent != 1); // ESC precionado no terminal principal

    CloseHandle(hBlockExProcessoEvent);
    CloseHandle(hTerminateEvent);
    CloseHandle(hMailSlot);
    CloseHandle(hMailSlotProcessoCriadoEvent);
    CloseHandle(hNovaMensagemProcesso);

    cc_printf(CCRED, "[S] Encerrando Processo de Exibição de Dados de Processo\n");
    exit(EXIT_SUCCESS);
}

