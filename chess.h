#ifndef CHESS_H
#define CHESS_H

#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <cctype>

enum PType
{
	PT_EMPTY = 0,
	PT_PAWN,
	PT_KNIGHT,
	PT_BISHOP,
	PT_ROOK,
	PT_QUEEN,
	PT_KING
};

enum PColor
{
	PC_NONE = 0,
	PC_WHITE,
	PC_BLACK
};

struct Move
{
	int ff = 0, fr = 0;	  // pole zrodlowe (from)
	int tf = 0, tr = 0;	  // pole docelowe (to)
	bool castleK = false; // roszada krotka  (O-O)
	bool castleQ = false; // roszada dluga   (O-O-O)
	int promo = PT_EMPTY; // figura promocji (lub PT_EMPTY)
};

// Zamienia literke figury na typ (do promocji)
inline int charToType(char c)
{
	switch (std::toupper((unsigned char)c))
	{
	case 'Q':
		return PT_QUEEN;
	case 'R':
		return PT_ROOK;
	case 'B':
		return PT_BISHOP;
	case 'N':
		return PT_KNIGHT;
	default:
		return PT_EMPTY;
	}
}

// Parsuje pole typu "e2" -> (file, rank). Zwraca false gdy niepoprawne.
inline bool parseSquare(const std::string &s, int &f, int &r)
{
	if (s.size() < 2)
		return false;
	int file = std::tolower((unsigned char)s[0]) - 'a';
	int rank = s[1] - '1';
	if (file < 0 || file > 7 || rank < 0 || rank > 7)
		return false;
	f = file;
	r = rank;
	return true;
}

// Wczytuje partie z pliku. Format (jeden ruch na linie):
//   e2 e4            - ruch/bicie (bicie wykrywane automatycznie)
//   e7 e8 Q          - ruch z promocja na hetmana
//   e2e4             - dozwolony zapis bez spacji
//   O-O / O-O-O      - roszada krotka / dluga
//   # ...            - komentarz (do konca linii), puste linie pomijane
inline bool parseGame(const std::string &path, std::vector<Move> &moves, std::string &err)
{
	std::ifstream in(path.c_str());
	if (!in)
	{
		err = "Nie mozna otworzyc pliku: " + path;
		return false;
	}

	std::string line;
	int lineNo = 0;
	while (std::getline(in, line))
	{
		lineNo++;
		size_t h = line.find('#');
		if (h != std::string::npos)
			line = line.substr(0, h);

		std::vector<std::string> tok;
		std::istringstream iss(line);
		std::string w;
		while (iss >> w)
			tok.push_back(w);
		if (tok.empty())
			continue;

		Move mv;
		std::string a = tok[0];

		if (a == "O-O" || a == "0-0")
		{
			mv.castleK = true;
			moves.push_back(mv);
			continue;
		}
		if (a == "O-O-O" || a == "0-0-0")
		{
			mv.castleQ = true;
			moves.push_back(mv);
			continue;
		}

		std::string from, to, promo;
		bool packed = a.size() >= 4 &&
					  std::isalpha((unsigned char)a[0]) && std::isdigit((unsigned char)a[1]) &&
					  std::isalpha((unsigned char)a[2]) && std::isdigit((unsigned char)a[3]);
		if (packed)
		{
			from = a.substr(0, 2);
			to = a.substr(2, 2);
			if (a.size() >= 5)
				promo = a.substr(4, 1);
			else if (tok.size() >= 2)
				promo = tok[1];
		}
		else
		{
			if (tok.size() < 2)
			{
				err = "Niepelny ruch w linii " + std::to_string(lineNo);
				return false;
			}
			from = tok[0];
			to = tok[1];
			if (tok.size() >= 3)
				promo = tok[2];
		}

		if (!parseSquare(from, mv.ff, mv.fr) || !parseSquare(to, mv.tf, mv.tr))
		{
			err = "Zla notacja pola w linii " + std::to_string(lineNo);
			return false;
		}
		if (!promo.empty())
			mv.promo = charToType(promo[0]);
		moves.push_back(mv);
	}
	return true;
}

// Wypelnia ustawienie poczatkowe: type[file][rank], color[file][rank].
inline void startingPosition(PType type[8][8], PColor color[8][8])
{
	for (int f = 0; f < 8; f++)
		for (int r = 0; r < 8; r++)
		{
			type[f][r] = PT_EMPTY;
			color[f][r] = PC_NONE;
		}

	PType back[8] = {PT_ROOK, PT_KNIGHT, PT_BISHOP, PT_QUEEN, PT_KING, PT_BISHOP, PT_KNIGHT, PT_ROOK};
	for (int f = 0; f < 8; f++)
	{
		type[f][0] = back[f];
		color[f][0] = PC_WHITE; // 1. rzad - biale figury
		type[f][1] = PT_PAWN;
		color[f][1] = PC_WHITE; // 2. rzad - biale pionki
		type[f][6] = PT_PAWN;
		color[f][6] = PC_BLACK; // 7. rzad - czarne pionki
		type[f][7] = back[f];
		color[f][7] = PC_BLACK; // 8. rzad - czarne figury
	}
}

#endif
