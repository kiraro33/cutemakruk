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

#include "boardfactory.h"
#include "aiwokboard.h"
#include "almostboard.h"
#include "amazonboard.h"
#include "andernachboard.h"
#include "antiboard.h"
#include "aseanboard.h"
#include "atomicboard.h"
#include "berolinaboard.h"
#include "capablancaboard.h"
#include "caparandomboard.h"
#include "chancellorboard.h"
#include "checklessboard.h"
#include "chessgiboard.h"
#include "chigorinboard.h"
#include "codrusboard.h"
#include "connectboard.h"
#include "coregalboard.h"
#include "courierboard.h"
#include "crazyhouseboard.h"
#include "dobutsushogiboard.h"
#include "embassyboard.h"
#include "euroshogiboard.h"
#include "extinctionboard.h"
#include "frcboard.h"
#include "giveawayboard.h"
#include "gothicboard.h"
#include "grandboard.h"
#include "gridboard.h"
#include "gryphonboard.h"
#include "gustavboard.h"
#include "hoppelpoppelboard.h"
#include "hordeboard.h"
#include "janusboard.h"
#include "jesonmorboard.h"
#include "judkinsshogiboard.h"
#include "kingofthehillboard.h"
#include "knightmateboard.h"
#include "knightrelayboard.h"
#include "loopboard.h"
#include "losalamosboard.h"
#include "losersboard.h"
#include "ncheckboard.h"
#include "makrukboard.h"
#include "minishogiboard.h"
#include "modernboard.h"
#include "oukboard.h"
#include "placementboard.h"
#include "pocketknightboard.h"
#include "racingkingsboard.h"
#include "rifleboard.h"
#include "seirawanboard.h"
#include "shatranjboard.h"
#include "shogiboard.h"
#include "sittuyinboard.h"
#include "standardboard.h"
#include "suicideboard.h"
#include "threekingsboard.h"
#include "twokingseachboard.h"

namespace Chess {

REGISTER_BOARD(MakrukBoard, "makruk")


ClassRegistry<Board>* BoardFactory::registry()
{
	static ClassRegistry<Board>* registry = new ClassRegistry<Board>;
	return registry;
}

Board* BoardFactory::create(const QString& variant)
{
	return registry()->create(variant);
}

QStringList BoardFactory::variants()
{
	return registry()->items().keys();
}

} // namespace Chess
