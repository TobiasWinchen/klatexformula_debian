/***************************************************************************
 *   file klfpobjeditwidget.h
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


#ifndef KLFPOBJEDITWIDGET_H
#define KLFPOBJEDITWIDGET_H

#include <klfdefs.h>

#include <QWidget>
#include <QTreeView>

class KLFAbstractPropertizedObject;
class KLFPObjEditWidgetPrivate;


class KLF_EXPORT KLFPObjEditWidget : public QTreeView
{
  Q_OBJECT
public:
  KLFPObjEditWidget(QWidget *parent = NULL);
  KLFPObjEditWidget(KLFAbstractPropertizedObject *pobj, QWidget *parent = NULL);
  virtual ~KLFPObjEditWidget();

public slots:
  virtual void setPObj(KLFAbstractPropertizedObject *pobj);

private:
  KLF_DECLARE_PRIVATE(KLFPObjEditWidget) ;
};



#endif
