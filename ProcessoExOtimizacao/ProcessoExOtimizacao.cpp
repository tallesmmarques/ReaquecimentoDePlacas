#define WIN32_LEAN_AND_MEAN 

#include <windows.h>
#include <stdio.h>
#include <string>
#include <conio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <process.h>
#include "Mensagem.h"

/// Constantes
#define ESC 27                          // valor da tecla ESC
#define SIZE_MSG 50                     // número máximo de caractéres nas mensagens
#define MAX_MSG_FILE 50                 // numero máximo de mensagens no arquivo circular
#define FILE_FULL 1                     // erro quando o arquivo circular esta cheio
#define OTIMIZACAO_SIZE 38              // tamanho da mensagem de otimização com quebra de linha
const char HeaderInit[] = "00 00 00\n"; // cabeçalho do arquivo circular
const int HeaderSize = strlen(HeaderInit);  // tamanho do cabeçalho do arquivo circular

/// Utilitários
#define _CHECKERROR    1                // Ativa função CheckForError
#include "CheckForError.h"

typedef unsigned (WINAPI* CAST_FUNCTION)(LPVOID);
typedef unsigned* CAST_LPDWORD;

/// Cores e ferramentas para Console
#define CCRED       FOREGROUND_RED
#define CCREDI      FOREGROUND_RED   | FOREGROUND_INTENSITY
#define CCBLUE      FOREGROUND_BLUE  | FOREGROUND_INTENSITY
#define CCGREEN     FOREGROUND_GREEN | FOREGROUND_INTENSITY
#define CCWHITE     FOREGROUND_RED   | FOREGROUND_GREEN     | FOREGROUND_BLUE
#define CCPURPLE    FOREGROUND_RED   | FOREGROUND_BLUE      | FOREGROUND_INTENSITY
HANDLE hOut;
CRITICAL_SECTION csConsole;             // seção crítica para escrever no terminal

/// printf colorido, e com exclusão mútua
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

/// Evento que sinaliza o termino de toda a aplicação
HANDLE hTerminateEvent;

/// Evento de bloqueio
HANDLE hBlockExOtimizacaoEvent;         // bloqueia tarefa de exibição de dados de otimização

/// Evento de limpeza da janela de console de otimização
HANDLE hClearConsoleEvent;              // limpa console de otimização quando pressionado 'x'

/// Handles para gerenciar arquivo circular em disco
HANDLE hArquivoCircular;                // handle para arquivo
HANDLE hMutexArquivo;                   // mutex para exclusão mutua entre operações no arquivo
HANDLE hNovoEspacoArquivoCircularEvent; // foi liberado espaço no arquivo circular
HANDLE hNovaMensagemOtimizacao;         // nova mensagem escrita no arquivo circular 

#define NO_MESSAGE 1                    // erro quando não há mensagem para ser lida no arquivo circular
#define ERROR_ARQUIVO 2                 // erro na leitura do arquivo circular
int LerArquivoCircular(char*, int*);    // função para ler mensagens no arquivo circular

int main()
{
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

    hArquivoCircular = CreateFile("otimizacao.data", 
        GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    CheckForError(hArquivoCircular != INVALID_HANDLE_VALUE);
    hMutexArquivo = OpenMutex(MUTEX_ALL_ACCESS, TRUE, "MutexArquivo");
    CheckForError(hMutexArquivo);
    hNovoEspacoArquivoCircularEvent = OpenEvent(EVENT_ALL_ACCESS, TRUE, "NovoEspacoArquivoCircularEvent");
    CheckForError(hNovoEspacoArquivoCircularEvent);

	HANDLE hEvents[4] = { hBlockExOtimizacaoEvent, hTerminateEvent, hClearConsoleEvent, hNovaMensagemOtimizacao };
    DWORD dwRet, numEvent;
    int status;
	int msgRestantes = 0;
    char msg[SIZE_MSG];
    
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
                    status = LerArquivoCircular(msg, &msgRestantes);
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
            PulseEvent(hNovoEspacoArquivoCircularEvent);
            ResetEvent(hNovaMensagemOtimizacao);
        }
    } while (numEvent != 1); // ESC precionado no terminal principal

    CloseHandle(hBlockExOtimizacaoEvent);
    CloseHandle(hTerminateEvent);
    CloseHandle(hClearConsoleEvent);

    cc_printf(CCRED, "[S] Encerrando Processo de Exibicao de Dados de Otimizacao\n");
    exit(EXIT_SUCCESS);
}

int LerArquivoCircular(char* msg, int* msgRestantes)
{
    TCHAR buffer[SIZE_MSG];

    // leitura do cabeçalho
    SetFilePointer(hArquivoCircular, 0, 0, FILE_BEGIN);
    if (FALSE == ReadFile(hArquivoCircular, buffer, HeaderSize, 0, NULL))
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

    // leitura dos dados
    SetFilePointer(hArquivoCircular, HeaderSize + initLine * OTIMIZACAO_SIZE, 0, FILE_BEGIN);
    if (FALSE == ReadFile(hArquivoCircular, msg, OTIMIZACAO_SIZE, 0, NULL))
    {
		cc_printf(CCRED, "Erro ao ler dados de otimizacao do arquivo\n");
        return ERROR_ARQUIVO;
    }
    msg[OTIMIZACAO_SIZE - 1] = '\0';

    // atualização do cabeçalho
    initLine = (initLine + 1) % MAX_MSG_FILE;
    numMsg--;
    sprintf_s(buffer, HeaderSize+1, "%02d %02d %02d\n", initLine, lastLine, numMsg);

    SetFilePointer(hArquivoCircular, 0, 0, FILE_BEGIN);
    if (FALSE == WriteFile(hArquivoCircular, buffer, HeaderSize, 0, NULL))
    {
		cc_printf(CCRED, "Erro ao escrever no cabecalho do arquivo\n");
        return ERROR_ARQUIVO;
    }

    return 0;
}
