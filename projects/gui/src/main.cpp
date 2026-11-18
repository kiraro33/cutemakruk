/*
    This file is part of Cute Chess.
    Copyright (C) 2008-2018 Cute Chess authors

    Cute Chess is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Cute Chess is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Cute Chess.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "cutechessapp.h"

#include <QLoggingCategory>
#include <QTextStream>
#include <QStringList>
#include <QMetaType>
#include <QTranslator>
#include <QSysInfo>
#include <QFile>
#include <QHash>
#include <QSet>
#include <QSettings>
#include <QStyleHints>

#include <board/genericmove.h>
#include <board/move.h>
#include <board/side.h>
#include <board/result.h>
#include <moveevaluation.h>

/*!
 * Translates the GUI from a table (res/i18n/<lang>.tsv: English source, TAB,
 * translation). The library's texts (game results written to PGN files,
 * time controls, engine errors) are left in English.
 */
class TableTranslator : public QTranslator
{
	public:
		bool loadTable(const QString& path)
		{
			QFile file(path);
			if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
				return false;
			const QStringList lines = QString::fromUtf8(file.readAll()).split('\n');
			for (QString line : lines)
			{
				if (line.endsWith('\r'))
					line.chop(1);
				if (line.isEmpty() || line.startsWith('#'))
					continue;
				int tab = line.indexOf('\t');
				if (tab > 0)
					m_table.insert(unescape(line.left(tab)),
						       unescape(line.mid(tab + 1)));
			}
			return !m_table.isEmpty();
		}

		bool isEmpty() const override
		{
			return m_table.isEmpty();
		}

		QString translate(const char* context,
				  const char* sourceText,
				  const char* disambiguation = nullptr,
				  int n = -1) const override
		{
			Q_UNUSED(disambiguation);
			Q_UNUSED(n);
			static const QSet<QString> libContexts = {
				"Board", "Result", "Side", "EngineBuilder",
				"ChessEngine", "ChessGame", "ChessPlayer", "EngineManager",
				"EngineOption", "EngineProcess", "GameInitializer",
				"GameManager", "GameThread", "GauntletTournament",
				"HumanPlayer", "KnockoutTournament", "MoveEvaluation",
				"OpeningBook", "OpeningSuite", "PipeReader", "PlayerBuilder",
				"PyramidTournament", "ResultFormatter", "RoundRobinTournament",
				"Sprt", "Tournament", "UciEngine", "Worker", "XboardEngine"
			};
			if (sourceText == nullptr)
				return QString();
			QString ctx = QString::fromLatin1(context);
			ctx = ctx.mid(ctx.lastIndexOf(':') + 1);
			if (libContexts.contains(ctx))
				return QString();
			return m_table.value(QString::fromUtf8(sourceText));
		}

	private:
		static QString unescape(const QString& s)
		{
			QString out;
			for (int i = 0; i < s.size(); i++)
			{
				if (s[i] == '\\' && i + 1 < s.size())
				{
					QChar c = s[++i];
					out += c == 'n' ? QChar('\n') : c == 't' ? QChar('\t') : c;
				}
				else
					out += s[i];
			}
			return out;
		}

		QHash<QString, QString> m_table;
};

int main(int argc, char* argv[])
{
	// Register types for signal / slot connections
	qRegisterMetaType<Chess::GenericMove>("Chess::GenericMove");
	qRegisterMetaType<Chess::Move>("Chess::Move");
	qRegisterMetaType<Chess::Side>("Chess::Side");
	qRegisterMetaType<Chess::Result>("Chess::Result");
	qRegisterMetaType<MoveEvaluation>("MoveEvaluation");

	QLoggingCategory::defaultCategory()->setEnabled(QtDebugMsg, true);

	CuteChessApplication app(argc, argv);

	QStringList arguments = app.arguments();
	arguments.takeFirst(); // application name

	// Thai by default; "--lang en" or View > Language switches to English
	QString lang = QSettings().value("ui/language", "th").toString();
	int langArg = arguments.indexOf("--lang");
	if (langArg >= 0 && langArg + 1 < arguments.size())
	{
		lang = arguments.at(langArg + 1);
		arguments.remove(langArg, 2);
	}
	TableTranslator translator;
	if (lang != "en" && translator.loadTable(":/i18n/" + lang + ".tsv"))
		app.installTranslator(&translator);

	// View > Theme: dark (default), light or follow Windows
	const QString theme = QSettings().value("ui/theme", "dark").toString();
	if (theme == "light")
		QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Light);
	else if (theme == "dark")
		QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Dark);

	// Use trivial command-line parsing for now
	QTextStream out(stdout);
	while (!arguments.isEmpty())
	{
		if (arguments.first() == QLatin1String("-v") ||
			arguments.first() == QLatin1String("--version"))
		{
			out << "Cute Makruk " << CUTEMAKRUK_VERSION << " (GUI)" << '\n';
			out << "Makruk only, Thai association rules 2569" << '\n';
			out << "Based on Cute Chess " << CUTECHESS_VERSION << '\n';
			out << "Using Qt version " << qVersion() << '\n';
			out << "Running on " << QSysInfo::prettyProductName();
			out << "/" << QSysInfo::currentCpuArchitecture() << '\n' << '\n';
			out << "Copyright (C) 2008-2020 Ilari Pihlajisto, Arto Jonsson ";
			out << "and contributors" << '\n';
			out << "Cute Makruk changes Copyright (C) 2026 Kiraro33 (O.T.)" << '\n';
			out << "Makruk piece images by Ada Joule (Fulmene), CC BY-SA 4.0" << '\n';
			out << "This is free software; see the source for copying ";
			out << "conditions.  There is NO" << '\n' << "warranty; not even for ";
			out << "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.";
			out << '\n' << '\n';

			return 0;
		}
		else
		{
			out << "Unknown argument: " << arguments.first() << '\n';
		}
		arguments.takeFirst();
	}
	app.newDefaultGame();
	return app.exec();
}
