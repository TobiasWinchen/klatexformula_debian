/***************************************************************************
 *   file klfuiloader.cpp
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
/* $Id: klfuiloader.cpp 892 2014-07-27 16:01:10Z phfaist $ */

#include "klfuiloader.h"
#include "klfuiloader_p.h"

#include "klfconfig.h"
#include "klfsearchbar.h"
#include "klfcolorchooser.h"
#include "klflatexedit.h"
#include "klfenumlistwidget.h"
#include "klfsidewidget.h"
#include "klfpathchooser.h"

KLFUiLoader::~KLFUiLoader()
{
}

QWidget * KLFUiLoader::createWidget(const QString& className, QWidget * parent,
				    const QString & name)
{
  QWidget * w = NULL;

  if (className == QLatin1String("KLFSearchBar")) {
    w = new KLFSearchBar(parent);
  } else if (className == QLatin1String("KLFColorChooseWidgetPane")) {
    w = new KLFColorChooseWidgetPane(parent);
  } else if (className == QLatin1String("KLFColorClickSquare")) {
    w = new KLFColorClickSquare(parent);
  } else if (className == QLatin1String("KLFColorChooseWidget")) {
    w = new KLFColorChooseWidget(parent);
  } else if (className == QLatin1String("KLFColorChooser")) {
    w = new KLFColorChooser(parent);
  } else if (className == QLatin1String("KLFLatexEdit")) {
    w = new KLFLatexEdit(parent);
    w->setFont(klfconfig.UI.preambleEditFont);
  } else if (className == QLatin1String("KLFEnumListWidget")) {
    w = new KLFEnumListWidget(parent);
  } else if (className == QLatin1String("KLFSideWidget")) {
    w = new KLFSideWidget(parent);
  } else if (className == QLatin1String("KLFPathChooser")) {
    w = new KLFPathChooser(parent);
  }

  if (w != NULL) {
    klfDbg("created a custom "<<className<<" (name "<<name<<")") ;
    w->setObjectName(name);
    return w;
  }

  klfDbg("using default QUiLoader::createWidget("<<className<<", "<<parent<<", "<<name<<")") ;
  return QUiLoader::createWidget(className, parent, name);
}



QWidget * klfLoadUI(QIODevice *iodevice, QWidget * parent)
{
  KLFUiLoader loader;
  QWidget *widget = loader.load(iodevice, parent);
  KLF_ASSERT_NOT_NULL(widget, "Unable to load UI widget form!", return NULL; ) ;
  return widget;
}
