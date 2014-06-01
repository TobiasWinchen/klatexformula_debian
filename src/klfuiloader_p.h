/***************************************************************************
 *   file klfuiloader_p.h
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2012 by Philippe Faist
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
/* $Id: klfuiloader_p.h 802 2012-07-15 14:20:30Z phfaist $ */

/** \file
 * This header contains (in principle private) auxiliary classes for
 * library routines defined in klfuiloader.cpp */

#ifndef KLFUILOADER_P_H
#define KLFUILOADER_P_H



#include <QWidget>
#include <QUiLoader>

#include <klfdefs.h>


class KLF_EXPORT KLFUiLoader : public QUiLoader
{
  Q_OBJECT
public:
  KLFUiLoader(QObject * parent = NULL)
    : QUiLoader(parent)
  {
  }
  virtual ~KLFUiLoader();

  virtual QWidget * createWidget(const QString& className, QWidget * parent = NULL,
				 const QString & name = QString());
};









#endif
