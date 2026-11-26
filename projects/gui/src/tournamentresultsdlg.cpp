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

#include "tournamentresultsdlg.h"

#include <limits>

#include <QPlainTextEdit>
#include <QBoxLayout>
#include <QFont>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QPushButton>

#include <tournament.h>

#include "cutechessapp.h"
#include "gamedatabasemanager.h"

TournamentResultsDialog::TournamentResultsDialog(QWidget* parent)
	: QDialog(parent)
{
	setWindowTitle(tr("Tournament Results"));

	m_resultsEdit = new QPlainTextEdit(this);
	m_resultsEdit->setReadOnly(true);

	QFont font("Courier New");
	font.setStyleHint(QFont::Monospace);
	font.setPointSize(font.pointSize() - 1);
	m_resultsEdit->document()->setDefaultFont(font);

	m_viewGamesBtn = new QPushButton(tr("View Games"), this);
	m_viewGamesBtn->setToolTip(tr("Open the saved games in the Game Database"));
	m_viewGamesBtn->hide();
	connect(m_viewGamesBtn, &QPushButton::clicked, this, [this]()
	{
		auto app = CuteChessApplication::instance();
		app->gameDatabaseManager()->importPgnFile(m_gamesFile);
		app->showGameDatabaseDialog();
		// Import once per match
		m_viewGamesBtn->setEnabled(false);
	});

	auto buttons = new QHBoxLayout;
	buttons->setContentsMargins(6, 0, 6, 6);
	buttons->addStretch();
	buttons->addWidget(m_viewGamesBtn);

	auto layout = new QBoxLayout(QBoxLayout::TopToBottom);
	layout->addWidget(m_resultsEdit);
	layout->addLayout(buttons);
	layout->setContentsMargins(0, 0, 0, 0);

	setLayout(layout);
	resize(1100, 400);
}

TournamentResultsDialog::~TournamentResultsDialog()
{
}

void TournamentResultsDialog::setTournament(Tournament* tournament,
					    const QString& gamesFile)
{
	setWindowTitle(tournament->name());
	m_resultsEdit->setPlainText(tournament->results());

	m_gamesFile = gamesFile;
	m_viewGamesBtn->setVisible(!gamesFile.isEmpty());
	m_viewGamesBtn->setEnabled(false);
	if (gamesFile.isEmpty())
		return;

	// The games can be looked at once the match is over
	connect(tournament, &Tournament::finished, this, [this]()
	{
		m_viewGamesBtn->setEnabled(QFileInfo::exists(m_gamesFile));
	});
}

void TournamentResultsDialog::update()
{
	auto tournament = qobject_cast<Tournament*>(QObject::sender());
	Q_ASSERT(tournament != nullptr);
	QString text;

	// A quick fix, copied from the CLI side.
	if (tournament->playerCount() == 2 && tournament->type() != "knockout")
	{
		TournamentPlayer fcp = tournament->playerAt(0);
		TournamentPlayer scp = tournament->playerAt(1);
		int totalResults = fcp.gamesFinished();
		double scoreRatio = std::numeric_limits<double>::quiet_NaN();
		if (totalResults > 0)
			scoreRatio = double(fcp.score()) / (totalResults * 2);
		text = tr("Score of %1 vs %2: %3 - %4 - %5 [%6]\n") // clazy:exclude=qstring-arg
		       .arg(fcp.name())
		       .arg(scp.name())
		       .arg(fcp.wins())
		       .arg(scp.wins())
		       .arg(fcp.draws())
		       .arg(scoreRatio, 0, 'f', 3);
	}

	text += tournament->results();
	text += tr("\n%1 of %2 games finished.")
		.arg(tournament->finishedGameCount())
		.arg(tournament->finalGameCount());
	if (tournament->finishedGameCount() >= tournament->finalGameCount()
	|| tournament->isStopping())
		text.append("\n").append(tournament->outcomes());
	m_resultsEdit->setPlainText(text);
}
