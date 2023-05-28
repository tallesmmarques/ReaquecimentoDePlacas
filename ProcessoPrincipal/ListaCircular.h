#pragma once

#include <string>
#include <Windows.h>

#define MAX_DADOS 5
#define MEMORY_FULL 1
#define MEMORY_EMPTY 2

#ifndef MAX_MSG
#define MAX_MSG 50
#endif

typedef struct Node
{
	std::string msg;
	int next;
}Node_t;

class ListaCircular
{
public:
	std::string* memoria_processo;
	std::string* memoria_otimizacao;

	int numDadosProcesso = 0;
	int primeiro_processo = 0;
	int ultimo_processo = 0;

	int numDadosOtimizacao = 0;
	int primeiro_otimizacao = 0;
	int ultimo_otimizacao = 0;

public:
	ListaCircular()
	{
		memoria_processo = new std::string[MAX_DADOS];
		memoria_otimizacao = new std::string[MAX_DADOS];
	}
	~ListaCircular()
	{
		delete[] memoria_processo;
		delete[] memoria_otimizacao;
	}

	int guardarDadoProcesso(char* msg)
	{
		if (numDadosProcesso + numDadosOtimizacao == MAX_DADOS)
			return MEMORY_FULL;

		memoria_processo[ultimo_processo] = std::string(msg);
		ultimo_processo = (ultimo_processo + 1) % MAX_DADOS;

		numDadosProcesso++;
		return 0;
	}
	int lerDadoProcesso(char* msg)
	{
		if (numDadosProcesso == 0)
			return MEMORY_EMPTY;

		strcpy_s(msg, MAX_MSG, memoria_processo[primeiro_processo].c_str());
		primeiro_processo = (primeiro_processo + 1) % MAX_DADOS;

		numDadosProcesso--;
		return 0;
	}

	int guardarDadoOtimizacao(char* msg)
	{
		if (numDadosOtimizacao + numDadosProcesso == MAX_DADOS)
			return MEMORY_FULL;

		memoria_otimizacao[ultimo_otimizacao] = std::string(msg);
		ultimo_otimizacao = (ultimo_otimizacao + 1) % MAX_DADOS;

		numDadosOtimizacao++;
		return 0;
	}
	int lerDadoOtimizacao(char* msg)
	{
		if (numDadosOtimizacao == 0)
			return MEMORY_EMPTY;

		strcpy_s(msg, MAX_MSG, memoria_otimizacao[primeiro_otimizacao].c_str());
		primeiro_otimizacao = (primeiro_otimizacao + 1) % MAX_DADOS;

		numDadosOtimizacao--;
		return 0;
	}
};

