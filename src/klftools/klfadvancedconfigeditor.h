/***************************************************************************
 *   file klfadvancedconfigeditor.h
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
/* $Id: klfadvancedconfigeditor.h 792 2012-06-09 20:42:41Z phfaist $ */

#ifndef KLFADVANCEDCONFIGEDITOR_H
#define KLFADVANCEDCONFIGEDITOR_H


#include <QDialog>

#include <klfcolorchooser.h>
#include <klfconfigbase.h>

class KLFAdvancedConfigEditorPrivate;

namespace Ui { class KLFAdvancedConfigEditor; };

class KLFAdvancedConfigEditor : public QDialog
{
  Q_OBJECT
public:
  KLFAdvancedConfigEditor(QWidget *parent, KLFConfigBase *c);
  virtual ~KLFAdvancedConfigEditor();

  virtual void setVisible(bool visible);

signals:
  void configModified(const QString& propertyName);

public slots:
  void updateConfig();

private:
  KLF_DECLARE_PRIVATE(KLFAdvancedConfigEditor) ;

  Ui::KLFAdvancedConfigEditor * u;
};




#endif
