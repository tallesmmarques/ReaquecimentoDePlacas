#define WIN32_LEAN_AND_MEAN 
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1
#define __WIN32_WINNT 0x0500

#include <windows.h>
#include <stdio.h>
#include <string>
#include <sstream>
#include <iostream>
#include <conio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <process.h>

#include "ListaCircular.h"
#include "Common.h"

/// Constantes
#define ESC 27                          // valor da tecla ESC
#define SIZE_MSG 50                     // número máximo de caractéres nas mensagens
#define MAX_MSG_FILE 50                 // numero máximo de mensagens no arquivo circular
#define FILE_FULL 1                     // erro quando o arquivo circular esta cheio
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

/// printf colorido e com exclusão mútua
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

/// Threads
void WINAPI ThreadLeituraTeclado(LPVOID);
void WINAPI ThreadLeituraDados(LPVOID);
void WINAPI ThreadCapturaProcesso(LPVOID);
void WINAPI ThreadCapturaOtimizacao(LPVOID);

/// Evento que sinaliza o termino de toda a aplicação
HANDLE hTerminateEvent;

/// Eventos de bloqueios 
HANDLE hBlockLeituraEvent;              // bloqueia tarefa de leitura de dados
HANDLE hBlockCapProcessoEvent;          // bloqueia tarefa de captura de dados de processos
HANDLE hBlockCapOtimizacaoEvent;        // bloqueia tarefa de captura de dados de otimizacao
HANDLE hBlockExProcessoEvent;           // bloqueia tarefa de exibição de dados de processos
HANDLE hBlockExOtimizacaoEvent;         // bloqueia tarefa de exibição de dados de otimização

/// Evento de limpeza da janela de console de otimização
HANDLE hClearConsoleEvent;              // limpa console de otimização quando pressionado 'x'

/// Handles para gerenciar lista circular em memória 
CRITICAL_SECTION csListaCircularIO;     // seção crítica para gravar ou ler a memoria circular
HANDLE hNovosDadosProcessoEvent;        // um novo dado de processo foi salvo na memoria circular
HANDLE hNovosDadosOtimizacaoEvent;      // um novo dado de otimizacao foi salvo na memoria circular
HANDLE hMemoriaCircularCheiaEvent;      // a memoria circular esta cheia
HANDLE hNovoEspacoMemoriaCircularEvent; // foi liberado espaço na memoria circular
HANDLE hNovaMensagemProcesso;           // nova mensagem escrita no mailslot de processos

/// Handles para gerenciar comunicação entre processos
HANDLE hMailSlotProcessoCriadoEvent;    // mail slot foi criado pelo processo de exibição de processo

/// Handles para gerenciar arquivo circular em disco
HANDLE hArquivoCircular;                // handle para arquivo
HANDLE hMutexArquivo;                   // mutex para exclusão mutua entre operações no arquivo
HANDLE hNovoEspacoArquivoCircularEvent; // foi liberado espaço no arquivo circular
HANDLE hNovaMensagemOtimizacao;         // nova mensagem escrita no arquivo circular 

/// Instância da lista circular
ListaCircular listaCircular;            // objeto da classe ListaCircular usado para 
                                        // armazenar alarmes e dados de processos

// --------------------------------------------------------------------------------------
// --| Tarefa Principal |----------------------------------------------------------------
// --------------------------------------------------------------------------------------

int main()
{
    // handles para threads
    HANDLE hThreadLeituraDados;
    HANDLE hThreadLeituraTeclado;
    HANDLE hThreadCapturaProcesso;
    HANDLE hThreadCapturaOtimizacao;

    // ferramentas para console
    hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) {
        printf("Erro ao obter handle para escrita no console\n");
        exit(EXIT_FAILURE);
    }
    InitializeCriticalSection(&csConsole);
    InitializeCriticalSection(&csListaCircularIO);

    // handles para threads
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

    // handles para memoria circular
    hMemoriaCircularCheiaEvent = CreateEvent(NULL, TRUE, FALSE, "MemoriaCircularCheia");
    CheckForError(hMemoriaCircularCheiaEvent);
    hNovoEspacoMemoriaCircularEvent = CreateEvent(NULL, FALSE, FALSE, "NovoEspacoMemoriaCircularEvent");
    CheckForError(hNovoEspacoMemoriaCircularEvent);
    hNovosDadosProcessoEvent = CreateEvent(NULL, TRUE, FALSE, "NovosDadosProcessoEvent");
    CheckForError(hNovosDadosProcessoEvent);
    hNovosDadosOtimizacaoEvent = CreateEvent(NULL, TRUE, FALSE, "NovosDadosOtimizacaoEvent");
    CheckForError(hNovosDadosOtimizacaoEvent);

    // handle de limpeza do console de otimização
    hClearConsoleEvent = CreateEvent(NULL, TRUE, FALSE, "ClearConsoleEvent");
    CheckForError(hClearConsoleEvent);

    // handles para comunicação entre processos
    hMailSlotProcessoCriadoEvent = CreateEvent(NULL, TRUE, FALSE, "MailSlotProcessoCriadoEvent");
    CheckForError(hMailSlotProcessoCriadoEvent);
    hNovaMensagemProcesso = CreateEvent(NULL, TRUE, FALSE, "NovaMensagemProcesso");
    CheckForError(hNovaMensagemProcesso);

    // handles para arquivo circular
    hMutexArquivo = CreateMutex(NULL, FALSE, "MutexArquivo");
    CheckForError(hMutexArquivo);
    hNovoEspacoArquivoCircularEvent = CreateEvent(NULL, FALSE, FALSE, "NovoEspacoArquivoCircularEvent");
    CheckForError(hNovoEspacoArquivoCircularEvent);
    hNovaMensagemOtimizacao = CreateEvent(NULL, TRUE, FALSE, "NovaMensagemOtimizacao");
    CheckForError(hNovaMensagemOtimizacao);
    // ---
    hArquivoCircular = CreateFile("otimizacao.data",
        GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    CheckForError(hArquivoCircular != INVALID_HANDLE_VALUE);
    WriteFile(hArquivoCircular, HeaderInit, HeaderSize, 0, NULL);

    DWORD status;

    // criação do processo de exibição de dados de processos
	STARTUPINFO siExP;
    PROCESS_INFORMATION piExP;
    ZeroMemory(&siExP, sizeof(siExP));
    siExP.cb = sizeof(siExP);
    ZeroMemory(&piExP, sizeof(piExP));
    status = CreateProcess(
        NULL,
        "ProcessoExProcesso.exe",
        NULL,
        NULL,
        TRUE,
        CREATE_NEW_CONSOLE,
        NULL,
        NULL,
        &siExP,
        &piExP
    );

    // criação do processo de exibição de dados de otimização
	STARTUPINFO siExO;
    PROCESS_INFORMATION piExO;
    ZeroMemory(&siExO, sizeof(siExO));
    siExO.cb = sizeof(siExO);
    ZeroMemory(&piExO, sizeof(piExO));
    status = CreateProcess(
        NULL,
        "ProcessoExOtimizacao.exe",
        NULL,
        NULL,
        TRUE,
        CREATE_NEW_CONSOLE,
        NULL,
        NULL,
        &siExO,
        &piExO
    );

    // criação da thread de leitura de dados
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

    // criação da thread de leitura do teclado
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

    // criação da thread de captura de dados de processo
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

    // criação da thread de captura de dados de otimização
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
    const DWORD numProcess = 2;
    HANDLE hProcess[numProcess];
    hProcess[0] = piExP.hProcess;
    hProcess[1] = piExO.hProcess;

    dwRet = WaitForMultipleObjects(numProcess, hProcess, FALSE, INFINITE);
    SetEvent(hTerminateEvent);
    cc_printf(CCRED, "[S] Processo Exibicao de Dados de Processo encerrado \n");
    cc_printf(CCRED, "[S] Processo Exibicao de Dados de Otimizacao encerrado \n");

    CloseHandle(piExP.hProcess);
    CloseHandle(piExP.hThread);
    CloseHandle(piExO.hProcess);
    CloseHandle(piExO.hThread);

    const DWORD numThreads = 4;
    HANDLE hThreads[numThreads];
    hThreads[0] = hThreadLeituraDados;
    hThreads[1] = hThreadCapturaProcesso;
    hThreads[2] = hThreadCapturaOtimizacao;
    hThreads[3] = hThreadLeituraTeclado;

    dwRet = WaitForMultipleObjects(numThreads, hThreads, TRUE, INFINITE);

    CloseHandle(hThreadLeituraDados);
    CloseHandle(hThreadLeituraTeclado);
    CloseHandle(hThreadCapturaProcesso);
    CloseHandle(hThreadCapturaOtimizacao);

    DeleteCriticalSection(&csConsole);
    DeleteCriticalSection(&csListaCircularIO);

    CloseHandle(hBlockLeituraEvent);
    CloseHandle(hBlockCapProcessoEvent);
    CloseHandle(hBlockCapOtimizacaoEvent);
    CloseHandle(hBlockExProcessoEvent);
    CloseHandle(hBlockExOtimizacaoEvent);
    CloseHandle(hMemoriaCircularCheiaEvent);
    CloseHandle(hNovoEspacoMemoriaCircularEvent);
    CloseHandle(hNovosDadosProcessoEvent);
    CloseHandle(hNovosDadosOtimizacaoEvent);
    CloseHandle(hClearConsoleEvent);
    CloseHandle(hMailSlotProcessoCriadoEvent);
    CloseHandle(hNovaMensagemProcesso);
    CloseHandle(hMutexArquivo);
    CloseHandle(hNovoEspacoArquivoCircularEvent);
    CloseHandle(hNovaMensagemOtimizacao);
    CloseHandle(hArquivoCircular);

    CloseHandle(hTerminateEvent);

    cc_printf(CCRED, "[S] Saindo da thread Principal\n");
    exit(EXIT_SUCCESS);
}

// --------------------------------------------------------------------------------------
// --| Tarefa de Leitura do Teclado |----------------------------------------------------
// --------------------------------------------------------------------------------------

void WINAPI ThreadAuxiliarLeituraTeclado(LPVOID);

void WINAPI ThreadLeituraTeclado(LPVOID tArgs) 
{
	cc_printf(CCWHITE, "[I] Thread Leitura do Teclado inicializada\n");

    // criação da thread auxiliar de leitura do teclado
    HANDLE hThreadAuxiliarLeituraDados = (HANDLE)_beginthreadex(
        NULL,
        0,
        (CAST_FUNCTION) ThreadAuxiliarLeituraTeclado,
        (LPVOID) NULL,
        0,
        (CAST_LPDWORD) NULL
    );
    if (hThreadAuxiliarLeituraDados == INVALID_HANDLE_VALUE) {
		cc_printf(CCRED, "Erro na criação da thread auxiliar de Leitura de Dados!\n");
        exit(0);
    }

    HANDLE hEvents[2] = { hTerminateEvent, hThreadAuxiliarLeituraDados };
    WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);

    cc_printf(CCRED, "[S] Saindo da thread Leitura do Teclado\n");

    _endthreadex(0);
}

void WINAPI ThreadAuxiliarLeituraTeclado(LPVOID tArgs)
{
    char key;
    int ret;

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

    _endthreadex(0);
}

// --------------------------------------------------------------------------------------
// --| Tarefa de Leitura de Dados |------------------------------------------------------
// --------------------------------------------------------------------------------------

int NSEQ_Processo = 1;
int NSEQ_Otimizacao = 1;
int NSEQ_Alarme = 1;

HANDLE hTimerQueue;
HANDLE hTimerProcesso, hTimerOtimizacao, hTimerAlarme;
const int msPeriodProcesso  = 500;
int msPeriodOtimizacao      = 1000;
int msPeriodAlarme          = 1000;

// Impede que Timer sejam apagados novamente ou criados novamente
BOOL ProcessoRodando    = FALSE;
BOOL OtimizacaoRodando  = FALSE;
BOOL AlarmeRodando      = FALSE;

HANDLE hMailSlotProcesso;
HANDLE hMailSlotOtimizacao;

void CALLBACK LerProcesso(PVOID, BOOLEAN);
void CALLBACK LerOtimizacao(PVOID, BOOLEAN);
void CALLBACK LerAlarme(PVOID, BOOLEAN);

void InitializeTimers(BOOL apenasAlarme = FALSE);
void StopTimers(BOOL apenasDados = FALSE);

void WINAPI ThreadLeituraDados(LPVOID tArgs)
{
	cc_printf(CCWHITE, "[I] Thread Leitura de Dados inicializada\n");

    HANDLE hInitProcessoEvents[2]   = { hMailSlotProcessoCriadoEvent, hTerminateEvent };
    HANDLE hEvents[3]               = { hBlockLeituraEvent, hTerminateEvent, hMemoriaCircularCheiaEvent };
	HANDLE hMemoriaEvents[3]        = { hBlockLeituraEvent, hTerminateEvent, hNovoEspacoMemoriaCircularEvent };
    DWORD dwRet;
    DWORD numEvent;

	BOOL status;
    BOOL blockAll = FALSE;
    BOOL blockDados = FALSE;

    dwRet = WaitForMultipleObjects(2, hInitProcessoEvents, FALSE, INFINITE);
    numEvent = dwRet - WAIT_OBJECT_0;

    if (numEvent == 0) // MailSlot processo criada
    {
        ResetEvent(hMailSlotProcessoCriadoEvent);

        hMailSlotProcesso = CreateFile("\\\\.\\mailslot\\processo",
            GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        CheckForError(hMailSlotProcesso);

		hTimerQueue = CreateTimerQueue();
		if (hTimerQueue == NULL) {
			cc_printf(CCRED, "[Leitura] Falha em CreateTimerQueue! Codigo =%d)\n", GetLastError());
			exit(EXIT_FAILURE);
		}
		InitializeTimers();

		do {
			dwRet = WaitForMultipleObjects(3, hEvents, FALSE, INFINITE);
			numEvent = dwRet - WAIT_OBJECT_0;

			if (numEvent == 2) // hMemoriaCircularCheiaEvent - bloqueio por memória cheia
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

					if (numEvent == 2) // hNovoEspacoMemoriaCircularEvent - espaço liberado na memória
					{
						ResetEvent(hMemoriaCircularCheiaEvent);
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
    }

    cc_printf(CCRED, "[S] Saindo da thread Leitura de Dados\n");
    _endthreadex(0);
}

void CALLBACK LerProcesso(PVOID nTimerID, BOOLEAN TimerOrWaitFired)
{
    int memoria_ret;
    char msg[SIZE_MSG];

	genDadosProcesso(msg, NSEQ_Processo);

	EnterCriticalSection(&csListaCircularIO);
	memoria_ret = listaCircular.guardarDadoProcesso(msg);
	LeaveCriticalSection(&csListaCircularIO);

    if (memoria_ret == MEMORY_FULL)
    {
        if (ProcessoRodando == FALSE) return; // já foi avisado que a memória está cheia
        SetEvent(hMemoriaCircularCheiaEvent);
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
    BOOL status;
    int memoria_ret;
    char msg[SIZE_MSG];

	genDadosOtimizacao(msg, NSEQ_Otimizacao);

	EnterCriticalSection(&csListaCircularIO);
	memoria_ret = listaCircular.guardarDadoOtimizacao(msg);
	LeaveCriticalSection(&csListaCircularIO);

    if (memoria_ret == MEMORY_FULL)
    {
        if (OtimizacaoRodando == FALSE) return; // já foi avisado que a memória está cheia
        SetEvent(hMemoriaCircularCheiaEvent);
    }

    if (memoria_ret != MEMORY_FULL) 
    {
		msPeriodOtimizacao = int(RandReal(1000, 5000));
		status = ChangeTimerQueueTimer(hTimerQueue, hTimerOtimizacao, msPeriodOtimizacao, msPeriodOtimizacao);
		if (!status)
		{
			cc_printf(CCRED, "[Leitura] Erro ao alterar período de leitura de Otimizacoes");
			exit(EXIT_FAILURE);
		}

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
    BOOL status;

    char msg[SIZE_MSG];

	genAlarme(msg, NSEQ_Alarme);

	//cc_printf(CCREDI, "[Leitura] %s\n", msg);
    WriteMailSlot(hMailSlotProcesso, msg);
    SetEvent(hNovaMensagemProcesso);

    msPeriodAlarme = int(RandReal(1000, 5000));
    status = ChangeTimerQueueTimer(hTimerQueue, hTimerAlarme, msPeriodAlarme, msPeriodAlarme);
    if (!status)
    {
        cc_printf(CCRED, "[Leitura] Erro ao alterar período de leitura de Alarmes");
        exit(EXIT_FAILURE);
    }

	NSEQ_Alarme = 1 + (NSEQ_Alarme % 9999);
}

void InitializeTimers(BOOL apenasAlarme)
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
void StopTimers(BOOL apenasDados)
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

// --------------------------------------------------------------------------------------
// --| Tarefa de Captura de dados de Otimizacao |----------------------------------------
// --------------------------------------------------------------------------------------

void WINAPI ThreadCapturaProcesso(LPVOID)
{
	cc_printf(CCWHITE, "[I] Thread Captura de Dados de Processo inicializada\n");

    HANDLE hInitEvents[2] = { hMailSlotProcessoCriadoEvent, hTerminateEvent };
    HANDLE hEvents[3] = { hBlockCapProcessoEvent, hTerminateEvent, hNovosDadosProcessoEvent };
    DWORD dwRet;
    DWORD numEvent;

    int ret;
    char msg[SIZE_MSG];

    dwRet = WaitForMultipleObjects(2, hInitEvents, FALSE, INFINITE);
    numEvent = dwRet - WAIT_OBJECT_0;

    if (numEvent == 0) // MailSlot criada
    {
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

                    //cc_printf(CCGREEN, "[Processo] %s\n", msg);
					WriteMailSlot(hMailSlotProcesso, msg);
					SetEvent(hNovaMensagemProcesso);
                    PulseEvent(hNovoEspacoMemoriaCircularEvent);
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
    }

    cc_printf(CCRED, "[S] Saindo da thread Captura de Dados de Processo\n");
    _endthreadex(0);
}

// --------------------------------------------------------------------------------------
// --| Tarefa de Captura de dados do Processo |------------------------------------------
// --------------------------------------------------------------------------------------

BOOL temEspacoArquivoCircular();
int EscreverArquivoCircular(char*);

void WINAPI ThreadCapturaOtimizacao(LPVOID)
{
	cc_printf(CCWHITE, "[I] Thread Captura de Dados de Otimizacao inicializada\n");

    HANDLE hEvents[3] = { hBlockCapOtimizacaoEvent, hTerminateEvent, hNovosDadosOtimizacaoEvent };
    HANDLE hEventsMemoryFull[3] = { hBlockCapOtimizacaoEvent, hTerminateEvent, hNovoEspacoArquivoCircularEvent };
    DWORD dwRet;
    DWORD numEvent;

    int ret;
    char msg[SIZE_MSG];

	do {
		dwRet = WaitForMultipleObjects(3, hEvents, FALSE, INFINITE);
		numEvent = dwRet - WAIT_OBJECT_0;

        if (numEvent == 1) break;

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

		else if (numEvent == 2)
		{
			ret = MEMORY_EMPTY;
			do {
                if (temEspacoArquivoCircular())
				{
					EnterCriticalSection(&csListaCircularIO);
					ret = listaCircular.lerDadoOtimizacao(msg);
					LeaveCriticalSection(&csListaCircularIO);
					if (ret == MEMORY_EMPTY) break;

					//cc_printf(CCBLUE, "[Otimizacao] %s\n", msg);
                    WaitForSingleObject(hMutexArquivo, INFINITE);
					EscreverArquivoCircular(msg);
                    ReleaseMutex(hMutexArquivo);

					ResetEvent(hNovosDadosOtimizacaoEvent);
				}
				else
				{
					cc_printf(CCPURPLE, "[Otimizacao] Arquivo de memoria circular cheio\n");

					dwRet = WaitForMultipleObjects(3, hEventsMemoryFull, FALSE, INFINITE);
					numEvent = dwRet - WAIT_OBJECT_0;

                    break;
				}

				SetEvent(hNovaMensagemOtimizacao);
				PulseEvent(hNovoEspacoMemoriaCircularEvent);
			} while (ret != MEMORY_EMPTY);
		}
	} while (numEvent != 1);

    cc_printf(CCRED, "[S] Saindo da thread Captura de Dados de Otimizacao\n");
    _endthreadex(0);
}

BOOL temEspacoArquivoCircular()
{
    char buffer[SIZE_MSG];

    SetFilePointer(hArquivoCircular, 0, 0, FILE_BEGIN);
    if (FALSE == ReadFile(hArquivoCircular, buffer, HeaderSize, 0, NULL))
    {
		cc_printf(CCRED, "Erro ao ler cabecalho do arquivo\n");
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

    if (numMsg >= MAX_MSG_FILE)
    {
        return FALSE;
    }
    return TRUE;
}
int EscreverArquivoCircular(char *msg)
{
    char buffer[SIZE_MSG];

    SetFilePointer(hArquivoCircular, 0, 0, FILE_BEGIN);
    if (FALSE == ReadFile(hArquivoCircular, buffer, HeaderSize, 0, NULL))
    {
		cc_printf(CCRED, "Erro ao ler cabecalho do arquivo\n");
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

    if (numMsg >= MAX_MSG_FILE)
    {
        return FILE_FULL;
    }

	std::string LineMsg = std::string(msg) + '\n';
	SetFilePointer(hArquivoCircular, HeaderSize + lastLine * LineMsg.length(), 0, FILE_BEGIN);
	if (FALSE == WriteFile(hArquivoCircular, LineMsg.c_str(), LineMsg.length(), 0, NULL))
	{
		cc_printf(CCRED, "Erro ao escrever otimizacao no arquivo\n");
	}

    lastLine = (lastLine + 1) % MAX_MSG_FILE; 
    numMsg++;
    sprintf_s(buffer, HeaderSize+1, "%02d %02d %02d\n", initLine, lastLine, numMsg);

    SetFilePointer(hArquivoCircular, 0, 0, FILE_BEGIN);
    if (FALSE == WriteFile(hArquivoCircular, buffer, HeaderSize, 0, NULL))
    {
		cc_printf(CCRED, "Erro ao escrever no cabecalho do arquivo\n");
    }

    return 0;
}

