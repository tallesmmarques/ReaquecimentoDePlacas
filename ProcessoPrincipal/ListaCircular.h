#pragma once

#include <string>

#define NUM_DADOS 5

typedef struct Node
{
	std::string msg;
	int next;
}Node_t;

class ListaCircular
{
public:
	Node_t* memoria;

	int primeiro_geral = 0;
	int ultimo_geral = 0;

	int primeiro_processo = 0;
	int ultimo_processo = 0;

	int primeiro_otimizacao = 0;
	int ultimo_otimizacao = 0;

public:
	ListaCircular()
	{
		memoria = new Node[NUM_DADOS];
	}
	~ListaCircular()
	{
		delete[] memoria;
	}

	void guardarDado(char* msg)
	{
		memoria[ultimo_geral].msg = std::string(msg);
		ultimo_geral = (ultimo_geral + 1) % NUM_DADOS;
	}

	void guardarDadoProcesso(char* msg)
	{
		guardarDado(msg);
	}
	std::string lerDadoProcesso()
	{
		std::string ret = memoria[primeiro_geral].msg;
		primeiro_geral = (primeiro_geral + 1) % NUM_DADOS;
		return ret;
	}

	void guardarDadoOtimizacao(char* msg)
	{
		guardarDado(msg);
	}
	std::string lerDadoOtimizacao()
	{
		std::string ret = memoria[primeiro_geral].msg;
		primeiro_geral = (primeiro_geral + 1) % NUM_DADOS;
		return ret;
	}
};

