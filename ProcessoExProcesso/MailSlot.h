#pragma once

#include <Windows.h>

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

