/*
    This file is part of Cute Makruk.

    Cute Makruk is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
*/

#ifndef SPRTDIALOG_H
#define SPRTDIALOG_H

#include <QDialog>
#include <timecontrol.h>

class QComboBox;
class QDoubleSpinBox;
class QSpinBox;
class QLineEdit;
class QDialogButtonBox;
class EngineManager;
class GameManager;
class Tournament;

/*!
 * Engine vs engine match that stops on an SPRT decision.
 * Games are played without a board (no live view); results and the
 * saved games can be looked at afterwards.
 */
class SprtDialog : public QDialog
{
	Q_OBJECT

	public:
		SprtDialog(EngineManager* engineManager,
			   int baseMs, int incMs,
			   QWidget* parent = nullptr);

		/*! Creates the match; the caller owns and starts it. */
		Tournament* createTournament(GameManager* gameManager) const;
		/*! The PGN file the games are saved to. */
		QString pgnFile() const;

		/*! "10+0.1" style label of the time control. */
		static QString tcLabel(int baseMs, int incMs);

	private:
		void updateOk();
		void writeSettings() const;

		EngineManager* m_engineManager;
		TimeControl m_tc;
		QString m_tcLabel;

		QComboBox* m_engineA;
		QComboBox* m_engineB;
		QDoubleSpinBox* m_elo0;
		QDoubleSpinBox* m_elo1;
		QDoubleSpinBox* m_alpha;
		QDoubleSpinBox* m_beta;
		QSpinBox* m_maxGames;
		QSpinBox* m_concurrency;
		QSpinBox* m_threads;
		QLineEdit* m_openingsEdit;
		QLineEdit* m_pgnEdit;
		QDialogButtonBox* m_buttons;
};

#endif // SPRTDIALOG_H
