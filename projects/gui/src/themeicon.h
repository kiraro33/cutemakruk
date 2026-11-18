/*
    This file is part of Cute Makruk.
    Copyright (C) 2026 Kiraro33 (O.T.)

    Cute Makruk is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Cute Makruk is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Cute Makruk.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef THEMEICON_H
#define THEMEICON_H

#include <QAbstractButton>
#include <QApplication>
#include <QFile>
#include <QStyleHints>
#include <QTimer>
#include <QIcon>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QSvgRenderer>

/*!
 * Returns a Lucide icon (res/icons/lucide, ISC licence) drawn in the
 * theme's text colour: white on a dark theme, black on a light one, and
 * faded when the button is disabled. \a onName is the icon for the "on"
 * state of a checkable button.
 */
inline QIcon themeIcon(const QString& name, const QString& onName = QString())
{
	const QColor color = QApplication::palette().color(QPalette::Active,
							     QPalette::ButtonText);
	QIcon icon;
	auto add = [&](const QString& file, QIcon::State state)
	{
		QFile f(":/lucide/" + file + ".svg");
		if (!f.open(QIODevice::ReadOnly))
			return;
		const QByteArray svg = f.readAll();
		const struct { QIcon::Mode mode; qreal opacity; } modes[] = {
			{QIcon::Normal, 1.0}, {QIcon::Active, 1.0},
			{QIcon::Selected, 1.0}, {QIcon::Disabled, 0.3}
		};
		for (const auto& m : modes)
		{
			QByteArray data = svg;
			data.replace("stroke=\"currentColor\"",
				     QString("stroke=\"%1\" stroke-opacity=\"%2\"")
				     .arg(color.name()).arg(m.opacity).toUtf8());
			QSvgRenderer renderer(data);
			for (int size : {16, 20, 24, 32, 48})
			{
				QPixmap pixmap(size, size);
				pixmap.fill(Qt::transparent);
				QPainter painter(&pixmap);
				renderer.render(&painter);
				painter.end();
				icon.addPixmap(pixmap, m.mode, state);
			}
		}
	};
	add(name, QIcon::Off);
	if (!onName.isEmpty())
		add(onName, QIcon::On);
	return icon;
}

/*!
 * Gives \a button a themeIcon() that is redrawn when the colour scheme
 * changes (Windows setting or View > Theme).
 */
inline void setThemeIcon(QAbstractButton* button, const QString& name,
			 const QString& onName = QString())
{
	button->setIcon(themeIcon(name, onName));
	QObject::connect(QGuiApplication::styleHints(),
			 &QStyleHints::colorSchemeChanged, button, [=]()
	{
		// the palette follows the scheme; draw after it has changed
		QTimer::singleShot(0, button, [=]()
		{
			button->setIcon(themeIcon(name, onName));
		});
	});
}

#endif // THEMEICON_H
