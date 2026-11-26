/*
    This file is part of Cute Makruk.

    Cute Makruk is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
*/

#include "sprtdialog.h"

#include <QComboBox>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QStandardPaths>
#include <QThread>
#include <QVBoxLayout>

#include <enginebuilder.h>
#include <enginemanager.h>
#include <gamemanager.h>
#include <openingsuite.h>
#include <sprt.h>
#include <tournament.h>
#include <tournamentfactory.h>

static const char* s_variant = "makruk";

QString SprtDialog::tcLabel(int baseMs, int incMs)
{
	return QString("%1+%2").arg(baseMs / 1000.0).arg(incMs / 1000.0);
}

SprtDialog::SprtDialog(EngineManager* engineManager, int baseMs, int incMs,
		       QWidget* parent)
	: QDialog(parent),
	  m_engineManager(engineManager),
	  m_tcLabel(tcLabel(baseMs, incMs))
{
	Q_ASSERT(engineManager != nullptr);

	m_tc.setMovesPerTc(0);
	m_tc.setTimePerTc(baseMs);
	m_tc.setTimeIncrement(incMs);

	setWindowTitle(tr("AI vs AI SPRT - %1").arg(m_tcLabel));

	QSettings s;
	s.beginGroup("sprt");

	m_engineA = new QComboBox(this);
	m_engineB = new QComboBox(this);
	const auto engines = engineManager->engines();
	for (const auto& e : engines)
	{
		if (!e.supportsVariant(s_variant))
			continue;
		m_engineA->addItem(e.name());
		m_engineB->addItem(e.name());
	}
	int ia = m_engineA->findText(s.value("engine_a").toString());
	int ib = m_engineB->findText(s.value("engine_b").toString());
	if (ia < 0)
		ia = 0;
	if (ib < 0)
		ib = m_engineB->count() > 1 ? 1 : 0;
	m_engineA->setCurrentIndex(ia);
	m_engineB->setCurrentIndex(ib);

	auto dspin = [this](double min, double max, double step, int dec, double val)
	{
		auto sp = new QDoubleSpinBox(this);
		sp->setRange(min, max);
		sp->setSingleStep(step);
		sp->setDecimals(dec);
		sp->setValue(val);
		return sp;
	};
	m_elo0 = dspin(-1000, 1000, 1, 1, s.value("elo0", 0.0).toDouble());
	m_elo1 = dspin(-1000, 1000, 1, 1, s.value("elo1", 5.0).toDouble());
	m_alpha = dspin(0.001, 0.5, 0.01, 3, s.value("alpha", 0.05).toDouble());
	m_beta = dspin(0.001, 0.5, 0.01, 3, s.value("beta", 0.05).toDouble());

	m_maxGames = new QSpinBox(this);
	m_maxGames->setRange(2, 1000000);
	m_maxGames->setSingleStep(2);
	m_maxGames->setValue(s.value("max_games", 20000).toInt());
	m_maxGames->setToolTip(tr("The match stops earlier as soon as SPRT decides."));

	m_concurrency = new QSpinBox(this);
	m_concurrency->setRange(1, 256);
	m_concurrency->setValue(QSettings().value("tournament/concurrency", 1).toInt());

	const int cores = qMax(1, QThread::idealThreadCount());
	m_threads = new QSpinBox(this);
	m_threads->setRange(1, 1024);
	m_threads->setValue(s.value("threads", 1).toInt());
	m_threads->setToolTip(tr("Engine \"Threads\" option for this match only;\n"
				 "the engine's saved settings are not changed."));

	auto coresLabel = new QLabel(this);
	auto updateCores = [=]()
	{
		const int used = m_threads->value() * m_concurrency->value();
		coresLabel->setText(tr("About %1 of %2 CPU threads in use")
			.arg(used).arg(cores));
	};
	connect(m_threads, &QSpinBox::valueChanged, this, updateCores);
	connect(m_concurrency, &QSpinBox::valueChanged, this, updateCores);
	updateCores();

	m_openingsEdit = new QLineEdit(s.value("openings").toString(), this);
	m_openingsEdit->setPlaceholderText(tr("Start position only"));
	m_openingsEdit->setToolTip(tr("EPD or PGN file of openings, played in random order;\n"
				      "each opening is played twice with colours swapped."));
	auto openingsBtn = new QPushButton(tr("Browse..."), this);
	connect(openingsBtn, &QPushButton::clicked, this, [this]()
	{
		const QString f = QFileDialog::getOpenFileName(this, tr("Select openings file"),
			QFileInfo(m_openingsEdit->text()).absolutePath(),
			tr("Openings (*.epd *.pgn);;All Files (*)"));
		if (!f.isEmpty())
			m_openingsEdit->setText(QDir::toNativeSeparators(f));
	});
	auto openingsRow = new QHBoxLayout;
	openingsRow->addWidget(m_openingsEdit);
	openingsRow->addWidget(openingsBtn);

	const QString dir = QStandardPaths::writableLocation(
		QStandardPaths::DocumentsLocation) + "/Cute Makruk/SPRT";
	const QString pgnName = QString("sprt-%1-%2.pgn")
		.arg(m_tcLabel, QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss"));
	m_pgnEdit = new QLineEdit(QDir::toNativeSeparators(dir + "/" + pgnName), this);
	m_pgnEdit->setToolTip(tr("The games are saved here, to look at after they end."));
	auto pgnBtn = new QPushButton(tr("Browse..."), this);
	connect(pgnBtn, &QPushButton::clicked, this, [this]()
	{
		const QString f = QFileDialog::getSaveFileName(this, tr("Select PGN output file"),
			m_pgnEdit->text(), tr("Portable Game Notation (*.pgn)"));
		if (!f.isEmpty())
			m_pgnEdit->setText(QDir::toNativeSeparators(f));
	});
	auto pgnRow = new QHBoxLayout;
	pgnRow->addWidget(m_pgnEdit);
	pgnRow->addWidget(pgnBtn);

	s.endGroup();

	auto form = new QFormLayout;
	form->addRow(tr("Time control:"), new QLabel(m_tcLabel, this));
	form->addRow(tr("Test engine (new):"), m_engineA);
	form->addRow(tr("Base engine (old):"), m_engineB);
	form->addRow(tr("Elo0 (H0):"), m_elo0);
	form->addRow(tr("Elo1 (H1):"), m_elo1);
	form->addRow(tr("Alpha:"), m_alpha);
	form->addRow(tr("Beta:"), m_beta);
	form->addRow(tr("Maximum games:"), m_maxGames);
	form->addRow(tr("Concurrent games:"), m_concurrency);
	form->addRow(tr("Threads per engine:"), m_threads);
	form->addRow(QString(), coresLabel);
	form->addRow(tr("Openings:"), openingsRow);
	form->addRow(tr("Save games to:"), pgnRow);

	auto note = new QLabel(tr("Games are not shown while they are played. "
				  "See Tournament > Results... during and after the match."), this);
	note->setWordWrap(true);

	m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	m_buttons->button(QDialogButtonBox::Ok)->setText(tr("Start"));
	connect(m_buttons, &QDialogButtonBox::accepted, this, [this]()
	{
		const QString openings = m_openingsEdit->text().trimmed();
		if (!openings.isEmpty() && !QFileInfo::exists(openings))
		{
			QMessageBox::warning(this, windowTitle(),
				tr("Openings file not found:\n%1").arg(openings));
			return;
		}
		writeSettings();
		accept();
	});
	connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

	auto layout = new QVBoxLayout(this);
	layout->addLayout(form);
	layout->addWidget(note);
	layout->addWidget(m_buttons);

	connect(m_engineA, &QComboBox::currentIndexChanged, this, &SprtDialog::updateOk);
	connect(m_engineB, &QComboBox::currentIndexChanged, this, &SprtDialog::updateOk);
	connect(m_elo0, &QDoubleSpinBox::valueChanged, this, &SprtDialog::updateOk);
	connect(m_elo1, &QDoubleSpinBox::valueChanged, this, &SprtDialog::updateOk);
	connect(m_pgnEdit, &QLineEdit::textChanged, this, &SprtDialog::updateOk);
	updateOk();
	resize(520, sizeHint().height());
}

void SprtDialog::updateOk()
{
	bool ok = m_engineA->count() >= 2
		&& m_engineA->currentText() != m_engineB->currentText()
		&& m_elo0->value() < m_elo1->value()
		&& !m_pgnEdit->text().trimmed().isEmpty();
	m_buttons->button(QDialogButtonBox::Ok)->setEnabled(ok);
}

void SprtDialog::writeSettings() const
{
	QSettings s;
	s.beginGroup("sprt");
	s.setValue("engine_a", m_engineA->currentText());
	s.setValue("engine_b", m_engineB->currentText());
	s.setValue("elo0", m_elo0->value());
	s.setValue("elo1", m_elo1->value());
	s.setValue("alpha", m_alpha->value());
	s.setValue("beta", m_beta->value());
	s.setValue("max_games", m_maxGames->value());
	s.setValue("threads", m_threads->value());
	s.setValue("openings", m_openingsEdit->text());
	s.endGroup();
	QSettings().setValue("tournament/concurrency", m_concurrency->value());
}

QString SprtDialog::pgnFile() const
{
	return QDir::fromNativeSeparators(m_pgnEdit->text().trimmed());
}

Tournament* SprtDialog::createTournament(GameManager* gameManager) const
{
	Q_ASSERT(gameManager != nullptr);

	gameManager->setConcurrency(m_concurrency->value());

	auto t = TournamentFactory::create("round-robin", gameManager, parent());
	const QString a = m_engineA->currentText();
	const QString b = m_engineB->currentText();
	t->setName(tr("SPRT %1 vs %2 (%3)").arg(a, b, m_tcLabel));
	t->setVariant(s_variant);

	const QString pgn = pgnFile();
	QDir().mkpath(QFileInfo(pgn).absolutePath());
	t->setPgnOutput(pgn);
	// No board tabs hold the games, so the tournament frees them
	t->setPgnCleanupEnabled(true);
	// Games cut short when SPRT decides are left out of the PGN
	t->setPgnWriteUnfinishedGames(false);

	t->setGamesPerEncounter(2);
	t->setRoundMultiplier(qMax(1, m_maxGames->value() / 2));
	t->setSwapSides(true);
	t->setOpeningRepetitions(2);

	const QString openings = QDir::fromNativeSeparators(m_openingsEdit->text().trimmed());
	if (!openings.isEmpty())
	{
		auto format = openings.endsWith(".epd", Qt::CaseInsensitive)
			? OpeningSuite::EpdFormat : OpeningSuite::PgnFormat;
		auto suite = new OpeningSuite(openings, format, OpeningSuite::RandomOrder, 0);
		if (suite->initialize())
			t->setOpeningSuite(suite);
		else
			delete suite;
	}

	t->sprt()->initialize(m_elo0->value(), m_elo1->value(),
			      m_alpha->value(), m_beta->value());

	for (const QString& name : {a, b})
	{
		const auto engines = m_engineManager->engines();
		for (const auto& e : engines)
		{
			if (e.name() == name)
			{
				EngineConfiguration config(e);
				config.setOption("Threads", m_threads->value());
				t->addPlayer(new EngineBuilder(config), m_tc);
				break;
			}
		}
	}

	return t;
}
