/***************************************************************************
 *   file 
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2011 by Philippe Faist
 *   philippe.faist@bluewin.ch
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
/* $Id: klfsettings.h 819 2012-08-10 21:12:14Z phfaist $ */

#ifndef KLFSETTINGS_H
#define KLFSETTINGS_H

#include <QDialog>
#include <QTextCharFormat>
#include <QCheckBox>
#include <QPushButton>

#include <klfbackend.h>
#include <klfconfig.h>

class QTreeWidgetItem;
class KLFColorChooser;
class KLFPathChooser;
class KLFLatexSyntaxHighlighter;
class KLFMainWin;

namespace Ui { class KLFSettings; }

class KLFSettingsPrivate;

/** \brief A settings dialog
 *
 * See also \ref KLFConfig and \ref klfconfig.
 */
class KLF_EXPORT KLFSettings : public QDialog
{
  Q_OBJECT

public:
  enum SettingsControl {
    AppLanguage = 1,
    AppFonts,
    AppLookNFeel,
    AppMacOSXMetalLook,
    Preview,
    TooltipPreview,
    SyntaxHighlighting,
    SyntaxHighlightingColors,
    ExecutablePaths,
    ExpandEPSBBox,
    CalcEPSBoundingBox,
    ExportProfiles,
    LibrarySettings,
    UserScriptInfo,
    ManageAddOns,
    ManagePlugins,
    PluginsConfig
  };

  KLFSettings(KLFMainWin* parent = 0);
  ~KLFSettings();

  bool eventFilter(QObject *object, QEvent *event);

signals:
  void settingsApplied();

public slots:

  void reset();
  void show();

  /** \c controlNum is one of the values of the \ref SettingsControl enum. */
  void showControl(int controlNum);
  /** \c controlName is the name (string) of one of the controls listed in the
   * \c SettingsControl enum, eg. \c "AppFonts" or \c "ManageAddOns". */
  void showControl(const QString& controlName);

  void apply();

  void help();

  void setDefaultPaths();
  void importAddOn();
  void importAddOn(const QString& fileName, bool uiSuggestRestart = true);
  void removeAddOn();
  void removePlugin();
  /** \warning This method provides NO USER CONFIRMATION and NO AFTER-OPERATION REFRESH */
  void removePlugin(const QString& fname);

  void showAdvancedConfigEditor();
  void showSystemMessages();

  void retranslateUi(bool alsoBaseUi = true);

protected:

protected slots:
  virtual void accept();

private:
  Ui::KLFSettings *u;

  KLF_DECLARE_PRIVATE(KLFSettings);

};

#endif

