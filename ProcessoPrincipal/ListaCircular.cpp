#include "ListaCircular.h"


ListaCircular::ListaCircular()
{
	memoriaProcesso = new std::string[MAX_DADOS];
	memoriaOtimizacao = new std::string[MAX_DADOS];
}

ListaCircular::~ListaCircular()
{
	delete[] memoriaProcesso;
	delete[] memoriaOtimizacao;
}

int ListaCircular::guardarDadoProcesso(char* msg)
{
	if (numDadosProcesso + numDadosOtimizacao == MAX_DADOS)
		return MEMORY_FULL;

	memoriaProcesso[ultimoProcesso] = std::string(msg);
	ultimoProcesso = (ultimoProcesso + 1) % MAX_DADOS;

	numDadosProcesso++;
	return 0;
}
int ListaCircular::lerDadoProcesso(char* msg)
{
	if (numDadosProcesso == 0)
		return MEMORY_EMPTY;

	strcpy_s(msg, SIZE_MSG, memoriaProcesso[primeiroProcesso].c_str());
	primeiroProcesso = (primeiroProcesso + 1) % MAX_DADOS;

	numDadosProcesso--;
	return 0;
}

int ListaCircular::guardarDadoOtimizacao(char* msg)
{
	if (numDadosOtimizacao + numDadosProcesso == MAX_DADOS)
		return MEMORY_FULL;

	memoriaOtimizacao[ultimoOtimizacao] = std::string(msg);
	ultimoOtimizacao = (ultimoOtimizacao + 1) % MAX_DADOS;

	numDadosOtimizacao++;
	return 0;
}
int ListaCircular::lerDadoOtimizacao(char* msg)
{
	if (numDadosOtimizacao == 0)
		return MEMORY_EMPTY;

	strcpy_s(msg, SIZE_MSG, memoriaOtimizacao[primeiroOtimizacao].c_str());
	primeiroOtimizacao = (primeiroOtimizacao + 1) % MAX_DADOS;

	numDadosOtimizacao--;
	return 0;
}
