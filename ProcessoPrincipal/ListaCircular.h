#pragma once

#include <string>
#include <Windows.h>

#define MAX_DADOS 200
#define MEMORY_FULL 1
#define MEMORY_EMPTY 2

#ifndef MAX_MSG
#define MAX_MSG 50
#endif

class ListaCircular
{
private:
	std::string* memoria_processo;
	std::string* memoria_otimizacao;

	int numDadosProcesso = 0;
	int primeiro_processo = 0;
	int ultimo_processo = 0;

	int numDadosOtimizacao = 0;
	int primeiro_otimizacao = 0;
	int ultimo_otimizacao = 0;

public:
	ListaCircular();
	~ListaCircular();

	int guardarDadoProcesso(char* msg);
	int lerDadoProcesso(char* msg);
	int guardarDadoOtimizacao(char* msg);
	int lerDadoOtimizacao(char* msg);
};

