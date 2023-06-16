#pragma once

#include <Windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#ifndef MAX_MSG
#define MAX_MSG 50
#endif // !MAX_MSG

double RandReal(double min, double max);

void genDadosProcesso(char* msg, int NSEQ_Processo);
void genDadosOtimizacao(char* msg, int NSEQ_Otimizacao);
void genAlarme(char* msg, int NSEQ_Alarme);

BOOL WriteMailSlot(HANDLE hSlot, char* msg);
