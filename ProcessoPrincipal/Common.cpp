#include "Common.h"

double RandReal(double min, double max) 
{
    return min + double(rand()) / RAND_MAX * (max - min);
}

void genDadosProcesso(char* msg, int NSEQ_Processo)
{
    int TIPO = 55;
    double T_ZONA_P, T_ZONA_A, T_ZONA_E, PRESSAO;
    SYSTEMTIME TIMESTAMP;

	T_ZONA_P = RandReal(700, 900);
	T_ZONA_A = RandReal(901, 1200);
	T_ZONA_E = RandReal(1201, 1400);
	PRESSAO  = RandReal(10, 12);
	GetLocalTime(&TIMESTAMP);

	sprintf_s(msg, SIZE_MSG, "%04d$%02d$%06.1f$%06.1f$%06.1f$%04.1f$%02d:%02d:%02d", 
		NSEQ_Processo, TIPO, T_ZONA_P, T_ZONA_A, T_ZONA_E, PRESSAO,
		TIMESTAMP.wHour, TIMESTAMP.wMinute, TIMESTAMP.wSecond);
}

void genDadosOtimizacao(char* msg, int NSEQ_Otimizacao) 
{
    int TIPO = 01;
    double T_ZONA_P, T_ZONA_A, T_ZONA_E;
    SYSTEMTIME TIMESTAMP;

	T_ZONA_P = RandReal(700, 900);
	T_ZONA_A = RandReal(901, 1200);
	T_ZONA_E = RandReal(1201, 1400);
	GetLocalTime(&TIMESTAMP);

	sprintf_s(msg, SIZE_MSG, "%04d$%02d$%06.1f$%06.1f$%06.1f$%02d:%02d:%02d", 
		NSEQ_Otimizacao, TIPO, T_ZONA_P, T_ZONA_A, T_ZONA_E,
		TIMESTAMP.wHour, TIMESTAMP.wMinute, TIMESTAMP.wSecond);
}

void genAlarme(char* msg, int NSEQ_Alarme)
{
    int TIPO = 99;
    int CODIGO;
    SYSTEMTIME TIMESTAMP;

    CODIGO = round(RandReal(0, 99));
	GetLocalTime(&TIMESTAMP);

	sprintf_s(msg, SIZE_MSG, "%04d$%02d$%02d$%02d:%02d:%02d", 
		NSEQ_Alarme, TIPO, CODIGO,
		TIMESTAMP.wHour, TIMESTAMP.wMinute, TIMESTAMP.wSecond);
}

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_RESET   "\x1b[0m"
BOOL WriteMailSlot(HANDLE hSlot, char* msg)
{
    BOOL status;

	status = WriteFile(hSlot, 
        msg, (DWORD) lstrlen(msg)+1,  NULL, NULL); 

	if (!status) 
	{ 
	  printf(ANSI_COLOR_RED "Erro ao escrever no mailslot de processos, codigo %d" ANSI_COLOR_RESET "\n", 
		  GetLastError()); 
	  return FALSE; 
	} 

	return TRUE;
}
