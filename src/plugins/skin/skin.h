/***************************************************************************
 *   file plugins/skin/skin.h
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
/* $Id: skin.h 789 2012-06-01 07:09:57Z phfaist $ */

#ifndef PLUGINS_SKIN_H
#define PLUGINS_SKIN_H

#include <QtCore>
#include <QtGui>

#include <klfpluginiface.h>
#include <klfconfig.h>

#include <ui_skinconfigwidget.h>

struct SkinSyntaxHighlightingScheme
{
  QTextCharFormat fmtKeyword;
  QTextCharFormat fmtComment;
  QTextCharFormat fmtParenMatch;
  QTextCharFormat fmtParenMismatch;
  QTextCharFormat fmtLonelyParen;
};

struct Skin
{
  Skin() : overrideSHScheme(false) { }
  QString fn; //!< File Name (XML)
  
  QString name;
  QString author;
  QString description;

  QString stylesheet;

  bool overrideSHScheme;
  SkinSyntaxHighlightingScheme shscheme;
};

Q_DECLARE_METATYPE(Skin) ;


class SkinConfigWidget : public QWidget, public Ui::SkinConfigWidget
{
  Q_OBJECT
public:
  SkinConfigWidget(QWidget *skinconfigwidget, KLFPluginConfigAccess *conf);
  virtual ~SkinConfigWidget() { }

  /// the currently selected skin
  inline Skin currentSkin() { return cbxSkin->itemData(cbxSkin->currentIndex()).value<Skin>(); }
  /// the skin (XML) full filename. equivalent to currentSkin().fn
  inline QString currentSkinFn() { return currentSkin().fn; }

  /*
  static QString getStyleSheet(const QString& fn) {
    if (fn.isEmpty())
      return QString();

    QFile f(fn);
    if ( !f.open(QIODevice::ReadOnly) ) {
      qWarning()<<KLF_FUNC_NAME<<"Can't open file "<<fn;
      return QString();
    }
    QByteArray data = f.readAll();
    return QString::fromUtf8(data.constData(), data.size());
  }
  */

  static Skin loadSkin(KLFPluginConfigAccess *cfg, const QString& fn, bool getStyleSheet = true);

  bool getModifiedAndReset() { bool m = _modified; _modified = false; return m; }

public slots:
  void loadSkinList(QString skin);
  void skinSelected(int index);
  void refreshSkin();
  //  void installSkin();

  void updateSkinDescription(const Skin& skin);

  void on_chkNoSyntaxHighlightingChange_toggled(bool /*value*/)
  { _modified = true; }

private:
  KLFPluginConfigAccess *config;

  bool _modified;
};

class SkinPlugin : public QObject, public KLFPluginGenericInterface
{
  Q_OBJECT
  Q_INTERFACES(KLFPluginGenericInterface)
public:
  SkinPlugin() : QObject(qApp), _applyDelayed(false), _preventpleasewait(false) { }
  virtual ~SkinPlugin() { }

  virtual QVariant pluginInfo(PluginInfo which) const {
    switch (which) {
    case PluginName: return QString("skin");
    case PluginAuthor: return QString("Philippe Faist <philippe.faist")+QString("@bluewin.ch>");
    case PluginTitle: return tr("Skin");
    case PluginDescription: return tr("Personalize the look of KLatexFormula");
    case PluginDefaultEnable: return true;
    default:
      return QVariant();
    }
  }

  virtual void initialize(QApplication *app, KLFMainWin *mainWin, KLFPluginConfigAccess *config);

  virtual QWidget * createConfigWidget(QWidget *parent);
  virtual void loadFromConfig(QWidget *confwidget, KLFPluginConfigAccess *config);
  virtual void saveToConfig(QWidget *confwidget, KLFPluginConfigAccess *config);

  virtual Skin applySkin(KLFPluginConfigAccess *config, bool isStartup);

  virtual bool eventFilter(QObject *object, QEvent *event);

signals:
  void skinChanged(const QString& skin);

public slots:
  virtual void changeSkin(const QString& newSkin, bool force = false);
  virtual void changeSkinHelpLinkAction(const QUrl& link);

protected:
  KLFMainWin *_mainwin;
  QApplication *_app;
  QStyle *_defaultstyle;

  QMap<QString,QString> _baseStyleSheets;

  KLFPluginConfigAccess *_config;

  bool _applyDelayed;
  bool _preventpleasewait;
};


#endif
