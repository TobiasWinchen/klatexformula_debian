/***************************************************************************
 *   file plugins/buffers/buffers.cpp
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2011 by Philippe Faist
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
/* $Id$ */

#include <QtCore>
#include <QtGui>


#include "buffers.h"

// --------------------------------------------------------------------------------


void BuffersPlugin::initialize(QApplication *app, KLFMainWin *mainWin, KLFPluginConfigAccess *rwconfig)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("Initializing Buffers plugin (compiled for KLF version " KLF_VERSION_STRING ")");

  Q_INIT_RESOURCE(buffersdata);
  klfDbg("initialized buffers resource data.") ;

  _mainwin = mainWin;
  _app = app;

  _config = rwconfig;

  // add what's new text
  _mainwin->addWhatsNewText(
      tr("<p>The <b>buffers</b> plug-in allows you to work on different equations at once, save "
         "them to files, and switch between them.</p>")
      );

  klfDbg("About to create buffers widget and show.") ;

  _widget = new OpenBuffersWidget(mainWin);
  // will show automatically once there's 1 open buffer
  //  _widget->show();
}

QWidget * BuffersPlugin::createConfigWidget(QWidget *parent)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  QWidget *w = new QWidget(parent);
  return w;
}

void BuffersPlugin::loadFromConfig(QWidget *confwidget, KLFPluginConfigAccess *config)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
}
void BuffersPlugin::saveToConfig(QWidget *confwidget, KLFPluginConfigAccess *config)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
}






// Export Plugin
Q_EXPORT_PLUGIN2(buffers, BuffersPlugin);


