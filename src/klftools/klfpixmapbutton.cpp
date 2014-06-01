/***************************************************************************
 *   file klfpixmapbutton.cpp
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
/* $Id: klfpixmapbutton.cpp 604 2011-02-27 23:34:37Z phfaist $ */

#include <QApplication>
#include <QPushButton>
#include <QStyleOption>
#include <QPainter>
#include <QPixmap>
#include <QStyle>
#include <QPaintEvent>

#include <klfdefs.h>
#include "klfpixmapbutton.h"



KLFPixmapButton::KLFPixmapButton(const QPixmap& pix, QWidget *parent)
  : QPushButton(parent), _pix(pix), _pixmargin(2), _xalignfactor(0.5f), _yalignfactor(0.5f)
{
  setText(QString());
  setIcon(QIcon());
}

QSize KLFPixmapButton::minimumSizeHint() const
{
  return sizeHint();
}

QSize KLFPixmapButton::sizeHint() const
{
  // inspired by QPushButton::sizeHint() in qpushbutton.cpp

  ensurePolished();

  int w = 0, h = 0;
  QStyleOptionButton opt;
  initStyleOption(&opt);

  // calculate contents size...
  w = _pix.width() + _pixmargin;
  h = _pix.height() + _pixmargin;

  if (menu())
    w += style()->pixelMetric(QStyle::PM_MenuButtonIndicator, &opt, this);

  return (style()->sizeFromContents(QStyle::CT_PushButton, &opt, QSize(w, h), this).
	  expandedTo(QApplication::globalStrut()).expandedTo(QSize(50, 30)));
  // (50,30) is minimum non-square buttons on Qt/Mac
}

void KLFPixmapButton::paintEvent(QPaintEvent *event)
{
  QPushButton::paintEvent(event);
  QPainter p(this);
  p.setClipRect(event->rect());
  p.drawPixmap(QPointF( _xalignfactor*(width()-(2*_pixmargin+_pix.width())) + _pixmargin,
			_yalignfactor*(height()-(2*_pixmargin+_pix.height())) + _pixmargin ),
	       _pix);
}



