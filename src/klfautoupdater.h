/***************************************************************************
 *   file klfautoupdater.h
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
/* $Id: klfautoupdater.h 909 2014-08-10 17:58:33Z phfaist $ */

#ifndef KLFAUTOUPDATER_H
#define KLFAUTOUPDATER_H

#include <QObject>


/** Minimal interface for an auto-updater.
 *
 * This removes the necessity of having too much sparkle-specific code in main.cpp or
 * klfmainwin.cpp etc.
 */
class KLFAutoUpdater : public QObject
{
  Q_OBJECT
public:
  KLFAutoUpdater(QObject * parent = NULL) : QObject(parent)  { }
  virtual ~KLFAutoUpdater() { }

public slots:
  virtual void checkForUpdates(bool inBackground = false) { Q_UNUSED(inBackground); }

};



#endif
