/***************************************************************************
 *   file klfsearchbar_desplugin.cpp
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2010 by Philippe Faist
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
/* $Id: klftools_desplugin.cpp 801 2012-07-15 13:13:44Z phfaist $ */

#include <stdio.h>

#include "klftools_desplugin.h"

/** \bug THIS FILE IS ACTUALLY NEEDED in the packaged version for QUiLoader to be able to access
 *    these widgets when creating user scripts' input widgets */


KLFSearchBarDesPlugin::KLFSearchBarDesPlugin(QObject *parent)
{
  Q_UNUSED(parent) ;
}

bool KLFSearchBarDesPlugin::isContainer() const
{
  return false;
}
bool KLFSearchBarDesPlugin::isInitialized() const
{
  return pInitialized;
}
QIcon KLFSearchBarDesPlugin::icon() const
{
  return QIcon();
}
QString KLFSearchBarDesPlugin::domXml() const
{
  return "<widget class=\"KLFSearchBar\" name=\"searchBar\"></widget>\n";
}
QString KLFSearchBarDesPlugin::group() const
{
  return "Utilities";
}
QString KLFSearchBarDesPlugin::includeFile() const
{
  return "klfsearchbar.h";
}
QString KLFSearchBarDesPlugin::name() const
{
  return "KLFSearchBar";
}
QString KLFSearchBarDesPlugin::toolTip() const
{
  return "A full-featured search bar";
}
QString KLFSearchBarDesPlugin::whatsThis() const
{
  return "A full-featured search bar";
}
QWidget *KLFSearchBarDesPlugin::createWidget(QWidget *parent)
{
  KLFSearchBar *searchbar = new KLFSearchBar(parent);
  searchbar->_isInQtDesigner = true;
  return searchbar;
}
void KLFSearchBarDesPlugin::initialize(QDesignerFormEditorInterface *core)
{
  Q_UNUSED(core) ;
  if (pInitialized)
    return;

  // initialize ... :)

  pInitialized = true;
}


// ---------------------

KLFColorChooseWidgetPaneDesPlugin::KLFColorChooseWidgetPaneDesPlugin(QObject *parent)
{
  Q_UNUSED(parent) ;
}

bool KLFColorChooseWidgetPaneDesPlugin::isContainer() const
{
  return false;
}
bool KLFColorChooseWidgetPaneDesPlugin::isInitialized() const
{
  return pInitialized;
}
QIcon KLFColorChooseWidgetPaneDesPlugin::icon() const
{
  return QIcon();
}
QString KLFColorChooseWidgetPaneDesPlugin::domXml() const
{
  return "<widget class=\"KLFColorChooseWidgetPane\" name=\"colorChooseWidgetPane\"></widget>\n";
}
QString KLFColorChooseWidgetPaneDesPlugin::group() const
{
  return "Utilities";
}
QString KLFColorChooseWidgetPaneDesPlugin::includeFile() const
{
  return "klfcolorchooser.h";
}
QString KLFColorChooseWidgetPaneDesPlugin::name() const
{
  return "KLFColorChooseWidgetPane";
}
QString KLFColorChooseWidgetPaneDesPlugin::toolTip() const
{
  return "A color chooser pane";
}
QString KLFColorChooseWidgetPaneDesPlugin::whatsThis() const
{
  return "A 2-component color chooser pane";
}
QWidget *KLFColorChooseWidgetPaneDesPlugin::createWidget(QWidget *parent)
{
  KLFColorChooseWidgetPane *w = new KLFColorChooseWidgetPane(parent);
  return w;
}
void KLFColorChooseWidgetPaneDesPlugin::initialize(QDesignerFormEditorInterface *core)
{
  Q_UNUSED(core) ;
  if (pInitialized)
    return;

  // initialize ... :)

  pInitialized = true;
}


// --------------------




KLFColorComponentSpinBoxDesPlugin::KLFColorComponentSpinBoxDesPlugin(QObject *parent)
{
  Q_UNUSED(parent) ;
}

bool KLFColorComponentSpinBoxDesPlugin::isContainer() const
{
  return false;
}
bool KLFColorComponentSpinBoxDesPlugin::isInitialized() const
{
  return pInitialized;
}
QIcon KLFColorComponentSpinBoxDesPlugin::icon() const
{
  return QIcon();
}
QString KLFColorComponentSpinBoxDesPlugin::domXml() const
{
  return "<widget class=\"KLFColorComponentSpinBox\" name=\"colorComponentSpinBox\"></widget>\n";
}
QString KLFColorComponentSpinBoxDesPlugin::group() const
{
  return "Utilities";
}
QString KLFColorComponentSpinBoxDesPlugin::includeFile() const
{
  return "klfcolorchooser.h";
}
QString KLFColorComponentSpinBoxDesPlugin::name() const
{
  return "KLFColorComponentSpinBox";
}
QString KLFColorComponentSpinBoxDesPlugin::toolTip() const
{
  return "A spinbox to edit a color component";
}
QString KLFColorComponentSpinBoxDesPlugin::whatsThis() const
{
  return "A spinbox to edit a color component";
}
QWidget *KLFColorComponentSpinBoxDesPlugin::createWidget(QWidget *parent)
{
  KLFColorComponentSpinBox *w = new KLFColorComponentSpinBox(parent);
  return w;
}
void KLFColorComponentSpinBoxDesPlugin::initialize(QDesignerFormEditorInterface *core)
{
  Q_UNUSED(core) ;
  if (pInitialized)
    return;

  // initialize ... :)

  pInitialized = true;
}



// --------------------




KLFColorClickSquareDesPlugin::KLFColorClickSquareDesPlugin(QObject *parent)
{
  Q_UNUSED(parent) ;
}

bool KLFColorClickSquareDesPlugin::isContainer() const
{
  return false;
}
bool KLFColorClickSquareDesPlugin::isInitialized() const
{
  return pInitialized;
}
QIcon KLFColorClickSquareDesPlugin::icon() const
{
  return QIcon();
}
QString KLFColorClickSquareDesPlugin::domXml() const
{
  return "<widget class=\"KLFColorClickSquare\" name=\"colorClickSquare\"></widget>\n";
}
QString KLFColorClickSquareDesPlugin::group() const
{
  return "Utilities";
}
QString KLFColorClickSquareDesPlugin::includeFile() const
{
  return "klfcolorchooser.h";
}
QString KLFColorClickSquareDesPlugin::name() const
{
  return "KLFColorClickSquare";
}
QString KLFColorClickSquareDesPlugin::toolTip() const
{
  return "A spinbox to edit a color component";
}
QString KLFColorClickSquareDesPlugin::whatsThis() const
{
  return "A spinbox to edit a color component";
}
QWidget *KLFColorClickSquareDesPlugin::createWidget(QWidget *parent)
{
  KLFColorClickSquare * w = new KLFColorClickSquare(parent);
  return w;
}
void KLFColorClickSquareDesPlugin::initialize(QDesignerFormEditorInterface *core)
{
  Q_UNUSED(core) ;
  if (pInitialized)
    return;

  // initialize ... :)

  pInitialized = true;
}



// -----------


KLFColorChooseWidgetDesPlugin::KLFColorChooseWidgetDesPlugin(QObject *parent)
{
  Q_UNUSED(parent) ;
}

bool KLFColorChooseWidgetDesPlugin::isContainer() const
{
  return false;
}
bool KLFColorChooseWidgetDesPlugin::isInitialized() const
{
  return pInitialized;
}
QIcon KLFColorChooseWidgetDesPlugin::icon() const
{
  return QIcon();
}
QString KLFColorChooseWidgetDesPlugin::domXml() const
{
  return "<widget class=\"KLFColorChooseWidget\" name=\"colorChooseWidget\"></widget>\n";
}
QString KLFColorChooseWidgetDesPlugin::group() const
{
  return "Utilities";
}
QString KLFColorChooseWidgetDesPlugin::includeFile() const
{
  return "klfcolorchooser.h";
}
QString KLFColorChooseWidgetDesPlugin::name() const
{
  return "KLFColorChooseWidget";
}
QString KLFColorChooseWidgetDesPlugin::toolTip() const
{
  return "A spinbox to edit a color component";
}
QString KLFColorChooseWidgetDesPlugin::whatsThis() const
{
  return "A spinbox to edit a color component";
}
QWidget *KLFColorChooseWidgetDesPlugin::createWidget(QWidget *parent)
{
  KLFColorChooseWidget * w = new KLFColorChooseWidget(parent);
  return w;
}
void KLFColorChooseWidgetDesPlugin::initialize(QDesignerFormEditorInterface *core)
{
  Q_UNUSED(core) ;
  if (pInitialized)
    return;

  // initialize ... :)

  pInitialized = true;
}






// -----------


KLFColorChooserDesPlugin::KLFColorChooserDesPlugin(QObject *parent)
{
  Q_UNUSED(parent) ;
}

bool KLFColorChooserDesPlugin::isContainer() const
{
  return false;
}
bool KLFColorChooserDesPlugin::isInitialized() const
{
  return pInitialized;
}
QIcon KLFColorChooserDesPlugin::icon() const
{
  return QIcon();
}
QString KLFColorChooserDesPlugin::domXml() const
{
  return "<widget class=\"KLFColorChooser\" name=\"colorChooser\"></widget>\n";
}
QString KLFColorChooserDesPlugin::group() const
{
  return "Utilities";
}
QString KLFColorChooserDesPlugin::includeFile() const
{
  return "klfcolorchooser.h";
}
QString KLFColorChooserDesPlugin::name() const
{
  return "KLFColorChooser";
}
QString KLFColorChooserDesPlugin::toolTip() const
{
  return "A spinbox to edit a color component";
}
QString KLFColorChooserDesPlugin::whatsThis() const
{
  return "A spinbox to edit a color component";
}
QWidget *KLFColorChooserDesPlugin::createWidget(QWidget *parent)
{
  KLFColorChooser * w = new KLFColorChooser(parent);
  return w;
}
void KLFColorChooserDesPlugin::initialize(QDesignerFormEditorInterface *core)
{
  Q_UNUSED(core) ;
  if (pInitialized)
    return;

  // initialize ... :)

  pInitialized = true;
}





// -----------


KLFLatexEditDesPlugin::KLFLatexEditDesPlugin(QObject *parent)
{
  Q_UNUSED(parent) ;
}

bool KLFLatexEditDesPlugin::isContainer() const
{
  return false;
}
bool KLFLatexEditDesPlugin::isInitialized() const
{
  return pInitialized;
}
QIcon KLFLatexEditDesPlugin::icon() const
{
  return QIcon();
}
QString KLFLatexEditDesPlugin::domXml() const
{
  return "<widget class=\"KLFLatexEdit\" name=\"latexEdit\"></widget>\n";
}
QString KLFLatexEditDesPlugin::group() const
{
  return "Utilities";
}
QString KLFLatexEditDesPlugin::includeFile() const
{
  return "klflatexedit.h";
}
QString KLFLatexEditDesPlugin::name() const
{
  return "KLFLatexEdit";
}
QString KLFLatexEditDesPlugin::toolTip() const
{
  return "A latex code editor";
}
QString KLFLatexEditDesPlugin::whatsThis() const
{
  return "A simple latex code editor";
}
QWidget *KLFLatexEditDesPlugin::createWidget(QWidget *parent)
{
  KLFLatexEdit * w = new KLFLatexEdit(parent);
  return w;
}
void KLFLatexEditDesPlugin::initialize(QDesignerFormEditorInterface *core)
{
  Q_UNUSED(core) ;
  if (pInitialized)
    return;

  // initialize ... :)

  pInitialized = true;
}

// -------------------


KLFEnumListWidgetDesPlugin::KLFEnumListWidgetDesPlugin(QObject *parent)
{
  Q_UNUSED(parent) ;
}

bool KLFEnumListWidgetDesPlugin::isContainer() const
{
  return false;
}
bool KLFEnumListWidgetDesPlugin::isInitialized() const
{
  return pInitialized;
}
QIcon KLFEnumListWidgetDesPlugin::icon() const
{
  return QIcon();
}
QString KLFEnumListWidgetDesPlugin::domXml() const
{
  return "<widget class=\"KLFEnumListWidget\" name=\"enumListWidget\"></widget>\n";
}
QString KLFEnumListWidgetDesPlugin::group() const
{
  return "Utilities";
}
QString KLFEnumListWidgetDesPlugin::includeFile() const
{
  return "klfenumlistwidget.h";
}
QString KLFEnumListWidgetDesPlugin::name() const
{
  return "KLFEnumListWidget";
}
QString KLFEnumListWidgetDesPlugin::toolTip() const
{
  return "A list of strings displayed in a flow layout (e.g. tags)";
}
QString KLFEnumListWidgetDesPlugin::whatsThis() const
{
  return "A list of strings displayed in a flow layout (e.g. tags)";
}
QWidget *KLFEnumListWidgetDesPlugin::createWidget(QWidget *parent)
{
  KLFEnumListWidget * w = new KLFEnumListWidget(parent);
  return w;
}
void KLFEnumListWidgetDesPlugin::initialize(QDesignerFormEditorInterface *core)
{
  Q_UNUSED(core) ;
  if (pInitialized)
    return;

  // initialize ... :)

  pInitialized = true;
}



// -----


KLFSideWidgetDesPlugin::KLFSideWidgetDesPlugin(QObject *parent)
{
  Q_UNUSED(parent) ;
}

bool KLFSideWidgetDesPlugin::isContainer() const
{
  return true;
}
bool KLFSideWidgetDesPlugin::isInitialized() const
{
  return pInitialized;
}
QIcon KLFSideWidgetDesPlugin::icon() const
{
  return QIcon();
}
QString KLFSideWidgetDesPlugin::domXml() const
{
  // make sure sideWidgetManagerType is NON-translatable (so that it is not reset
  // at language change time !)
  return "<widget class=\"KLFSideWidget\" name=\"sideWidget\">\n"
    "  <property name=\"sideWidgetManagerType\">\n"
    "    <string notr=\"true\">ShowHide</string>\n"
    "  </property>\n"
    "</widget>" ;
}
QString KLFSideWidgetDesPlugin::group() const
{
  return "Utilities";
}
QString KLFSideWidgetDesPlugin::includeFile() const
{
  return "klfsidewidget.h";
}
QString KLFSideWidgetDesPlugin::name() const
{
  return "KLFSideWidget";
}
QString KLFSideWidgetDesPlugin::toolTip() const
{
  return "A widget that pops into the side of a window (in different variants)";
}
QString KLFSideWidgetDesPlugin::whatsThis() const
{
  return "A widget that pops into the side of a window (in different variants)";
}
QWidget *KLFSideWidgetDesPlugin::createWidget(QWidget *parent)
{
  KLFSideWidget * w = new KLFSideWidget(parent);
  w->_inqtdesigner = true;
  return w;
}
void KLFSideWidgetDesPlugin::initialize(QDesignerFormEditorInterface *core)
{
  Q_UNUSED(core) ;
  if (pInitialized)
    return;

  // initialize ... :)

  pInitialized = true;
}


// -----


KLFPathChooserDesPlugin::KLFPathChooserDesPlugin(QObject *parent)
{
  Q_UNUSED(parent) ;
}

bool KLFPathChooserDesPlugin::isContainer() const
{
  return false;
}
bool KLFPathChooserDesPlugin::isInitialized() const
{
  return pInitialized;
}
QIcon KLFPathChooserDesPlugin::icon() const
{
  return QIcon();
}
QString KLFPathChooserDesPlugin::domXml() const
{
  // make sure sideWidgetManagerType is NON-translatable (so that it is not reset
  // at language change time !)
  return "<widget class=\"KLFPathChooser\" name=\"pathChooser\">\n"
    "</widget>" ;
}
QString KLFPathChooserDesPlugin::group() const
{
  return "Utilities";
}
QString KLFPathChooserDesPlugin::includeFile() const
{
  return "klfpathchooser.h";
}
QString KLFPathChooserDesPlugin::name() const
{
  return "KLFPathChooser";
}
QString KLFPathChooserDesPlugin::toolTip() const
{
  return "A text field to input a path to a file or directory";
}
QString KLFPathChooserDesPlugin::whatsThis() const
{
  return "A text field to input a path to a file or directory";
}
QWidget *KLFPathChooserDesPlugin::createWidget(QWidget *parent)
{
  KLFPathChooser * w = new KLFPathChooser(parent);
  //  w->_inqtdesigner = true;
  return w;
}
void KLFPathChooserDesPlugin::initialize(QDesignerFormEditorInterface *core)
{
  Q_UNUSED(core) ;
  if (pInitialized)
    return;

  // initialize ... :)

  pInitialized = true;
}







// -----------------------------------------------------------

KLFToolsDesPlugin::KLFToolsDesPlugin(QObject* parent)
  : QObject(parent)
{
  _widgetPlugins.push_back(new KLFSearchBarDesPlugin(this));
  _widgetPlugins.push_back(new KLFColorChooseWidgetPaneDesPlugin(this));
  _widgetPlugins.push_back(new KLFColorComponentSpinBoxDesPlugin(this));
  _widgetPlugins.push_back(new KLFColorClickSquareDesPlugin(this));
  _widgetPlugins.push_back(new KLFColorChooseWidgetDesPlugin(this));
  _widgetPlugins.push_back(new KLFColorChooserDesPlugin(this));
  _widgetPlugins.push_back(new KLFLatexEditDesPlugin(this));
  _widgetPlugins.push_back(new KLFEnumListWidgetDesPlugin(this));
  _widgetPlugins.push_back(new KLFSideWidgetDesPlugin(this));
  _widgetPlugins.push_back(new KLFPathChooserDesPlugin(this));
}

KLFToolsDesPlugin::~KLFToolsDesPlugin()
{
  QList<QDesignerCustomWidgetInterface*>::iterator it;
  for (it = _widgetPlugins.begin(); it != _widgetPlugins.end(); it++) {
    delete (*it);
  }
  _widgetPlugins.clear();
}


// ----------------------------------------------------------



Q_EXPORT_PLUGIN2(klftoolsdesplugin, KLFToolsDesPlugin)
  ;


