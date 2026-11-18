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

#include "gameviewer.h"
#include "themeicon.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QToolButton>
#include <QSlider>
#include <QMessageBox>
#include <QSettings>
#include <QLabel>
#include <board/makrukboard.h>
#include <pgngame.h>
#include <chessgame.h>
#include <chessplayer.h>
#include "boardview/boardscene.h"
#include "boardview/boardview.h"
#include "chessclock.h"

GameViewer::GameViewer(Qt::Orientation orientation,
                       QWidget* parent,
                       bool addChessClock)
	: QWidget(parent),
	  m_moveNumberSlider(new QSlider(Qt::Horizontal)),
	  m_viewFirstMoveBtn(new QToolButton),
	  m_viewPreviousMoveBtn(new QToolButton),
	  m_viewNextMoveBtn(new QToolButton),
	  m_viewLastMoveBtn(new QToolButton),
	  m_statusLabel(nullptr),
	  m_moveIndex(0),
	  m_humanGame(false)
{
	#ifdef Q_OS_MAC
	setStyleSheet("QToolButton:!hover { border: none; }");
	#endif

	m_boardScene = new BoardScene(this);
	m_boardView = new BoardView(m_boardScene, this);
	m_boardView->setEnabled(false);

	m_viewFirstMoveBtn->setEnabled(false);
	m_viewFirstMoveBtn->setAutoRaise(true);
	m_viewFirstMoveBtn->setMinimumSize(32, 32);
	m_viewFirstMoveBtn->setToolTip(tr("Skip to the beginning"));
	setThemeIcon(m_viewFirstMoveBtn, "chevron-first");
	connect(m_viewFirstMoveBtn, SIGNAL(clicked()),
		this, SLOT(viewFirstMoveClicked()));

	m_viewPreviousMoveBtn->setEnabled(false);
	m_viewPreviousMoveBtn->setAutoRaise(true);
	m_viewPreviousMoveBtn->setMinimumSize(32, 32);
	m_viewPreviousMoveBtn->setToolTip(tr("Previous move"));
	setThemeIcon(m_viewPreviousMoveBtn, "chevron-left");
	connect(m_viewPreviousMoveBtn, SIGNAL(clicked()),
		this, SLOT(viewPreviousMoveClicked()));

	m_viewNextMoveBtn->setEnabled(false);
	m_viewNextMoveBtn->setAutoRaise(true);
	m_viewNextMoveBtn->setMinimumSize(32, 32);
	m_viewNextMoveBtn->setToolTip(tr("Next move"));
	setThemeIcon(m_viewNextMoveBtn, "chevron-right");
	connect(m_viewNextMoveBtn, SIGNAL(clicked()),
		this, SLOT(viewNextMoveClicked()));

	m_viewLastMoveBtn->setEnabled(false);
	m_viewLastMoveBtn->setAutoRaise(true);
	m_viewLastMoveBtn->setMinimumSize(32, 32);
	m_viewLastMoveBtn->setToolTip(tr("Skip to the end"));
	setThemeIcon(m_viewLastMoveBtn, "chevron-last");
	connect(m_viewLastMoveBtn, SIGNAL(clicked()),
		this, SLOT(viewLastMoveClicked()));

	m_moveNumberSlider->setEnabled(false);
	#ifdef Q_OS_WIN
	m_moveNumberSlider->setMinimumHeight(18);
	#endif
	connect(m_moveNumberSlider, SIGNAL(valueChanged(int)),
		this, SLOT(viewPositionClicked(int)));

	QHBoxLayout* controls = new QHBoxLayout();
	controls->setContentsMargins(0, 0, 0, 0);
	controls->setSpacing(0);
	controls->addWidget(m_viewFirstMoveBtn);
	controls->addWidget(m_viewPreviousMoveBtn);
	controls->addWidget(m_viewNextMoveBtn);
	controls->addWidget(m_viewLastMoveBtn);

	QVBoxLayout* layout = new QVBoxLayout();
	layout->setContentsMargins(0, 0, 0, 0);

	if (addChessClock)
	{
		QHBoxLayout* clockLayout = new QHBoxLayout();
		for (int i = 0; i < 2; i++)
		{
			m_chessClock[i] = new ChessClock();
			clockLayout->addWidget(m_chessClock[i]);
		}
		clockLayout->insertSpacing(1, 20);
		layout->addLayout(clockLayout);

		// Thai rules: the running count, or the result when the game is over
		m_statusLabel = new QLabel();
		m_statusLabel->setAlignment(Qt::AlignCenter);
		m_statusLabel->setMinimumHeight(m_statusLabel->fontMetrics().height() + 8);
		layout->addWidget(m_statusLabel);
	}
	else
	{
		m_chessClock[0] = nullptr;
		m_chessClock[1] = nullptr;
	}

	layout->addWidget(m_boardView);

	if (orientation == Qt::Horizontal)
	{
		controls->addSpacing(6);
		controls->addWidget(m_moveNumberSlider);
	}
	else
	{
		layout->addWidget(m_moveNumberSlider);

		controls->insertStretch(0);
		controls->addStretch();
	}

	layout->addLayout(controls);
	setLayout(layout);
}

ChessClock* GameViewer::chessClock(Chess::Side side)
{
	return m_chessClock[side];
}

void GameViewer::autoFlip()
{
	if (QSettings().value("ui/auto_flip_board_for_human_games",false).toBool())
		m_boardScene->flip();
}

void GameViewer::setGame(ChessGame* game)
{
	Q_ASSERT(game != nullptr);

	setGame(game->pgn());
	m_game = game;

	connect(m_game, SIGNAL(fenChanged(QString)),
		this, SLOT(onFenChanged(QString)));
	connect(m_game, SIGNAL(moveMade(Chess::GenericMove, QString, QString)),
		this, SLOT(onMoveMade(Chess::GenericMove)));
	connect(m_game, SIGNAL(humanEnabled(bool)),
		m_boardView, SLOT(setEnabled(bool)));

	for (int i = 0; i < 2; i++)
	{
		ChessPlayer* player(m_game->player(Chess::Side::Type(i)));
		if (player->isHuman())
			connect(m_boardScene, SIGNAL(humanMove(Chess::GenericMove, Chess::Side)),
				player, SLOT(onHumanMove(Chess::GenericMove, Chess::Side)));
	}

	connect(m_game, SIGNAL(finished(ChessGame*, Chess::Result)),
		m_boardScene, SLOT(onGameFinished(ChessGame*, Chess::Result)));
	connect(m_game, &ChessGame::finished, this,
		[this](ChessGame*, const Chess::Result& result)
	{
		m_result = result;
		updateStatus();
	});
	m_result = m_game->result();
	updateStatus();
	m_boardView->setEnabled(!m_game->isFinished() &&
				m_game->playerToMove()->isHuman());
	m_humanGame = !m_game.isNull()
		    && m_game->playerToMove()->isHuman()
		    && m_game->playerToWait()->isHuman();

	if (m_humanGame && m_game->playerToMove() != m_game->player(Chess::Side::White))
		autoFlip();
}

void GameViewer::setGame(const PgnGame* pgn)
{
	Q_ASSERT(pgn != nullptr);

	disconnectGame();

	auto board = pgn->createBoard();
	if (board)
	{
		m_boardScene->setBoard(board);
		m_boardScene->populate();
	}
	else
	{
		QMessageBox::critical(
			this, tr("Cannot show game"),
			tr("This game is incompatible with Cute Makruk and cannot be shown."));
	}
	m_moveIndex = 0;

	m_moves.clear();
	for (const PgnGame::MoveData& md : pgn->moves())
		m_moves.append(md.move);

	m_viewFirstMoveBtn->setEnabled(false);
	m_viewPreviousMoveBtn->setEnabled(false);
	m_viewNextMoveBtn->setEnabled(!m_moves.isEmpty());
	m_viewLastMoveBtn->setEnabled(!m_moves.isEmpty());

	m_moveNumberSlider->setEnabled(!m_moves.isEmpty());
	m_moveNumberSlider->setMaximum(m_moves.count());
	m_moveNumberSlider->setValue(0);

	m_result = pgn->result();
	viewLastMove();
	updateStatus();
}

void GameViewer::disconnectGame()
{
	m_boardView->setEnabled(false);
	if (m_game.isNull())
		return;

	disconnect(m_game, nullptr, m_boardScene, nullptr);
	disconnect(m_game, nullptr, m_boardView, nullptr);
	disconnect(m_game, nullptr, this, nullptr);

	for (int i = 0; i < 2; i++)
	{
		ChessPlayer* player(m_game->player(Chess::Side::Type(i)));
		if (player != nullptr)
			disconnect(m_boardScene, nullptr, player, nullptr);
	}

	m_game = nullptr;
}

Chess::Board* GameViewer::board() const
{
	return m_boardScene->board();
}

BoardScene* GameViewer::boardScene() const
{
	return m_boardScene;
}

void GameViewer::viewFirstMoveClicked()
{
	viewFirstMove();
	emit moveSelected(m_moveIndex - 1);
}

void GameViewer::viewFirstMove()
{
	while (m_moveIndex > 0)
		viewPreviousMove();
}

void GameViewer::viewPreviousMoveClicked()
{
	viewPreviousMove();
	emit moveSelected(m_moveIndex - 1);
}

void GameViewer::viewPreviousMove()
{
	m_moveIndex--;

	if (m_moveIndex == 0)
	{
		m_viewPreviousMoveBtn->setEnabled(false);
		m_viewFirstMoveBtn->setEnabled(false);
	}

	m_boardScene->undoMove();

	m_viewNextMoveBtn->setEnabled(true);
	m_viewLastMoveBtn->setEnabled(true);
	m_boardView->setEnabled(false);

	m_moveNumberSlider->setSliderPosition(m_moveIndex);
	updateStatus();
}

void GameViewer::viewNextMoveClicked()
{
	viewNextMove();
	emit moveSelected(m_moveIndex - 1);
}

void GameViewer::viewNextMove()
{
	m_boardScene->makeMove(m_moves.at(m_moveIndex++));

	m_viewPreviousMoveBtn->setEnabled(true);
	m_viewFirstMoveBtn->setEnabled(true);

	if (m_moveIndex >= m_moves.count())
	{
		m_viewNextMoveBtn->setEnabled(false);
		m_viewLastMoveBtn->setEnabled(false);

		if (!m_game.isNull()
		&&  !m_game->isFinished() && m_game->playerToMove()->isHuman())
			m_boardView->setEnabled(true);
	}

	m_moveNumberSlider->setSliderPosition(m_moveIndex);
	updateStatus();
}

void GameViewer::viewLastMoveClicked()
{
	viewLastMove();
	emit moveSelected(m_moveIndex - 1);
}

void GameViewer::viewLastMove()
{
	while (m_moveIndex < m_moves.count())
		viewNextMove();
}

void GameViewer::viewPositionClicked(int index)
{
	viewPosition(index);
	emit moveSelected(m_moveIndex - 1);
}

void GameViewer::viewPosition(int index)
{
	if (m_moves.isEmpty())
		return;

	while (index < m_moveIndex)
		viewPreviousMove();
	while (index > m_moveIndex)
		viewNextMove();
}

void GameViewer::viewMove(int index, bool keyLeft)
{
	Q_ASSERT(index >= 0);
	Q_ASSERT(!m_moves.isEmpty());

	if (keyLeft && index == m_moveIndex - 2)
		viewPreviousMove();
	else if (index < m_moveIndex)
	{
		// We backtrack one move too far and then make one
		// move forward to highlight the correct move
		while (index < m_moveIndex)
			viewPreviousMove();
		viewNextMove();
	}
	else
	{
		while (index + 1 > m_moveIndex)
			viewNextMove();
	}
}

void GameViewer::onFenChanged(const QString& fen)
{
	m_moves.clear();
	m_moveIndex = 0;

	m_viewFirstMoveBtn->setEnabled(false);
	m_viewPreviousMoveBtn->setEnabled(false);
	m_viewNextMoveBtn->setEnabled(false);
	m_viewLastMoveBtn->setEnabled(false);
	m_moveNumberSlider->setEnabled(false);
	m_moveNumberSlider->setMaximum(0);

	m_boardScene->setFenString(fen);
	m_result = Chess::Result();
	updateStatus();
}

void GameViewer::updateStatus()
{
	if (m_statusLabel == nullptr)
		return;

	QString text;
	QString color;
	if (!m_result.isNone() && m_moveIndex >= m_moves.count())
	{
		// result texts come from the library in English (they also go to
		// the PGN); show them translated
		auto translated = [](const QString& s)
		{
			return QCoreApplication::translate("GameResult",
							   s.toUtf8().constData());
		};
		QString reason = m_result.description();
		QString thai = translated(reason);
		// e.g. "Draw by adjudication: maximal game length"
		int sep = reason.indexOf(": ");
		if (thai == reason && sep > 0)
			thai = translated(reason.left(sep)) + ": "
			     + translated(reason.mid(sep + 2));
		reason = thai;
		text = reason.isEmpty() ? m_result.toShortString()
					: QString("%1  %2").arg(m_result.toShortString(), reason);
	}
	else if (auto board = dynamic_cast<Chess::MakrukBoard*>(this->board()))
	{
		const auto info = board->thaiCountInfo();
		if (info.type != 0)
		{
			const QString side = info.side == Chess::Side::White
					     ? tr("White") : tr("Black");
			text = (info.type == 1
				? tr("Board's honour counting (%1 counts): %2 / %3")
				: tr("Pieces' honour counting (%1 counts): %2 / %3"))
				.arg(side).arg(info.count).arg(info.limit);
			int left = info.limit - info.count;
			if (left <= 3)
				color = "#d9534f";
			else if (left <= 8)
				color = "#e8a33d";
		}
	}

	m_statusLabel->setText(text);
	m_statusLabel->setStyleSheet(color.isEmpty()
		? QString("QLabel { font-weight: bold; }")
		: QString("QLabel { font-weight: bold; color: %1; }").arg(color));
}

void GameViewer::onMoveMade(const Chess::GenericMove& move)
{
	m_moves.append(move);

	m_moveNumberSlider->setEnabled(true);
	m_moveNumberSlider->setMaximum(m_moves.count());

	if (m_moveIndex == m_moves.count() - 1)
		viewNextMove();

	if (m_humanGame)
		autoFlip();
}
