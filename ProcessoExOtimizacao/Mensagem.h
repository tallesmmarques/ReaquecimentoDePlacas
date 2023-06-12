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

public:
	Mensagem(char* initMsg)
	{
		msg = std::string(initMsg);
		dados.resize(7);
		int i = 0;

		std::istringstream iss(msg);
		std::string token;
		while (std::getline(iss, token, '$') && i < 7)
		{
			dados.at(i) = token;
			i++;
		}

		NSEQ = stoi(dados.at(0));
		TIPO = stoi(dados.at(1));
	}

	std::string getMensagemFormatada()
	{
		std::ostringstream mensagem;

		if (TIPO == 99) // Alarme
		{
			int CODIGO = stoi(dados.at(2));
			mensagem << dados.at(3) << " ";
			mensagem << "NSEQ: " << std::setfill('0') << std::setw(4) << NSEQ << " ";
			mensagem << "CODIGO: " << std::setfill('0') << std::setw(2) << CODIGO << " ";

			switch (CODIGO)
			{
			case 5:
				mensagem << "Mensagem 1";
				break;
			case 10:
				mensagem << "Mensagem 2";
				break;
			case 20:
				mensagem << "Mensagem 3";
				break;
			case 30:
				mensagem << "Mensagem 4";
				break;
			case 40:
				mensagem << "Mensagem 5";
				break;
			case 50:
				mensagem << "Mensagem 6";
				break;
			case 60:
				mensagem << "Mensagem 7";
				break;
			case 70:
				mensagem << "Mensagem 8";
				break;
			case 80:
				mensagem << "Mensagem 9";
				break;
			case 90:
				mensagem << "Mensagem 10";
				break;
			default:
				mensagem << "O Processo gerou o alarme de código " << CODIGO << "!";
				break;
			}
		}
		else if (TIPO == 55)
		{
			mensagem << msg;
		}
		else
		{
			mensagem << msg;
		}

		return mensagem.str();
	}

	void print()
	{
		for (std::string m : dados)
		{
			printf("%s ", m.c_str());
		}
		printf("\n");
	}
};

