/***************************************************************************
 *   file klfkateplugin_config.h
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
/* $Id: klfkateplugin_config.h 902 2014-08-09 23:44:15Z phfaist $ */

#ifndef KLFKATEPLUGIN_CONFIG_H
#define KLFKATEPLUGIN_CONFIG_H

#include <kcmodule.h>

namespace Ui { class KLFKatePluginConfigWidget; }


class KLFKteConfigData;


class KLFKteConfig  :  public KCModule
{
  Q_OBJECT

public:
  explicit KLFKteConfig(QWidget *parent = 0, const QVariantList &args = QVariantList());
  virtual ~KLFKteConfig();
  
  virtual void save();
  virtual void saveViaConfigData(KLFKteConfigData * d);
  virtual void load();
  virtual void defaults();
  
private Q_SLOTS:
  void slotChanged();
  
  void slotMaxSize(int step);

private:
  Ui::KLFKatePluginConfigWidget *u;
};

#endif // KLFKATEPLUGIN_CONFIG_H

