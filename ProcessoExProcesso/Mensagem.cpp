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

	if (TIPO == 99) // Alarme
	{
		int CODIGO = stoi(dados.at(2));
		mensagem << dados.at(3) << " ";
		mensagem << "NSEQ: " << std::setfill('0') << std::setw(4) << NSEQ << " ";
		mensagem << "CODIGO: " << std::setfill('0') << std::setw(2) << CODIGO << " ";

		//  mensagem << "12345678901234567890123456789" << CODIGO;
		if (CODIGO < 10)
			mensagem << "Temperatura alta na zona " << CODIGO;
		else if (CODIGO < 20)
			mensagem << "Temperatura baixa na zona " << CODIGO - 10;
		else if (CODIGO < 30)
			mensagem << "Velocidade excessiva na zona " << CODIGO - 20;
		else if (CODIGO < 40)
			mensagem << "Velocidade baixa na zona " << CODIGO - 30;
		else if (CODIGO < 50)
			mensagem << "Alta vibracao na zona " << CODIGO - 40;
		else if (CODIGO < 60)
			mensagem << "Falha na ignicao do forno " << CODIGO - 50;
		else if (CODIGO < 70)
			mensagem << "Pressao muito alta na zona " << CODIGO - 60;
		else if (CODIGO <80)
			mensagem << "Pressao muito baixa na zona " << CODIGO - 70;
		else if (CODIGO < 90)
			mensagem << "Presenca de fumaca na zona " << CODIGO - 80;
		else if (CODIGO < 100)
			mensagem << "Vazamento de gas na zona " << CODIGO - 90;
		else
			mensagem << "Problema desconhecido n" << CODIGO;
	}
	else if (TIPO == 55)
	{
		mensagem << "NSEQ: " << std::setfill('0') << std::setw(4) << NSEQ << " ";
		mensagem << "T_ZP: " << dados.at(2) << "C ";
		mensagem << "T_ZA: " << dados.at(3) << "C ";
		mensagem << "T_ZE: " << dados.at(4) << "C ";
		mensagem << "P: "	 << dados.at(5) << "psi ";
		mensagem << dados.at(6);
	}
	else mensagem << msg;

	return mensagem.str();
}
