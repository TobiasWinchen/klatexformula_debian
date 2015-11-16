/***************************************************************************
 *   file klfconfig.cpp
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
/* $Id: klfconfig.cpp 901 2014-07-29 22:35:04Z phfaist $ */

#include <iostream>

#include <QApplication>
#include <QMessageBox>
#include <QObject>
#include <QDir>
#include <QTextStream>
#include <QFont>
#include <QFontDatabase>
#include <QMap>
#include <QString>
#include <QListView> // icon view flow
#include <QLocale>
#include <QDesktopServices> // "My Documents" or "Documents" directory
#include <QDesktopWidget>
#include <QDomDocument>
#include <QDomElement>

#include <klfconfigbase.h>
#include <klfutil.h>
#include "klfmain.h"
#include "klfmainwin.h"
#include "klfconfig.h"
#include "klfconfig_p.h"



static const char * __klf_fallback_share_dir =
#if defined(Q_OS_WIN32) || defined(Q_OS_WIN64)  // windows
	"..";   // note: program is in a bin/ directory by default (this is for nsis-installer)
#elif defined(Q_OS_MAC) || defined(Q_OS_DARWIN) // Mac OS X
	"../Resources";
#else  // unix-like system
	"../share/klatexformula";
#endif


static const char * __klf_share_dir =
#ifdef KLF_SHARE_DIR  // defined by the build system
	KLF_SHARE_DIR;
#else
        NULL;
#endif

static QString __klf_share_dir_cached;

KLF_EXPORT QString klf_share_dir_abspath()
{
  if (!__klf_share_dir_cached.isEmpty())
    return __klf_share_dir_cached;

  klfDbg(klfFmtCC("cmake-share-dir=%s; fallback-share-dir=%s\n", __klf_share_dir,
		  __klf_fallback_share_dir)) ;

  QString sharedir;
  if (__klf_share_dir != NULL)
    sharedir = QLatin1String(__klf_share_dir);
  else
    sharedir = QLatin1String(__klf_fallback_share_dir);

  __klf_share_dir_cached = klfPrefixedPath(sharedir); // prefixed by app-dir-path
  klfDbg("share dir is "<<__klf_share_dir_cached) ;
  return __klf_share_dir_cached;
}



// global variable to access our config
// remember to initialize it in main() in main.cpp !
KLFConfig klfconfig;



/*void settings_write_QTextCharFormat(QSettings& s, const QString& basename,
  const QTextCharFormat& charfmt)
  {
  s.setValue(basename+"_charformat", charfmt);
  }
*/
static QTextCharFormat settings_read_QTextCharFormat(QSettings& s, const QString& basename,
						     const QTextCharFormat& dflt)
{
  QVariant val = s.value(basename+"_charformat", dflt);
  QTextFormat tf = val.value<QTextFormat>();
  return tf.toCharFormat();
}
/*
 template<class T>
 void settings_write_list(QSettings& s, const QString& basename, const QList<T>& list)
 {
 QList<QVariant> l;
 int k;
 for (k = 0; k < list.size(); ++k)
 l.append(QVariant(list[k]));
 s.setValue(basename+"_list", l);
 }
*/

template<class T>
static QList<T> settings_read_list(QSettings& s, const QString& basename, const QList<T>& dflt)
{
  QList<QVariant> l = s.value(basename+"_list", QList<QVariant>()).toList();
  if (l.size() == 0)
    return dflt;
  QList<T> list;
  int k;
  for (k = 0; k < l.size(); ++k)
    list.append(l[k].value<T>());
  return list;
}


// -----------------------------------------------------



#define KLFCONFIG_TEST_FIXED_FONT(found_fcode, fdb, fcode, f, fps)	\
  if (!found_fcode && fdb.isFixedPitch(f)) {				\
    fcode = QFont(f, fps);						\
    found_fcode = true;							\
  }


void KLFConfig::loadDefaults()
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;

  KLFCONFIGPROP_INIT_CONFIG(this) ;

  homeConfigDir = QDir::homePath() + "/.klatexformula";
  globalShareDir = klf_share_dir_abspath();
  homeConfigSettingsFile = homeConfigDir + "/klatexformula.conf";
  homeConfigSettingsFileIni = homeConfigDir + "/config";
  homeConfigDirRCCResources = homeConfigDir + "/rccresources";
  homeConfigDirPlugins = homeConfigDir + "/plugins";
  homeConfigDirPluginData = homeConfigDir + "/plugindata";
  homeConfigDirI18n = homeConfigDir + "/i18n";
  homeConfigDirUserScripts = homeConfigDir + "/userscripts";

  //debug: QMessageBox::information(0, "", QString("global share dir=")+globalShareDir);

  QFont cmuappfont = QFont();
  QFont fcodeMain = QFont();
  QFont fcodePreamble = QFont();

  if (qApp->inherits("QApplication")) { // and not QCoreApplication...

    QFontDatabase fdb;
    QFont f = QApplication::font();
#if defined(Q_WS_X11)
    int fps = QFontInfo(f).pointSize();
    double cmuffactor = 1.4;
    double codeffactor = 1.4;
    int cmufpsfinal = (int)(fps*cmuffactor+0.5);
    int codefpsfinal = (int)(fps*codeffactor+0.5);
#elif defined(Q_WS_WIN)
    int fps = QFontInfo(f).pointSize();
    double cmuffactor = 1.3;
    double codeffactor = 1.3;
    int cmufpsfinal = (int)(fps*cmuffactor+0.5);
    int codefpsfinal = (int)(fps*codeffactor+0.5);
#else // mac OS X
    //int fps = QFontInfo(f).pointSize();
    //double cmuffactor = 1.1;
    //double codeffactor = 1.3;
    int cmufpsfinal = 14;//(int)(fps*cmuffactor+0.5);
    int codefpsfinal = 14;//(int)(fps*codeffactor+0.5);
#endif

    defaultStdFont = f;

    cmuappfont = f;
#ifdef Q_WS_MAC
    if (fdb.families().contains("CMU Bright")) {
      cmuappfont = QFont("CMU Bright", cmufpsfinal);
    }
#else
    if (fdb.families().contains("CMU Sans Serif")) {
      // CMU Sans Serif is available ;-)
      cmuappfont = QFont("CMU Sans Serif", cmufpsfinal);
    }
#endif

    QFont fcode;
    bool found_fcode = false;
    int ps = codefpsfinal;
    KLFCONFIG_TEST_FIXED_FONT(found_fcode, fdb, fcode, "Courier 10 Pitch", ps);
    KLFCONFIG_TEST_FIXED_FONT(found_fcode, fdb, fcode, "ETL Fixed", ps);
    KLFCONFIG_TEST_FIXED_FONT(found_fcode, fdb, fcode, "Courier New", ps);
    KLFCONFIG_TEST_FIXED_FONT(found_fcode, fdb, fcode, "Efont Fixed", ps);
    KLFCONFIG_TEST_FIXED_FONT(found_fcode, fdb, fcode, "Adobe Courier", ps);
    KLFCONFIG_TEST_FIXED_FONT(found_fcode, fdb, fcode, "Courier", ps);
    KLFCONFIG_TEST_FIXED_FONT(found_fcode, fdb, fcode, "Misc Fixed", ps);
    KLFCONFIG_TEST_FIXED_FONT(found_fcode, fdb, fcode, "Monospace", ps);
    if ( ! found_fcode )
      fcode = f;

    fcodeMain = fcode;
    fcodeMain.setPointSize(ps+1);
    fcodePreamble = fcode;

    defaultCMUFont = cmuappfont;
    defaultTTFont = fcode;
  } else {
    defaultStdFont = QFont();
    defaultCMUFont = QFont();
    defaultTTFont = QFont();
  }

  // by default, this is first run!
  KLFCONFIGPROP_INIT(Core.thisVersionMajFirstRun, true);
  KLFCONFIGPROP_INIT(Core.thisVersionMajMinFirstRun, true) ;
  KLFCONFIGPROP_INIT(Core.thisVersionMajMinRelFirstRun, true) ;
  KLFCONFIGPROP_INIT(Core.thisVersionExactFirstRun, true) ;
  
  KLFCONFIGPROP_INIT(Core.libraryFileName, "library.klf.db") ;
  KLFCONFIGPROP_INIT(Core.libraryLibScheme, "klf+sqlite") ;
  
  KLFCONFIGPROP_INIT(UI.locale, QLocale::system().name()) ;
  klfDbg("System locale: "<<QLocale::system().name());
#ifdef KLF_NO_CMU_FONT
  KLFCONFIGPROP_INIT(UI.useSystemAppFont, true) ;
  KLFCONFIGPROP_INIT(UI.applicationFont, defaultStdFont) ;
#else
  KLFCONFIGPROP_INIT(UI.useSystemAppFont, false) ;
  KLFCONFIGPROP_INIT(UI.applicationFont, cmuappfont) ;
#endif
  KLFCONFIGPROP_INIT(UI.latexEditFont, fcodeMain) ;
  KLFCONFIGPROP_INIT(UI.preambleEditFont, fcodePreamble) ;
  KLFCONFIGPROP_INIT(UI.editorTabInsertsTab, false) ;
  KLFCONFIGPROP_INIT(UI.editorWrapLines, true) ;
  KLFCONFIGPROP_INIT(UI.previewTooltipMaxSize, QSize(800, 600)) ;
  KLFCONFIGPROP_INIT(UI.labelOutputFixedSize, QSize(280, 80)) ;
  KLFCONFIGPROP_INIT(UI.smallPreviewSize, QSize(280, 80));
  //  KLFCONFIGPROP_INIT(UI.savedWindowSize, QSize());
  QString swtype;
#ifdef Q_WS_MAC
  swtype = QLatin1String("Drawer");
#else
  swtype = QLatin1String("ShowHide");
#endif
  KLFCONFIGPROP_INIT(UI.detailsSideWidgetType, swtype) ;
  KLFCONFIGPROP_INIT(UI.lastSaveDir, QDir::homePath()) ;
  KLFCONFIGPROP_INIT(UI.symbolsPerLine, 6) ;
  KLFCONFIGPROP_INIT(UI.symbolIncludeWithPreambleDefs, true) ;
  QList<QColor> colorlist;
  colorlist.append(QColor(0,0,0));
  colorlist.append(QColor(255,255,255));
  colorlist.append(QColor(170,0,0));
  colorlist.append(QColor(0,0,128));
  colorlist.append(QColor(0,0,255));
  colorlist.append(QColor(0,85,0));
  colorlist.append(QColor(255,85,0));
  colorlist.append(QColor(0,255,255));
  colorlist.append(QColor(85,0,127));
  colorlist.append(QColor(128,255,255));
  KLFCONFIGPROP_INIT(UI.userColorList, colorlist) ;
  KLFCONFIGPROP_INIT(UI.colorChooseWidgetRecent, QList<QColor>()) ;
  KLFCONFIGPROP_INIT(UI.colorChooseWidgetCustom, QList<QColor>()) ;
  KLFCONFIGPROP_INIT(UI.maxUserColors, 12) ;
  KLFCONFIGPROP_INIT(UI.enableToolTipPreview, false) ;
  KLFCONFIGPROP_INIT(UI.enableRealTimePreview, true) ;
  KLFCONFIGPROP_INIT(UI.realTimePreviewExceptBattery, true) ;
  KLFCONFIGPROP_INIT(UI.autosaveLibraryMin, 5) ;
  KLFCONFIGPROP_INIT(UI.showHintPopups, true) ;
  KLFCONFIGPROP_INIT(UI.clearLatexOnly, false) ;
  KLFCONFIGPROP_INIT(UI.glowEffect, false) ;
#ifdef Q_WS_MAC
  QColor glowcolor = QColor(155, 207, 255, 62);
#else
  QColor glowcolor = QColor(128, 255, 128, 12);
#endif
  KLFCONFIGPROP_INIT(UI.glowEffectColor, glowcolor) ;
  KLFCONFIGPROP_INIT(UI.glowEffectRadius, 4) ;
  KLFCONFIGPROP_INIT(UI.customMathModes, QStringList()) ;
  KLFCONFIGPROP_INIT(UI.emacsStyleBackspaceSearch, true) ;
  KLFCONFIGPROP_INIT(UI.macBrushedMetalLook, true) ;
  KLFCONFIGPROP_INIT(ExportData.copyExportProfile, "default") ;
  KLFCONFIGPROP_INIT(ExportData.dragExportProfile, "default") ;
  KLFCONFIGPROP_INIT(ExportData.showExportProfilesLabel, true) ;
  KLFCONFIGPROP_INIT(ExportData.menuExportProfileAffectsDrag, true) ;
  KLFCONFIGPROP_INIT(ExportData.menuExportProfileAffectsCopy, true) ;
  KLFCONFIGPROP_INIT(ExportData.oooExportScale, 1.6) ;
  KLFCONFIGPROP_INIT(ExportData.htmlExportDpi, 150);
  // query display DPI
  int dispdpi = 96;
  /*  if (qApp->inherits("QApplication")) {
      QDesktopWidget *w = QApplication::desktop();
      if (w != NULL) {
      // dispdpi = (w->physicalDpiX() + w->physicalDpiY()) / 2;
      dispdpi = w->physicalDpiX();
      }
      } */
  KLFCONFIGPROP_INIT(ExportData.htmlExportDisplayDpi, dispdpi);

  KLFCONFIGPROP_INIT(SyntaxHighlighter.enabled, true) ;
  KLFCONFIGPROP_INIT(SyntaxHighlighter.highlightParensOnly, false) ;
  KLFCONFIGPROP_INIT(SyntaxHighlighter.highlightLonelyParens, true) ;
  //  KLFCONFIGPROP_INIT(SyntaxHighlighter.matchParenTypes, true) ;
  QTextCharFormat f_keyword;
  QTextCharFormat f_comment;
  QTextCharFormat f_parenmatch;
  QTextCharFormat f_parenmismatch;
  QTextCharFormat f_lonelyparen;
  f_keyword.setForeground(QColor(0, 0, 128));
  f_comment.setForeground(QColor(180, 0, 0));
  f_comment.setFontItalic(true);
  f_parenmatch.setBackground(QColor(180, 238, 180));
  f_parenmismatch.setBackground(QColor(255, 20, 147));
  f_lonelyparen.setForeground(QColor(255, 0, 255));
  f_lonelyparen.setFontWeight(QFont::Bold);
  KLFCONFIGPROP_INIT(SyntaxHighlighter.fmtKeyword, f_keyword) ;
  KLFCONFIGPROP_INIT(SyntaxHighlighter.fmtComment, f_comment) ;
  KLFCONFIGPROP_INIT(SyntaxHighlighter.fmtParenMatch, f_parenmatch) ;
  KLFCONFIGPROP_INIT(SyntaxHighlighter.fmtParenMismatch, f_parenmismatch) ;
  KLFCONFIGPROP_INIT(SyntaxHighlighter.fmtLonelyParen, f_lonelyparen) ;

  // invalid value, by convention ".". if the config is not read from file, then settings will
  // be detected in detectMissingSettings()
  KLFCONFIGPROP_INIT_DEFNOTDEF(BackendSettings.tempDir, ".") ;
  KLFCONFIGPROP_INIT_DEFNOTDEF(BackendSettings.execLatex, ".") ;
  KLFCONFIGPROP_INIT_DEFNOTDEF(BackendSettings.execDvips, ".") ;
  KLFCONFIGPROP_INIT_DEFNOTDEF(BackendSettings.execGs, ".") ;
  KLFCONFIGPROP_INIT_DEFNOTDEF(BackendSettings.execEpstopdf, ".") ;
  KLFCONFIGPROP_INIT_DEFNOTDEF(BackendSettings.execenv, QStringList()) ;
  KLFCONFIGPROP_INIT(BackendSettings.setTexInputs, QString());

  KLFCONFIGPROP_INIT(BackendSettings.lborderoffset, 0) ;
  KLFCONFIGPROP_INIT(BackendSettings.tborderoffset, 0) ;
  KLFCONFIGPROP_INIT(BackendSettings.rborderoffset, 0) ;
  KLFCONFIGPROP_INIT(BackendSettings.bborderoffset, 0) ;
  KLFCONFIGPROP_INIT(BackendSettings.calcEpsBoundingBox, true) ;
  KLFCONFIGPROP_INIT(BackendSettings.outlineFonts, true) ;
  KLFCONFIGPROP_INIT_DEFNOTDEF(BackendSettings.wantPDF, true) ;
  KLFCONFIGPROP_INIT_DEFNOTDEF(BackendSettings.wantSVG, true) ;
  KLFCONFIGPROP_INIT(BackendSettings.userScriptAddPath, QStringList() );

  KLFCONFIGPROP_INIT(LibraryBrowser.colorFound, QColor(128, 255, 128)) ;
  KLFCONFIGPROP_INIT(LibraryBrowser.colorNotFound, QColor(255, 128, 128)) ;
  KLFCONFIGPROP_INIT(LibraryBrowser.restoreURLs, false) ;
  KLFCONFIGPROP_INIT(LibraryBrowser.confirmClose, true) ;
  KLFCONFIGPROP_INIT(LibraryBrowser.groupSubCategories, true) ;
  KLFCONFIGPROP_INIT(LibraryBrowser.iconViewFlow, QListView::TopToBottom) ;
  KLFCONFIGPROP_INIT(LibraryBrowser.historyTagCopyToArchive, true) ;
  KLFCONFIGPROP_INIT(LibraryBrowser.lastFileDialogPath,
		     QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation)) ; // "My Documents"
  KLFCONFIGPROP_INIT(LibraryBrowser.treePreviewSizePercent, 75) ;
  KLFCONFIGPROP_INIT(LibraryBrowser.listPreviewSizePercent, 75) ;
  KLFCONFIGPROP_INIT(LibraryBrowser.iconPreviewSizePercent, 100) ;

  Plugins.pluginConfig = QMap< QString, QMap<QString,QVariant> >() ;
  UserScripts.userScriptConfig = QMap< QString, QMap<QString,QVariant> >() ;
}

void KLFConfig::detectMissingSettings()
{
  int neededsettings = 
    !BackendSettings.tempDir.defaultValueDefinite()      << 0 |
    !BackendSettings.execLatex.defaultValueDefinite()    << 1 |
    !BackendSettings.execDvips.defaultValueDefinite()    << 2 |
    !BackendSettings.execGs.defaultValueDefinite()       << 3 |
    !BackendSettings.execEpstopdf.defaultValueDefinite() << 4 |
    !BackendSettings.execenv.defaultValueDefinite()      << 5 |
    !BackendSettings.wantPDF.defaultValueDefinite()      << 6 |
    !BackendSettings.wantSVG.defaultValueDefinite()      << 7 ;

  if (neededsettings) {
    KLFBackend::klfSettings defaultsettings;
    KLFBackend::detectSettings(&defaultsettings);
    if (neededsettings & (1<<0))
      BackendSettings.tempDir.setDefaultValue(defaultsettings.tempdir);
    if (neededsettings & (1<<1))
      BackendSettings.execLatex.setDefaultValue(defaultsettings.latexexec);
    if (neededsettings & (1<<2))
      BackendSettings.execDvips.setDefaultValue(defaultsettings.dvipsexec);
    if (neededsettings & (1<<3))
      BackendSettings.execGs.setDefaultValue(defaultsettings.gsexec);
    if (neededsettings & (1<<4))
      BackendSettings.execEpstopdf.setDefaultValue(defaultsettings.epstopdfexec);
    if (neededsettings & (1<<5))
      BackendSettings.execenv.setDefaultValue(BackendSettings.execenv() << defaultsettings.execenv);
    if (neededsettings & (1<<6))
      BackendSettings.wantPDF.setDefaultValue(defaultsettings.wantPDF);
    if (neededsettings & (1<<7))
      BackendSettings.wantSVG.setDefaultValue(defaultsettings.wantSVG);
  }

}



int KLFConfig::ensureHomeConfigDir()
{
  if ( !klfEnsureDir(homeConfigDir) )
    return -1;
  if ( !klfEnsureDir(homeConfigDirRCCResources) )
    return -1;
  if ( !klfEnsureDir(homeConfigDirPlugins) )
    return -1;
  if ( !klfEnsureDir(homeConfigDirPluginData) )
    return -1;
  if ( !klfEnsureDir(homeConfigDirI18n) )
    return -1;
  if ( !klfEnsureDir(homeConfigDirUserScripts) )
    return -1;

  return 0;
}


bool KLFConfig::checkExePaths()
{
  return QFileInfo(BackendSettings.execLatex).isExecutable() &&
    QFileInfo(BackendSettings.execDvips).isExecutable() &&
    QFileInfo(BackendSettings.execGs).isExecutable() &&
    ( BackendSettings.execEpstopdf().isEmpty() ||
      QFileInfo(BackendSettings.execEpstopdf).isExecutable()) ;
}




int KLFConfig::readFromConfig()
{
  klfDbgT(" reading config.") ;

  ensureHomeConfigDir();

  QString globalconfig = globalShareDir+"/klatexformula.conf";
  klfDbg("Testing for global config file "<<globalconfig);
  if (QFile::exists(globalconfig)) {
    klfDbg("Reading configuration from "<<globalconfig);
    // pre-load global settings
    readFromConfig_v2(globalconfig);
  }

  if (QFile::exists(homeConfigSettingsFile)) {
    klfDbg("Reading configuration from "<<homeConfigSettingsFile<<" ...");
    return readFromConfig_v2(homeConfigSettingsFile);
  }
  if (QFile::exists(homeConfigSettingsFileIni)) {
    return readFromConfig_v1();
  }

  return -1;
}

static QString firstRunConfigKey(int N)
{
  QString s = QString("versionFirstRun-%1_").arg(N);
  if (N >= 4)
    return s + QLatin1String(KLF_VERSION_STRING);

  if (N-- > 0)
    s += QString("%1").arg(KLF_VERSION_MAJ);
  if (N-- > 0)
    s += QString(".%1").arg(KLF_VERSION_MIN);
  if (N-- > 0)
    s += QString(".%1").arg(KLF_VERSION_REL);
  return s;
}

int KLFConfig::readFromConfig_v2(const QString& fname)
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;

  QSettings s(fname, QSettings::IniFormat);

  qDebug("Reading base configuration");

  s.beginGroup("Core");
  klf_config_read(s, firstRunConfigKey(1), &Core.thisVersionMajFirstRun);
  klf_config_read(s, firstRunConfigKey(2), &Core.thisVersionMajMinFirstRun);
  klf_config_read(s, firstRunConfigKey(3), &Core.thisVersionMajMinRelFirstRun);
  klf_config_read(s, firstRunConfigKey(4), &Core.thisVersionExactFirstRun);
  klf_config_read(s, "libraryfilename", &Core.libraryFileName);
  klf_config_read(s, "librarylibscheme", &Core.libraryLibScheme);
  s.endGroup();

  s.beginGroup("UI");
  klf_config_read(s, "locale", &UI.locale);
  klf_config_read(s, "usesystemfont", &UI.useSystemAppFont);
  klf_config_read(s, "applicationfont", &UI.applicationFont);
  klf_config_read(s, "latexeditfont", &UI.latexEditFont);
  klf_config_read(s, "preambleeditfont", &UI.preambleEditFont);
  klf_config_read(s, "editortabinsertstab", &UI.editorTabInsertsTab);
  klf_config_read(s, "editorwraplines", &UI.editorWrapLines);
  klf_config_read(s, "previewtooltipmaxsize", &UI.previewTooltipMaxSize);
  klf_config_read(s, "lbloutputfixedsize", &UI.labelOutputFixedSize);
  klf_config_read(s, "smallpreviewsize", &UI.smallPreviewSize);
  //  klf_config_read(s, "savedwindowsize", &UI.savedWindowSize);
  klf_config_read(s, "detailssidewidgettype", &UI.detailsSideWidgetType);
  klf_config_read(s, "lastsavedir", &UI.lastSaveDir);
  klf_config_read(s, "symbolsperline", &UI.symbolsPerLine);
  klf_config_read(s, "symbolincludewithpreambledefs", &UI.symbolIncludeWithPreambleDefs);
  klf_config_read_list(s, "usercolorlist", &UI.userColorList);
  klf_config_read_list(s, "colorchoosewidgetrecent", &UI.colorChooseWidgetRecent);
  klf_config_read_list(s, "colorchoosewidgetcustom", &UI.colorChooseWidgetCustom);
  klf_config_read(s, "maxusercolors", &UI.maxUserColors);
  klf_config_read(s, "enabletooltippreview", &UI.enableToolTipPreview);
  klf_config_read(s, "enablerealtimepreview", &UI.enableRealTimePreview);
  klf_config_read(s, "realtimepreviewexceptbattery", &UI.realTimePreviewExceptBattery);
  klf_config_read(s, "autosavelibrarymin", &UI.autosaveLibraryMin);
  klf_config_read(s, "showhintpopups", &UI.showHintPopups);
  klf_config_read(s, "clearlatexonly", &UI.clearLatexOnly);
  klf_config_read(s, "gloweffect", &UI.glowEffect);
  klf_config_read(s, "gloweffectcolor", &UI.glowEffectColor);
  klfDbg("Read glow effect color from config: color="<<UI.glowEffectColor
	 <<", alpha="<<UI.glowEffectColor().alpha());
  klf_config_read(s, "gloweffectradius", &UI.glowEffectRadius);
  klf_config_read(s, "custommathmodes", &UI.customMathModes);
  klf_config_read(s, "emacsstylebackspacesearch", &UI.emacsStyleBackspaceSearch);
  klf_config_read(s, "macbrushedmetallook", &UI.macBrushedMetalLook);
  // backwards-compatible (v 3.2): read first in UI group, then overwrite with settings in ExportData group
  klf_config_read(s, "copyexportprofile", &ExportData.copyExportProfile);
  klf_config_read(s, "dragexportprofile", &ExportData.dragExportProfile);
  klf_config_read(s, "showexportprofileslabel", &ExportData.showExportProfilesLabel);
  klf_config_read(s, "menuexportprofileaffectsdrag", &ExportData.menuExportProfileAffectsDrag);
  klf_config_read(s, "menuexportprofileaffectscopy", &ExportData.menuExportProfileAffectsCopy);
  klf_config_read(s, "oooexportscale", &ExportData.oooExportScale);
  s.endGroup();

  s.beginGroup("ExportData");
  klf_config_read(s, "copyexportprofile", &ExportData.copyExportProfile);
  klf_config_read(s, "dragexportprofile", &ExportData.dragExportProfile);
  klf_config_read(s, "showexportprofileslabel", &ExportData.showExportProfilesLabel);
  klf_config_read(s, "menuexportprofileaffectsdrag", &ExportData.menuExportProfileAffectsDrag);
  klf_config_read(s, "menuexportprofileaffectscopy", &ExportData.menuExportProfileAffectsCopy);
  klf_config_read(s, "oooexportscale", &ExportData.oooExportScale);
  klf_config_read(s, "htmlexportdpi", &ExportData.htmlExportDpi);
  klf_config_read(s, "htmlexportdisplaydpi", &ExportData.htmlExportDisplayDpi);
  s.endGroup();

  s.beginGroup("SyntaxHighlighter");
  klf_config_read(s, "enabled", &SyntaxHighlighter.enabled);
  klf_config_read(s, "highlightparensonly", &SyntaxHighlighter.highlightParensOnly);
  klf_config_read(s, "highlightlonelyparens", &SyntaxHighlighter.highlightLonelyParens);
  //  klf_config_read(s, "matchparentypes", &SyntaxHighlighter.matchParenTypes);
  klf_config_read<QTextCharFormat>(s, "keyword", &SyntaxHighlighter.fmtKeyword);
  klf_config_read<QTextCharFormat>(s, "comment", &SyntaxHighlighter.fmtComment);
  klf_config_read<QTextCharFormat>(s, "parenmatch", &SyntaxHighlighter.fmtParenMatch);
  klf_config_read<QTextCharFormat>(s, "parenmismatch", &SyntaxHighlighter.fmtParenMismatch);
  klf_config_read<QTextCharFormat>(s, "lonelyparen", &SyntaxHighlighter.fmtLonelyParen);
  s.endGroup();

  s.beginGroup("BackendSettings");
  klf_config_read(s, "tempdir", &BackendSettings.tempDir);
  klf_config_read(s, "latexexec", &BackendSettings.execLatex);
  klf_config_read(s, "dvipsexec", &BackendSettings.execDvips);
  klf_config_read(s, "gsexec", &BackendSettings.execGs);
  klf_config_read(s, "epstopdfexec", &BackendSettings.execEpstopdf);
  klf_config_read(s, "execenv", &BackendSettings.execenv);
  klf_config_read(s, "settexinputs", &BackendSettings.setTexInputs);
  klf_config_read(s, "lborderoffset", &BackendSettings.lborderoffset);
  klf_config_read(s, "tborderoffset", &BackendSettings.tborderoffset);
  klf_config_read(s, "rborderoffset", &BackendSettings.rborderoffset);
  klf_config_read(s, "bborderoffset", &BackendSettings.bborderoffset);
  klf_config_read(s, "calcepsboundingbox", &BackendSettings.calcEpsBoundingBox);
  klf_config_read(s, "outlinefonts", &BackendSettings.outlineFonts);
  klf_config_read(s, "wantpdf", &BackendSettings.wantPDF);
  klf_config_read(s, "wantsvg", &BackendSettings.wantSVG);
  klf_config_read(s, "userscriptaddpath", &BackendSettings.userScriptAddPath);
  s.endGroup();

  s.beginGroup("LibraryBrowser");
  klf_config_read(s, "colorfound", &LibraryBrowser.colorFound);
  klf_config_read(s, "colornotfound", &LibraryBrowser.colorNotFound);
  klf_config_read(s, "restoreurls", &LibraryBrowser.restoreURLs);
  klf_config_read(s, "confirmclose", &LibraryBrowser.confirmClose);
  klf_config_read(s, "groupsubcategories", &LibraryBrowser.groupSubCategories);
  klf_config_read(s, "iconviewflow", &LibraryBrowser.iconViewFlow);
  klf_config_read(s, "historytagcopytoarchive", &LibraryBrowser.historyTagCopyToArchive);
  klf_config_read(s, "lastfiledialogpath", &LibraryBrowser.lastFileDialogPath);
  klf_config_read(s, "treepreviewsizepercent", &LibraryBrowser.treePreviewSizePercent);
  klf_config_read(s, "listpreviewsizepercent", &LibraryBrowser.listPreviewSizePercent);
  klf_config_read(s, "iconpreviewsizepercent", &LibraryBrowser.iconPreviewSizePercent);
  s.endGroup();

  // Special treatment for Plugins.pluginConfig
  // for reading, we cannot rely on klf_plugins since we are called before plugins are loaded!
  int k, j;
  QDir plugindatadir = QDir(homeConfigDirPluginData);
  QStringList plugindirs = plugindatadir.entryList(QDir::Dirs);
  for (k = 0; k < plugindirs.size(); ++k) {
    if (plugindirs[k] == "." || plugindirs[k] == "..")
      continue;
    qDebug("Reading config for plugin %s", qPrintable(plugindirs[k]));
    QString fn = plugindatadir.absoluteFilePath(plugindirs[k])+"/"+plugindirs[k]+".conf";
    if ( ! QFile::exists(fn) ) {
      qDebug("\tskipping plugin %s since the file %s does not exist.",
	     qPrintable(plugindirs[k]), qPrintable(fn));
      continue;
    }
    QSettings psettings(fn, QSettings::IniFormat);
    QVariantMap pconfmap;
    QStringList keys = psettings.allKeys();
    for (j = 0; j < keys.size(); ++j) {
      pconfmap[keys[j]] = psettings.value(keys[j]);
    }
    Plugins.pluginConfig[plugindirs[k]] = pconfmap;
  }

  // Special treatment for UserScripts.userScriptConfig
  // save into a dedicated XML file
  KLF_BLOCK {
    QDomDocument xmldoc;
    QFile usfile(homeConfigDir+"/userscripts.xml");
    if (!usfile.exists()) {
      // don't attempt to load user script settings if file does not exist
      break;
    }
    KLF_TRY( usfile.open(QIODevice::ReadOnly) ,
			  "Can't open file "<<usfile.fileName()<<" for read access!", break; );
    QString errorMsg;
    int errorLine, errorColumn;
    KLF_TRY( xmldoc.setContent(&usfile, &errorMsg, &errorLine, &errorColumn) ,
	     "Error reading XML "<<usfile.fileName()<<": line "<<errorLine<<" column "<<errorColumn<<":\n"
	     <<errorMsg,
	     break ) ;
    usfile.close();

    QDomElement root = xmldoc.documentElement();
    KLF_ASSERT_CONDITION(root.tagName() == QLatin1String("userscript-config"),
			 "Error parsing userscript config XML", break; );

    QDomNodeList nodelist = root.childNodes();
    int k;
    for (k = 0; k < nodelist.count(); ++k) {
      QDomNode node = nodelist.at(k);
      if (!node.isElement()) {
	klfDbg("skipping non-element node "<<node.nodeName());
	continue;
      }
      QDomElement usnode = node.toElement();

      KLF_ASSERT_CONDITION(usnode.hasAttribute("name"),
			   "Node <userscript> does not have 'name' attribute",
			   continue; ) ;
      QString usname = usnode.attribute("name");
      QVariantMap usconfig = klfLoadVariantMapFromXML(usnode);

      UserScripts.userScriptConfig[usname] = usconfig;
    }
  }

  // POST-CONFIG-READ SETUP

  // forbid empty locale
  if (klfconfig.UI.locale().isEmpty())
    klfconfig.UI.locale = "en_US";
  // set Qt default locale to ours
  QLocale::setDefault(klfconfig.UI.locale());

  return 0;
}


int KLFConfig::writeToConfig()
{
  ensureHomeConfigDir();
  QSettings s(homeConfigSettingsFile, QSettings::IniFormat);

  bool thisVersionFirstRunFalse = false;

  s.beginGroup("Core");
  klf_config_write_value(s, firstRunConfigKey(1), &thisVersionFirstRunFalse);
  klf_config_write_value(s, firstRunConfigKey(2), &thisVersionFirstRunFalse);
  klf_config_write_value(s, firstRunConfigKey(3), &thisVersionFirstRunFalse);
  klf_config_write_value(s, firstRunConfigKey(4), &thisVersionFirstRunFalse);
  klf_config_write(s, "libraryfilename", &Core.libraryFileName);
  klf_config_write(s, "librarylibscheme", &Core.libraryLibScheme);
  s.endGroup();

  s.beginGroup("UI");
  klf_config_write(s, "locale", &UI.locale);
  klf_config_write(s, "usesystemfont", &UI.useSystemAppFont);
  klf_config_write(s, "applicationfont", &UI.applicationFont);
  klf_config_write(s, "latexeditfont", &UI.latexEditFont);
  klf_config_write(s, "preambleeditfont", &UI.preambleEditFont);
  klf_config_write(s, "editortabinsertstab", &UI.editorTabInsertsTab);
  klf_config_write(s, "editorwraplines", &UI.editorWrapLines);
  klf_config_write(s, "previewtooltipmaxsize", &UI.previewTooltipMaxSize);
  klf_config_write(s, "lbloutputfixedsize", &UI.labelOutputFixedSize);
  klf_config_write(s, "smallpreviewsize", &UI.smallPreviewSize);
  //  klf_config_write(s, "savedwindowsize", &UI.savedWindowSize);
  klf_config_write(s, "detailssidewidgettype", &UI.detailsSideWidgetType);
  klf_config_write(s, "lastsavedir", &UI.lastSaveDir);
  klf_config_write(s, "symbolsperline", &UI.symbolsPerLine);
  klf_config_write(s, "symbolincludewithpreambledefs", &UI.symbolIncludeWithPreambleDefs);
  klf_config_write_list(s, "usercolorlist", &UI.userColorList);
  klf_config_write_list(s, "colorchoosewidgetrecent", &UI.colorChooseWidgetRecent);
  klf_config_write_list(s, "colorchoosewidgetcustom", &UI.colorChooseWidgetCustom);
  klf_config_write(s, "maxusercolors", &UI.maxUserColors);
  klf_config_write(s, "enabletooltippreview", &UI.enableToolTipPreview);
  klf_config_write(s, "enablerealtimepreview", &UI.enableRealTimePreview);
  klf_config_write(s, "realtimepreviewexceptbattery", &UI.realTimePreviewExceptBattery);
  klf_config_write(s, "autosavelibrarymin", &UI.autosaveLibraryMin);
  klf_config_write(s, "showhintpopups", &UI.showHintPopups);
  klf_config_write(s, "clearlatexonly", &UI.clearLatexOnly);
  klf_config_write(s, "gloweffect", &UI.glowEffect);
  klf_config_write(s, "gloweffectcolor", &UI.glowEffectColor);
  klf_config_write(s, "gloweffectradius", &UI.glowEffectRadius);
  klf_config_write(s, "custommathmodes", &UI.customMathModes);
  klf_config_write(s, "emacsstylebackspacesearch", &UI.emacsStyleBackspaceSearch);
  klf_config_write(s, "macbrushedmetallook", &UI.macBrushedMetalLook);
  s.endGroup();

  s.beginGroup("ExportData");
  klf_config_write(s, "copyexportprofile", &ExportData.copyExportProfile);
  klf_config_write(s, "dragexportprofile", &ExportData.dragExportProfile);
  klf_config_write(s, "showexportprofileslabel", &ExportData.showExportProfilesLabel);
  klf_config_write(s, "menuexportprofileaffectsdrag", &ExportData.menuExportProfileAffectsDrag);
  klf_config_write(s, "menuexportprofileaffectscopy", &ExportData.menuExportProfileAffectsCopy);
  klf_config_write(s, "oooexportscale", &ExportData.oooExportScale);
  klf_config_write(s, "htmlexportdpi", &ExportData.htmlExportDpi);
  klf_config_write(s, "htmlexportdisplaydpi", &ExportData.htmlExportDisplayDpi);
  s.endGroup();

  s.beginGroup("SyntaxHighlighter");
  klf_config_write(s, "enabled", &SyntaxHighlighter.enabled);
  klf_config_write(s, "highlightparensonly", &SyntaxHighlighter.highlightParensOnly);
  klf_config_write(s, "highlightlonelyparens", &SyntaxHighlighter.highlightLonelyParens);
  //  klf_config_write(s, "matchparentypes", &SyntaxHighlighter.matchParenTypes);
  klf_config_write(s, "keyword", &SyntaxHighlighter.fmtKeyword);
  klf_config_write(s, "comment", &SyntaxHighlighter.fmtComment);
  klf_config_write(s, "parenmatch", &SyntaxHighlighter.fmtParenMatch);
  klf_config_write(s, "parenmismatch", &SyntaxHighlighter.fmtParenMismatch);
  klf_config_write(s, "lonelyparen", &SyntaxHighlighter.fmtLonelyParen);
  s.endGroup();

  s.beginGroup("BackendSettings");
  klf_config_write(s, "tempdir", &BackendSettings.tempDir);
  klf_config_write(s, "latexexec", &BackendSettings.execLatex);
  klf_config_write(s, "dvipsexec", &BackendSettings.execDvips);
  klf_config_write(s, "gsexec", &BackendSettings.execGs);
  klf_config_write(s, "epstopdfexec", &BackendSettings.execEpstopdf);
  klf_config_write(s, "execenv", &BackendSettings.execenv);
  klf_config_write(s, "settexinputs", &BackendSettings.setTexInputs);
  klf_config_write(s, "lborderoffset", &BackendSettings.lborderoffset);
  klf_config_write(s, "tborderoffset", &BackendSettings.tborderoffset);
  klf_config_write(s, "rborderoffset", &BackendSettings.rborderoffset);
  klf_config_write(s, "bborderoffset", &BackendSettings.bborderoffset); 
  klf_config_write(s, "calcepsboundingbox", &BackendSettings.calcEpsBoundingBox);
  klf_config_write(s, "outlinefonts", &BackendSettings.outlineFonts);
  klf_config_write(s, "wantpdf", &BackendSettings.wantPDF);
  klf_config_write(s, "wantsvg", &BackendSettings.wantSVG);
  klf_config_write(s, "userscriptaddpath", &BackendSettings.userScriptAddPath);
  s.endGroup();

  s.beginGroup("LibraryBrowser");
  klf_config_write(s, "colorfound", &LibraryBrowser.colorFound);
  klf_config_write(s, "colornotfound", &LibraryBrowser.colorNotFound);
  klf_config_write(s, "restoreurls", &LibraryBrowser.restoreURLs);
  klf_config_write(s, "confirmclose", &LibraryBrowser.confirmClose);
  klf_config_write(s, "groupsubcategories", &LibraryBrowser.groupSubCategories);
  klf_config_write(s, "iconviewflow", &LibraryBrowser.iconViewFlow);
  klf_config_write(s, "historytagcopytoarchive", &LibraryBrowser.historyTagCopyToArchive);
  klf_config_write(s, "lastfiledialogpath", &LibraryBrowser.lastFileDialogPath);
  klf_config_write(s, "treepreviewsizepercent", &LibraryBrowser.treePreviewSizePercent);
  klf_config_write(s, "listpreviewsizepercent", &LibraryBrowser.listPreviewSizePercent);
  klf_config_write(s, "iconpreviewsizepercent", &LibraryBrowser.iconPreviewSizePercent);
  s.endGroup();

  // Special treatment for Plugins.pluginConfig
  int k;
  for (k = 0; k < klf_plugins.size(); ++k) {
    QString fn = homeConfigDirPluginData+"/"+klf_plugins[k].name+"/"+klf_plugins[k].name+".conf";
    QSettings psettings(fn, QSettings::IniFormat);
    QVariantMap pconfmap = Plugins.pluginConfig[klf_plugins[k].name];
    QVariantMap::const_iterator it;
    for (it = pconfmap.begin(); it != pconfmap.end(); ++it) {
      psettings.setValue(it.key(), it.value());
    }
    psettings.sync();
  }

  // Special treatment for UserScripts.userScriptConfig
  // save into a dedicated XML file
  do { // do { ... } while (false);   is used to allow 'break' statement to skip to end of block
    QDomDocument xmldoc("userscript-config");
    QDomElement root = xmldoc.createElement("userscript-config");
    xmldoc.appendChild(root);
    for (QMap<QString, QMap<QString, QVariant> >::const_iterator it = UserScripts.userScriptConfig.begin();
	 it != UserScripts.userScriptConfig.end(); ++it) {
      QString usname = it.key();
      QVariantMap usconfig = it.value();

      klfDbg("saving config for user script "<< usname<< "; config="<<usconfig) ;

      QDomElement scriptconfig = xmldoc.createElement("userscript");
      scriptconfig.setAttribute("name", usname);

      klfSaveVariantMapToXML(usconfig, scriptconfig);
      root.appendChild(scriptconfig);
    }
    // now, save the document
    QByteArray userscriptconfdata = xmldoc.toByteArray(4);
    QFile usfile(homeConfigDir+"/userscripts.xml");
    KLF_ASSERT_CONDITION( usfile.open(QIODevice::WriteOnly) ,
			  "Can't open file "<<usfile.fileName()<<" for write access!", break; );
    usfile.write(userscriptconfdata);
    usfile.close();
  } while (0);


  s.sync();
  return 0;
}




KLFPluginConfigAccess KLFConfig::getPluginConfigAccess(const QString& name)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ; klfDbg("... for plugin: "<<name) ;
  return KLFPluginConfigAccess(this, name);
}


// --------------------------------------




KLFPluginConfigAccess::KLFPluginConfigAccess()
{
  _config = NULL;
  _pluginname = QString::null;
}
KLFPluginConfigAccess::KLFPluginConfigAccess(const KLFPluginConfigAccess& other)
  : _config(other._config), _pluginname(other._pluginname)
{
  klfDbg("made copy. _config="<<_config<<"; _pluginname="<<_pluginname) ;
  if (_config != NULL) {
    klfDbg("_config->homeConfigDir: "<<_config->homeConfigDir) ;
  }
}
KLFPluginConfigAccess::~KLFPluginConfigAccess()
{
}

KLFPluginConfigAccess::KLFPluginConfigAccess(KLFConfig *configObject, const QString& pluginName)
{
  _config = configObject;
  _pluginname = pluginName;

  klfDbg("_config="<<_config<<", _pluginname="<<_pluginname) ;
  if (_config != NULL) {
    klfDbg("_config->homeConfigDir: "<<_config->homeConfigDir) ;
  }
}



QString KLFPluginConfigAccess::homeConfigDir() const
{
  if ( _config == NULL ) {
    qWarning("KLFPluginConfigAccess::homeConfigDir: Invalid Config Pointer!\n");
    return QString();
  }
  klfDbg("_config->homeConfigDir="<<_config->homeConfigDir) ;
  return _config->homeConfigDir;
}

QString KLFPluginConfigAccess::globalShareDir() const
{
  if ( _config == NULL ) {
    qWarning("KLFPluginConfigAccess::homeConfigDir: Invalid Config Pointer!\n");
    return QString();
  }

  return _config->globalShareDir;
}

QString KLFPluginConfigAccess::tempDir() const
{
  if ( _config == NULL ) {
    qWarning("KLFPluginConfigAccess::tempDir: Invalid Config Pointer!\n");
    return QString();
  }

  return _config->BackendSettings.tempDir;
}

QString KLFPluginConfigAccess::homeConfigPluginDataDir(bool createIfNeeded) const
{
  if ( _config == NULL ) {
    qWarning("KLFPluginConfigAccess::homeConfigPluginDataDir: Invalid Config Pointer!\n");
    return QString();
  }
  klfDbg("_config->homeConfigDirPluginData is "<<_config->homeConfigDirPluginData) ;

  QString d = _config->homeConfigDirPluginData + "/" + _pluginname;
  if ( createIfNeeded && ! klfEnsureDir(d) ) {
    qWarning("KLFPluginConfigAccess::homeConfigPluginDataDir: Can't create directory: `%s'",
	     qPrintable(d));
    return QString();
  }
  return d;
}

QVariant KLFPluginConfigAccess::readValue(const QString& key) const
{
  if ( _config == NULL ) {
    qWarning("KLFPluginConfigAccess::readValue: Invalid Config Pointer!\n");
    return QVariant();
  }

  if ( ! _config->Plugins.pluginConfig[_pluginname].contains(key) )
    return QVariant();

  return _config->Plugins.pluginConfig[_pluginname][key];
}

QVariant KLFPluginConfigAccess::makeDefaultValue(const QString& key, const QVariant& defaultValue)
{
  if ( _config == NULL ) {
    qWarning("KLFPluginConfigAccess::makeDefaultValue: Invalid Config Pointer!\n");
    return QVariant();
  }

  if (_config->Plugins.pluginConfig[_pluginname].contains(key))
    return _config->Plugins.pluginConfig[_pluginname][key];

  // assign the value into the plugin config, and return it
  return ( _config->Plugins.pluginConfig[_pluginname][key] = defaultValue );
}
void KLFPluginConfigAccess::writeValue(const QString& key, const QVariant& value)
{
  if ( _config == NULL ) {
    qWarning("KLFPluginConfigAccess::writeValue: Invalid Config Pointer!\n");
    return;
  }

  _config->Plugins.pluginConfig[_pluginname][key] = value;
}








// ----------------------









// KEPT FOR COMPATIBILITY WITH OLDER VERSIONS

int KLFConfig::readFromConfig_v1()
{
  QSettings s(homeConfigSettingsFileIni, QSettings::IniFormat);

  s.beginGroup("UI");
  UI.locale = s.value("locale", UI.locale.toVariant()).toString();
  // ingnore KLF 3.1 app font setting, we have our nice CMU Sans Serif font ;-)
  //  UI.applicationFont = s.value("applicationfont", UI.applicationFont).value<QFont>();
  UI.latexEditFont = s.value("latexeditfont", UI.latexEditFont.toVariant()).value<QFont>();
  UI.preambleEditFont = s.value("preambleeditfont", UI.preambleEditFont.toVariant()).value<QFont>();
  UI.previewTooltipMaxSize = s.value("previewtooltipmaxsize", UI.previewTooltipMaxSize.toVariant()).toSize();
  UI.labelOutputFixedSize = s.value("lbloutputfixedsize", UI.labelOutputFixedSize.toVariant() ).toSize();
  UI.lastSaveDir = s.value("lastsavedir", UI.lastSaveDir.toVariant()).toString();
  UI.symbolsPerLine = s.value("symbolsperline", UI.symbolsPerLine.toVariant()).toInt();
  UI.userColorList = settings_read_list(s, "usercolorlist", UI.userColorList());
  UI.colorChooseWidgetRecent = settings_read_list(s, "colorchoosewidgetrecent", UI.colorChooseWidgetRecent());
  UI.colorChooseWidgetCustom = settings_read_list(s, "colorchoosewidgetcustom", UI.colorChooseWidgetCustom());
  UI.maxUserColors = s.value("maxusercolors", UI.maxUserColors.toVariant()).toInt();
  UI.enableToolTipPreview = s.value("enabletooltippreview", UI.enableToolTipPreview.toVariant()).toBool();
  UI.enableRealTimePreview = s.value("enablerealtimepreview", UI.enableRealTimePreview.toVariant()).toBool();
  UI.autosaveLibraryMin = s.value("autosavelibrarymin", UI.autosaveLibraryMin.toVariant()).toInt();
  s.endGroup();

  s.beginGroup("SyntaxHighlighter");
  uint configFlags = s.value("configflags", 0x5).toUInt();
  SyntaxHighlighter.enabled = (configFlags &  0x01);
  SyntaxHighlighter.highlightParensOnly = (configFlags & 0x02);
  SyntaxHighlighter.highlightLonelyParens = (configFlags & 0x04);
  SyntaxHighlighter.fmtKeyword = settings_read_QTextCharFormat(s, "keyword", SyntaxHighlighter.fmtKeyword());
  SyntaxHighlighter.fmtComment = settings_read_QTextCharFormat(s, "comment", SyntaxHighlighter.fmtComment());
  SyntaxHighlighter.fmtParenMatch = settings_read_QTextCharFormat(s, "parenmatch",
								  SyntaxHighlighter.fmtParenMatch());
  SyntaxHighlighter.fmtParenMismatch = settings_read_QTextCharFormat(s, "parenmismatch",
								     SyntaxHighlighter.fmtParenMismatch());
  SyntaxHighlighter.fmtLonelyParen = settings_read_QTextCharFormat(s, "lonelyparen",
								   SyntaxHighlighter.fmtLonelyParen());
  s.endGroup();

  s.beginGroup("BackendSettings");
  BackendSettings.tempDir = s.value("tempdir", BackendSettings.tempDir.toVariant()).toString();
  BackendSettings.execLatex = s.value("latexexec", BackendSettings.execLatex.toVariant()).toString();
  BackendSettings.execDvips = s.value("dvipsexec", BackendSettings.execDvips.toVariant()).toString();
  BackendSettings.execGs = s.value("gsexec", BackendSettings.execGs.toVariant()).toString();
  BackendSettings.execEpstopdf = s.value("epstopdfexec", BackendSettings.execEpstopdf.toVariant()).toString();
  BackendSettings.lborderoffset = s.value("lborderoffset", BackendSettings.lborderoffset.toVariant()).toInt();
  BackendSettings.tborderoffset = s.value("tborderoffset", BackendSettings.tborderoffset.toVariant()).toInt();
  BackendSettings.rborderoffset = s.value("rborderoffset", BackendSettings.rborderoffset.toVariant()).toInt();
  BackendSettings.bborderoffset = s.value("bborderoffset", BackendSettings.bborderoffset.toVariant()).toInt();
  s.endGroup();

  s.beginGroup("LibraryBrowser");
  LibraryBrowser.colorFound = s.value("colorfound", LibraryBrowser.colorFound.toVariant()).value<QColor>();
  LibraryBrowser.colorNotFound = s.value("colornotfound", LibraryBrowser.colorNotFound.toVariant()).value<QColor>();
  s.endGroup();

  // Special treatment for Plugins.pluginConfig
  s.beginGroup("Plugins/Config");
  QStringList pluginList = s.childGroups();
  s.endGroup();
  int j;
  for (j = 0; j < pluginList.size(); ++j) {
    QString name = pluginList[j];
    s.beginGroup( QString("Plugins/Config/%1").arg(name) );
    QMap<QString,QVariant> thispluginconfig;
    QStringList plconfkeys = s.childKeys();
    int k;
    for (k = 0; k < plconfkeys.size(); ++k) {
      thispluginconfig[plconfkeys[k]] = s.value(plconfkeys[k]);
    }
    klfconfig.Plugins.pluginConfig[name] = thispluginconfig;
    s.endGroup();
  }

  return 0;
}

