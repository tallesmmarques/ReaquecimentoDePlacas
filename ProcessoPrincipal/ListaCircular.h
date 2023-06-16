#pragma once

#include <string>
#include <Windows.h>

#define MAX_DADOS 200
#define MEMORY_FULL 1
#define MEMORY_EMPTY 2

#ifndef SIZE_MSG
#define SIZE_MSG 50
#endif

class ListaCircular
{
private:
	std::string* memoriaProcesso;
	std::string* memoriaOtimizacao;

	int numDadosProcesso = 0;
	int primeiroProcesso = 0;
	int ultimoProcesso = 0;

	int numDadosOtimizacao = 0;
	int primeiroOtimizacao = 0;
	int ultimoOtimizacao = 0;

public:
	ListaCircular();
	~ListaCircular();

	int guardarDadoProcesso(char* msg);
	int lerDadoProcesso(char* msg);
	int guardarDadoOtimizacao(char* msg);
	int lerDadoOtimizacao(char* msg);
};

