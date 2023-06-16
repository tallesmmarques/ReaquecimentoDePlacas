#include "Mensagem.h"

Mensagem::Mensagem(char* initMsg)
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

std::string Mensagem::getMensagemFormatada()
{
	std::ostringstream mensagem;

	if (TIPO == 01)
	{
		mensagem << dados.at(5) << " ";
		mensagem << "NSEQ: " << std::setfill('0') << std::setw(4) << NSEQ << " ";
		mensagem << "SP_ZP: " << dados.at(2) << "C ";
		mensagem << "SP_ZA: " << dados.at(3) << "C ";
		mensagem << "SP_ZE: " << dados.at(4) << "C ";
	}
	else mensagem << msg;

	return mensagem.str();
}
