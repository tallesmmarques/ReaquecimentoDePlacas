#pragma once

#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <vector>
#include <Windows.h>

#ifndef MAX_MSG
#define MAX_MSG 50
#endif

class Mensagem
{
private:
	std::string msg;
	std::vector<std::string> dados;
public:
	int TIPO;
	int NSEQ;

	Mensagem(char* initMsg);
	std::string getMensagemFormatada();
};

