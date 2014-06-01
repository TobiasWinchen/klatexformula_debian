/***************************************************************************
 *   file klfapp.cpp
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
/* $Id$ */

#include <QSessionManager>
#include <QMessageBox>

#include "klfapp.h"


KLFGuiApplication::KLFGuiApplication(int& argc, char **argv)
  : QApplication(argc, argv)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
}

KLFGuiApplication::~KLFGuiApplication()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
}

void KLFGuiApplication::saveState(QSessionManager& sm)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  /** \todo .... */
}

void KLFGuiApplication::commitData(QSessionManager& sm)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  /** \todo .... */

  if (sm.allowsInteraction()) {
    klfDbg("interaction allowed.") ;
    //    QMessageBox::information(NULL, "info", "Shutting down.") ;
    sm.release();
  } else {
    klfDbg("interaction NOT allowed.") ;
  }
  // just don't cancel the app exit as the previous version did...
}
