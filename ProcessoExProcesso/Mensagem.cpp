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
		mensagem << "NSEQ: " << std::setfill('0') << std::setw(4) << NSEQ << " ";
		mensagem << "T_ZP: " << dados.at(2) << "C ";
		mensagem << "T_ZA: " << dados.at(3) << "C ";
		mensagem << "T_ZE: " << dados.at(4) << "C ";
		mensagem << "P: "	 << dados.at(5) << "psi ";
		mensagem << dados.at(6);
	}
	else if (TIPO == 01)
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
