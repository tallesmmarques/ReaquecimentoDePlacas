#pragma once

#include <string>
#include <Windows.h>

#define MAX_DADOS 10
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
	Node_t* memoria_processo;
	Node_t* memoria_otimizacao;

	int numDados = 0;

	int primeiro_processo = 0;
	int ultimo_processo = 0;

	int primeiro_otimizacao = 0;
	int ultimo_otimizacao = 0;

	BOOL primeiro_uso;

public:
	ListaCircular()
	{
		memoria_processo = new Node[MAX_DADOS];
		memoria_otimizacao = new Node[MAX_DADOS];

		primeiro_uso = TRUE;
	}
	~ListaCircular()
	{
		delete[] memoria_processo;
		delete[] memoria_otimizacao;
	}

	int guardarDadoProcesso(char* msg)
	{
		if ((ultimo_processo + 1) % MAX_DADOS == primeiro_processo)
			return MEMORY_FULL;

		if (primeiro_uso == FALSE)
		{
			ultimo_processo = (ultimo_processo + 1) % MAX_DADOS;
		}
		else primeiro_uso = FALSE;

		memoria_processo[ultimo_processo].msg = std::string(msg);

		numDados++;
		return 0;
	}
	int lerDadoProcesso(char* msg)
	{
		if (primeiro_processo == ultimo_processo || primeiro_uso)
			return MEMORY_EMPTY;
		primeiro_processo = (primeiro_processo + 1) % MAX_DADOS;

		std::string ret = memoria_processo[primeiro_processo].msg;
		strcpy_s(msg, MAX_MSG, ret.c_str());

		numDados--;
		return 0;
	}

	//int guardarDadoOtimizacao(char* msg)
	//{
	//	if ((ultimo_otimizacao + 1) % MAX_DADOS == primeiro_otimizacao)
	//		return MEMORY_FULL;

	//	if (primeiro_uso == FALSE)
	//	{
	//		ultimo_otimizacao = (ultimo_otimizacao + 1) % MAX_DADOS;
	//	}
	//	else primeiro_uso = FALSE;

	//	memoria_otimizacao[ultimo_otimizacao].msg = std::string(msg);

	//	numDados++;
	//	return 0;
	//}
	//int lerDadoOtimizacao(char *msg)
	//{
	//	if (primeiro_otimizacao == ultimo_otimizacao || primeiro_uso)
	//		return MEMORY_EMPTY;

	//	std::string ret = memoria_otimizacao[primeiro_otimizacao].msg;
	//	strcpy_s(msg, MAX_MSG, ret.c_str());

	//	primeiro_otimizacao = (primeiro_otimizacao + 1) % MAX_DADOS;
	//	numDados--;
	//	return 0;
	//}
};

