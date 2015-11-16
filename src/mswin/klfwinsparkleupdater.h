/***************************************************************************
 *   file klfwinsparkleupdater.h
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2014 by Philippe Faist
 *   philippe.faist at bluewin.ch
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
/* $Id: klfwinsparkleupdater.h 916 2014-08-24 22:17:20Z phfaist $ */

#ifndef KLFWINSPARKLEUPDATER_H
#define KLFWINSPARKLEUPDATER_H


#include <QObject>

#include <klfautoupdater.h>


class KLFWinSparkleUpdater : public KLFAutoUpdater
{
  Q_OBJECT
public:
  KLFWinSparkleUpdater(QObject * parent, const QString& feedurl);
  virtual ~KLFWinSparkleUpdater();

public slots:
  virtual void checkForUpdates(bool inBackground = false);

private:
  class Private;
  Private* d;
};





#endif
