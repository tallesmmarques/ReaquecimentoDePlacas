#pragma once

#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <vector>
#include <Windows.h>

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

