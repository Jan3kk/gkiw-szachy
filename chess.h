#ifndef CHESS_H
#define CHESS_H

#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <cctype>

enum PieceType
{
	PIECE_NONE = 0,
	PIECE_PAWN,
	PIECE_KNIGHT,
	PIECE_BISHOP,
	PIECE_ROOK,
	PIECE_QUEEN,
	PIECE_KING
};

enum PieceColor
{
	COLOR_NONE = 0,
	COLOR_WHITE,
	COLOR_BLACK
};

struct Move
{
	int fromFile = 0, fromRank = 0;
	int toFile = 0, toRank = 0;
	bool castleKingside = false;
	bool castleQueenside = false;
	int promotion = PIECE_NONE;
};

inline int letterToPieceType(char c)
{
	switch (std::toupper((unsigned char)c))
	{
	case 'Q': return PIECE_QUEEN;
	case 'R': return PIECE_ROOK;
	case 'B': return PIECE_BISHOP;
	case 'N': return PIECE_KNIGHT;
	default:  return PIECE_NONE;
	}
}

inline bool parseSquare(const std::string &text, int &file, int &rank)
{
	if (text.size() < 2)
		return false;
	int f = std::tolower((unsigned char)text[0]) - 'a';
	int r = text[1] - '1';
	if (f < 0 || f > 7 || r < 0 || r > 7)
		return false;
	file = f;
	rank = r;
	return true;
}

inline bool parseGame(const std::string &path, std::vector<Move> &moves, std::string &err)
{
	std::ifstream in(path.c_str());
	if (!in)
	{
		err = "Nie mozna otworzyc pliku: " + path;
		return false;
	}

	std::string line;
	int lineNumber = 0;
	while (std::getline(in, line))
	{
		lineNumber++;
		size_t commentPos = line.find('#');
		if (commentPos != std::string::npos)
			line = line.substr(0, commentPos);

		std::vector<std::string> tokens;
		std::istringstream stream(line);
		std::string word;
		while (stream >> word)
			tokens.push_back(word);
		if (tokens.empty())
			continue;

		Move move;
		std::string first = tokens[0];

		if (first == "O-O" || first == "0-0")
		{
			move.castleKingside = true;
			moves.push_back(move);
			continue;
		}
		if (first == "O-O-O" || first == "0-0-0")
		{
			move.castleQueenside = true;
			moves.push_back(move);
			continue;
		}

		std::string from, to, promotion;
		bool packed = first.size() >= 4 &&
					  std::isalpha((unsigned char)first[0]) && std::isdigit((unsigned char)first[1]) &&
					  std::isalpha((unsigned char)first[2]) && std::isdigit((unsigned char)first[3]);
		if (packed)
		{
			from = first.substr(0, 2);
			to = first.substr(2, 2);
			if (first.size() >= 5)
				promotion = first.substr(4, 1);
			else if (tokens.size() >= 2)
				promotion = tokens[1];
		}
		else
		{
			if (tokens.size() < 2)
			{
				err = "Niepelny ruch w linii " + std::to_string(lineNumber);
				return false;
			}
			from = tokens[0];
			to = tokens[1];
			if (tokens.size() >= 3)
				promotion = tokens[2];
		}

		if (!parseSquare(from, move.fromFile, move.fromRank) || !parseSquare(to, move.toFile, move.toRank))
		{
			err = "Zla notacja pola w linii " + std::to_string(lineNumber);
			return false;
		}
		if (!promotion.empty())
			move.promotion = letterToPieceType(promotion[0]);
		moves.push_back(move);
	}
	return true;
}

inline void startingPosition(PieceType pieceType[8][8], PieceColor pieceColor[8][8])
{
	for (int file = 0; file < 8; file++)
		for (int rank = 0; rank < 8; rank++)
		{
			pieceType[file][rank] = PIECE_NONE;
			pieceColor[file][rank] = COLOR_NONE;
		}

	PieceType backRank[8] = {PIECE_ROOK, PIECE_KNIGHT, PIECE_BISHOP, PIECE_QUEEN, PIECE_KING, PIECE_BISHOP, PIECE_KNIGHT, PIECE_ROOK};
	for (int file = 0; file < 8; file++)
	{
		pieceType[file][0] = backRank[file]; pieceColor[file][0] = COLOR_WHITE;
		pieceType[file][1] = PIECE_PAWN;     pieceColor[file][1] = COLOR_WHITE;
		pieceType[file][6] = PIECE_PAWN;     pieceColor[file][6] = COLOR_BLACK;
		pieceType[file][7] = backRank[file]; pieceColor[file][7] = COLOR_BLACK;
	}
}

#endif
