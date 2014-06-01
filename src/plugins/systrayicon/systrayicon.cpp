/***************************************************************************
 *   file plugins/systrayicon/systrayicon.cpp
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
/* $Id: systrayicon.cpp 761 2012-03-12 10:19:16Z phfaist $ */

#include <QtCore>
#include <QtGui>

#include <klfguiutil.h>
#include <klfmainwin.h>
//#include <klflibrary.h>
#include <klflatexsymbols.h>
#include <klfconfig.h>

#include "systrayicon.h"


SysTrayMainIconifyButtons::SysTrayMainIconifyButtons(QWidget *parent) : QWidget(parent)
{
  setupUi(this);
  btnQuit->setShortcut(QKeySequence(tr("Ctrl+Q", "SysTrayMainIconifyButtons->Quit")));

#ifdef Q_WS_MAC
  btnIconify->setText(tr("Hide", "[[iconify button text on mac]]"));
#endif
}


SysTrayIconConfigWidget::SysTrayIconConfigWidget(QWidget *parent)
  : QWidget(parent)
{
  setupUi(this);

#ifdef KLF_MAC_HIDE_INSTEAD
  chkSysTrayOn->setText(tr("Offer to hide application", "[[systrayicon config setting on Mac OS X]]"));
  chkRestoreOnHover->hide();
  chkReplaceQuitButton->hide();
  chkMinToSysTray->hide();
#endif

#if !defined(Q_WS_X11)
  chkRestoreOnHover->hide();
#endif
}

void SysTrayIconPlugin::initialize(QApplication */*app*/, KLFMainWin *mainWin,
				   KLFPluginConfigAccess *rwconfig)
{
  _mainButtonBar = NULL;

  _mainwin = mainWin;
  _config = rwconfig;

  // set default settings
#ifdef Q_WS_MAC
  // Mac: offer to hide application by default
  _config->makeDefaultValue("systrayon", true);
#else
  // Win/Linux: disable sys tray icon by default
  _config->makeDefaultValue("systrayon", false);
#endif
  _config->makeDefaultValue("replacequitbutton", true);
#ifdef Q_WS_X11
  _config->makeDefaultValue("restoreonhover", true);
#endif
#ifndef KLF_MAC_HIDE_INSTEAD
  _config->makeDefaultValue("mintosystray", true);
#else
  _config->makeDefaultValue("mintosystray", false);
#endif

  QMenu *menu = new QMenu(mainWin);
  menu->addAction(tr("Minimize"), this, SLOT(minimize()));
  menu->addAction(tr("Restore"), this, SLOT(restore()));
  menu->addAction(QIcon(":/pics/paste.png"), tr("Paste From Clipboard"),
		  this, SLOT(latexFromClipboard()));
#ifdef Q_WS_X11
  menu->addAction(QIcon(":/pics/paste.png"), tr("Paste From Mouse Selection"),
		  this, SLOT(latexFromClipboardSelection()));
#endif
  menu->addAction(QIcon(":/pics/closehide.png"), tr("Quit"), mainWin, SLOT(quit()));

#ifndef KLF_MAC_HIDE_INSTEAD
  _systrayicon = new QSystemTrayIcon(QIcon(":/pics/klatexformula-32.png"), mainWin);
  _systrayicon->setToolTip("KLatexFormula");
  _systrayicon->setContextMenu(menu);
  _systrayicon->installEventFilter(this);
  connect(_systrayicon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
	  this, SLOT(slotSysTrayActivated(QSystemTrayIcon::ActivationReason)));
#else
  _systrayicon = NULL;
#endif

  _mainwin->installEventFilter(this);

  connect(_mainwin, SIGNAL(userActivity()), this, SLOT(restore()));

  apply();
}

void SysTrayIconPlugin::apply()
{
  bool systrayon = _config->readValue("systrayon").toBool();

  if (_systrayicon != NULL) {
    if (systrayon && QSystemTrayIcon::isSystemTrayAvailable()) {
      _systrayicon->show();
      _mainwin->setQuitOnClose(false);
    } else {
      _systrayicon->hide();
      _mainwin->setQuitOnClose(true);
    }
  }

  // replace main window's quit button by a close button or restore quit button if option was unchecked.

  QFrame *mainFrmMain = _mainwin->findChild<QFrame*>("frmMain");
  QPushButton *mainBtnQuit = _mainwin->findChild<QPushButton*>("btnQuit");

  if ( systrayon && _mainButtonBar == NULL ) {
    _mainButtonBar = new SysTrayMainIconifyButtons(mainFrmMain);
    mainFrmMain->layout()->addWidget(_mainButtonBar);
    connect(_mainButtonBar->btnIconify, SIGNAL(clicked()), this, SLOT(minimize()));
    connect(_mainButtonBar->btnQuit, SIGNAL(clicked()), _mainwin, SLOT(quit()));
  }

  bool wantreplacequitbutton;
#ifdef KLF_MAC_HIDE_INSTEAD
  // the user should disable "Dock into systray" altogether if he doesn't want this
  // as the only way he would have to hide the app is through the mac menu anyway
  wantreplacequitbutton = true;
#else
  wantreplacequitbutton = _config->readValue("replacequitbutton").toBool();
#endif
    
  if ( systrayon && wantreplacequitbutton ) {
    mainBtnQuit->hide();
    _mainButtonBar->show();
  } else if (_mainButtonBar != NULL) {
    _mainButtonBar->hide();
    mainBtnQuit->show();
  }

}

bool SysTrayIconPlugin::eventFilter(QObject *obj, QEvent *e)
{
  if (obj == _systrayicon && e->type() == QEvent::ToolTip) {
    // works on X11 only
    if (_config->readValue("restoreonhover").toBool()) {
      restore();
    }
  }

  //#ifndef KLF_MAC_HIDE_INSTEAD ... well yes do this too, except by default disable this option
  if (obj == _mainwin && e->type() == QEvent::WindowStateChange) {
    if ( _mainwin->property("x11WindowShaded").toBool() )
      return false; // don't take action if the window is just shaded

    klfDbg("main win window state changed: oldState="<<((QWindowStateChangeEvent*)e)->oldState()
	   <<" new state="<<_mainwin->windowState()) ;
    if ( _config->readValue("systrayon").toBool() &&
	 _config->readValue("mintosystray").toBool() &&
	(_mainwin->windowState() & Qt::WindowMinimized) &&
	!(((QWindowStateChangeEvent*)e)->oldState() & Qt::WindowMinimized)
	) {
      // the user minimized the window, and we want to minimize it to system tray.
      QTimer::singleShot(20, this, SLOT(minimize()));
    }
  }
  //#endif // KLF_MAC_HIDE_INSTEAD

  return false;
}


QWidget * SysTrayIconPlugin::createConfigWidget(QWidget *parent)
{
  return new SysTrayIconConfigWidget(parent);
}

void SysTrayIconPlugin::loadFromConfig(QWidget *confqwidget, KLFPluginConfigAccess *config)
{
  SysTrayIconConfigWidget *confwidget = qobject_cast<SysTrayIconConfigWidget*>(confqwidget);
  bool systrayon = config->readValue("systrayon").toBool();
  bool restoreonhover = config->readValue("restoreonhover").toBool();
  bool replacequitbutton = config->readValue("replacequitbutton").toBool();
  bool mintosystray = config->readValue("mintosystray").toBool();
  confwidget->chkSysTrayOn->setChecked(systrayon);
  confwidget->chkRestoreOnHover->setChecked(restoreonhover);
  confwidget->chkReplaceQuitButton->setChecked(replacequitbutton);
  confwidget->chkMinToSysTray->setChecked(mintosystray);
}
void SysTrayIconPlugin::saveToConfig(QWidget *confqwidget, KLFPluginConfigAccess *config)
{
  SysTrayIconConfigWidget *confwidget = qobject_cast<SysTrayIconConfigWidget*>(confqwidget);
  bool systrayon = confwidget->chkSysTrayOn->isChecked();
  bool restoreonhover = confwidget->chkRestoreOnHover->isChecked();
  bool replacequitbutton = confwidget->chkReplaceQuitButton->isChecked();
  bool mintosystray = confwidget->chkMinToSysTray->isChecked();
  config->writeValue("systrayon", systrayon);
  config->writeValue("restoreonhover", restoreonhover);
  config->writeValue("replacequitbutton", replacequitbutton);
  config->writeValue("mintosystray", mintosystray);
  apply();
}


void SysTrayIconPlugin::latexFromClipboard(QClipboard::Mode mode)
{
  restore();
  _mainwin->pasteLatexFromClipboard(mode);
}

bool SysTrayIconPlugin::isMinimized()
{
  return !_mainwin->isApplicationVisible();
}

void SysTrayIconPlugin::restore()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  if (!isMinimized() || !_config->readValue("systrayon").toBool())
    return;

  klfDbg("debug... restoring.") ;
  klfRestoreWindows();
  _mainwin->raise();
  _mainwin->activateWindow();
  qApp->alert(_mainwin);
}

void SysTrayIconPlugin::minimize()
{
#ifdef KLF_MAC_HIDE_INSTEAD
  klfDbg("Hiding application...") ;
  _mainwin->hideApplication();
#else
  klfDbg("Hiding windows...") ;
  klfHideWindows();
  //  _mainwin->close();
#endif
}

void SysTrayIconPlugin::slotSysTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
#ifdef Q_WS_MAC
  // on mac simple clic gives the menu; don't automatically activate window.
  if (reason == QSystemTrayIcon::Trigger)
    return;
#endif

  if (reason == QSystemTrayIcon::Trigger) {
    if ( ! _mainwin->isVisible() ) {
      restore();
    } else {
      if ( QApplication::activeWindow() != NULL ) {
	minimize();
      } else {
	restore();
      }
    }
  }
  if (reason == QSystemTrayIcon::DoubleClick) {
    latexFromClipboard();
  }
  if (reason == QSystemTrayIcon::MiddleClick) {
    // paste mouse selection
    latexFromClipboard(QClipboard::Selection);
  }
}





// Export Plugin
Q_EXPORT_PLUGIN2(systrayicon, SysTrayIconPlugin);
