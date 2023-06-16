#include "ListaCircular.h"


ListaCircular::ListaCircular()
{
	memoria_processo = new std::string[MAX_DADOS];
	memoria_otimizacao = new std::string[MAX_DADOS];
}

ListaCircular::~ListaCircular()
{
	delete[] memoria_processo;
	delete[] memoria_otimizacao;
}

int ListaCircular::guardarDadoProcesso(char* msg)
{
	if (numDadosProcesso + numDadosOtimizacao == MAX_DADOS)
		return MEMORY_FULL;

	memoria_processo[ultimo_processo] = std::string(msg);
	ultimo_processo = (ultimo_processo + 1) % MAX_DADOS;

	numDadosProcesso++;
	return 0;
}
int ListaCircular::lerDadoProcesso(char* msg)
{
	if (numDadosProcesso == 0)
		return MEMORY_EMPTY;

	strcpy_s(msg, SIZE_MSG, memoria_processo[primeiro_processo].c_str());
	primeiro_processo = (primeiro_processo + 1) % MAX_DADOS;

	numDadosProcesso--;
	return 0;
}

int ListaCircular::guardarDadoOtimizacao(char* msg)
{
	if (numDadosOtimizacao + numDadosProcesso == MAX_DADOS)
		return MEMORY_FULL;

	memoria_otimizacao[ultimo_otimizacao] = std::string(msg);
	ultimo_otimizacao = (ultimo_otimizacao + 1) % MAX_DADOS;

	numDadosOtimizacao++;
	return 0;
}
int ListaCircular::lerDadoOtimizacao(char* msg)
{
	if (numDadosOtimizacao == 0)
		return MEMORY_EMPTY;

	strcpy_s(msg, SIZE_MSG, memoria_otimizacao[primeiro_otimizacao].c_str());
	primeiro_otimizacao = (primeiro_otimizacao + 1) % MAX_DADOS;

	numDadosOtimizacao--;
	return 0;
}
