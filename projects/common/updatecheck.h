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

#ifndef CUTEMAKRUK_UPDATECHECK_H
#define CUTEMAKRUK_UPDATECHECK_H

#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStringList>
#include <QTimer>
#include <QUrl>

/*!
 * Asks GitHub for the latest Cute Makruk release and compares it with
 * this build's version. Only the public releases API is contacted;
 * nothing about the user is sent.
 */
namespace UpdateCheck {

const char* const RepoUrl = "https://github.com/kiraro33/cutemakruk";
const char* const ApiUrl =
	"https://api.github.com/repos/kiraro33/cutemakruk/releases/latest";

struct Result
{
	bool ok = false;          //!< the check itself worked
	bool newer = false;       //!< a newer release exists
	QString current;          //!< this build's version
	QString latest;           //!< latest release version (without "v")
	QString url;              //!< release page
	QString error;            //!< why the check failed
};

//! Compares dotted versions ("1.2.10" > "1.2.9"); a leading "v" is ignored.
inline int compareVersions(QString a, QString b)
{
	if (a.startsWith('v') || a.startsWith('V'))
		a.remove(0, 1);
	if (b.startsWith('v') || b.startsWith('V'))
		b.remove(0, 1);
	const QStringList pa = a.split('.'), pb = b.split('.');
	for (int i = 0; i < qMax(pa.size(), pb.size()); i++)
	{
		int x = i < pa.size() ? pa.at(i).toInt() : 0;
		int y = i < pb.size() ? pb.at(i).toInt() : 0;
		if (x != y)
			return x < y ? -1 : 1;
	}
	return 0;
}

//! Checks for a newer release (blocks up to \a timeoutMs).
inline Result check(const QString& currentVersion, int timeoutMs = 15000)
{
	Result res;
	res.current = currentVersion;
	res.url = QString(RepoUrl) + "/releases";

	QNetworkAccessManager manager;
	QNetworkRequest request{QUrl(ApiUrl)};
	request.setHeader(QNetworkRequest::UserAgentHeader,
			  QString("cutemakruk/%1").arg(currentVersion));
	request.setRawHeader("Accept", "application/vnd.github+json");
	request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
			     QNetworkRequest::NoLessSafeRedirectPolicy);

	QNetworkReply* reply = manager.get(request);
	QEventLoop loop;
	QTimer timer;
	timer.setSingleShot(true);
	QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
	QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
	timer.start(timeoutMs);
	loop.exec();

	if (!reply->isFinished())
	{
		reply->abort();
		res.error = "no answer from GitHub (timeout)";
	}
	else if (reply->error() == QNetworkReply::ContentNotFoundError)
		res.error = "no release published yet";
	else if (reply->error() != QNetworkReply::NoError)
		res.error = reply->errorString();
	else
	{
		const QJsonObject obj =
			QJsonDocument::fromJson(reply->readAll()).object();
		QString tag = obj.value("tag_name").toString();
		if (tag.isEmpty())
			res.error = "unexpected answer from GitHub";
		else
		{
			res.ok = true;
			res.latest = tag.startsWith('v') ? tag.mid(1) : tag;
			res.newer = compareVersions(res.latest, currentVersion) > 0;
			QString page = obj.value("html_url").toString();
			if (!page.isEmpty())
				res.url = page;
		}
	}
	reply->deleteLater();
	return res;
}

} // namespace UpdateCheck

#endif // CUTEMAKRUK_UPDATECHECK_H
