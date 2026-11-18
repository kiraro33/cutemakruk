/*
    This file is part of cutemakruk, a Makruk-only fork of Cute Chess.

    cutemakruk is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    cutemakruk is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with cutemakruk.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
 * makrukjudge: test driver for cutemakruk's Makruk board (Thai rules 2569).
 *
 *   makrukjudge pos         stdin lines "<fen>|<uci moves>", one output
 *                           line per input: fen, result, reason, legal moves
 *   makrukjudge pgn FILE..  for each game: the first position (from the
 *                           start) that is over by the Thai rules
 */

#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <algorithm>
#include <memory>

#include <board/board.h>
#include <board/boardfactory.h>
#include <board/makrukboard.h>
#include <pgngame.h>
#include <pgnstream.h>

static QString resultToken(const Chess::Result& r)
{
	return r.isNone() ? "*" : r.toShortString();
}

static QString lan(Chess::Board* board, const Chess::Move& m)
{
	return board->moveString(m, Chess::Board::LongAlgebraic);
}

// Finds the legal move whose from-to part (first 4 chars) matches \a uci
static Chess::Move findMove(Chess::Board* board, const QString& uci)
{
	const auto moves = board->legalMoves();
	for (const auto& m : moves)
	{
		if (lan(board, m).left(4) == uci.left(4))
			return m;
	}
	return Chess::Move();
}

static int posMode()
{
	QTextStream in(stdin), out(stdout);
	QString line;
	while (in.readLineInto(&line))
	{
		line = line.trimmed();
		if (line.isEmpty())
			continue;
		const QStringList parts = line.split('|');
		std::unique_ptr<Chess::Board> board(
			Chess::BoardFactory::create("makruk"));
		if (!board->setFenString(parts.at(0)))
		{
			out << "error=bad fen\n" << Qt::flush;
			continue;
		}
		QString err;
		if (parts.size() > 1)
		{
			const auto moves = parts.at(1).split(' ', Qt::SkipEmptyParts);
			for (const auto& mv : moves)
			{
				Chess::Move m = findMove(board.get(), mv);
				if (m.isNull())
				{
					err = "illegal move " + mv;
					break;
				}
				board->makeMove(m);
			}
		}
		if (!err.isEmpty())
		{
			out << "error=" << err << "\n" << Qt::flush;
			continue;
		}
		QStringList legal;
		const auto moves = board->legalMoves();
		for (const auto& m : moves)
			legal << lan(board.get(), m);
		legal.sort();
		Chess::Result r = board->result();
		out << "fen=" << board->fenString()
		    << "|result=" << resultToken(r)
		    << "|reason=" << r.description()
		    << "|counting=" << static_cast<Chess::MakrukBoard*>(board.get())
					->thaiCountingString()
		    << "|legal=" << legal.join(' ') << "\n" << Qt::flush;
	}
	return 0;
}

static QString csv(QString s)
{
	if (s.contains(',') || s.contains('"'))
		return '"' + s.replace('"', "\"\"") + '"';
	return s;
}

static int pgnMode(const QStringList& files)
{
	QTextStream out(stdout);
	out << "file,game,pgn_result,result,reason,ply,plies,error\n";
	for (const auto& fn : files)
	{
		QFile f(fn);
		if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			out << csv(fn) << ",0,,,,,,cannot open\n";
			continue;
		}
		PgnStream stream(&f);
		int gi = 0;
		while (true)
		{
			PgnGame game;
			if (!game.read(stream, INT_MAX - 1, false))
				break;
			gi++;
			QString pgnRes = game.tagValue("Result");
			std::unique_ptr<Chess::Board> board(game.createBoard());
			if (!board)
			{
				out << csv(fn) << ',' << gi << ',' << pgnRes
				    << ",,,,,cannot create board\n";
				continue;
			}
			const auto& moves = game.moves();
			Chess::Result end;
			int endPly = -1;
			QString err;
			for (int ply = 0; ply <= moves.size(); ply++)
			{
				if (endPly < 0)
				{
					Chess::Result r = board->result();
					if (!r.isNone())
					{
						end = r;
						endPly = ply;
					}
				}
				if (ply == moves.size())
					break;
				Chess::Move m = board->moveFromGenericMove(moves.at(ply).move);
				if (m.isNull() || !board->isLegalMove(m))
				{
					err = QString("illegal move %1").arg(ply + 1);
					break;
				}
				board->makeMove(m);
			}
			out << csv(fn) << ',' << gi << ',' << pgnRes << ','
			    << resultToken(end) << ',' << csv(end.description()) << ','
			    << (endPly < 0 ? QString() : QString::number(endPly)) << ','
			    << moves.size() << ',' << csv(err) << '\n';
		}
	}
	out.flush();
	return 0;
}

int main(int argc, char* argv[])
{
	QCoreApplication app(argc, argv);
	QStringList args = app.arguments();
	args.takeFirst();
	if (!args.isEmpty() && args.first() == "pos")
		return posMode();
	if (args.size() > 1 && args.first() == "pgn")
		return pgnMode(args.mid(1));
	QTextStream(stderr) << "usage: makrukjudge pos < lines | makrukjudge pgn FILE...\n";
	return 2;
}
