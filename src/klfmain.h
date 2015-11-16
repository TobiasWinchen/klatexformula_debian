/***************************************************************************
 *   file klfmain.h
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
/* $Id: klfmain.h 909 2014-08-10 17:58:33Z phfaist $ */

#ifndef KLFMAIN_H
#define KLFMAIN_H

#include <QCoreApplication>
#include <QString>
#include <QList>
#include <QMap>
#include <QStringList>
#include <QDataStream>
#include <QTranslator>
#include <QDir>

#include <klfdefs.h>




// SOME DEFINITIONS

#if defined(Q_WS_WIN)
#define KLF_DLL_EXT_LIST (QStringList()<<"*.dll")
#elif defined(Q_WS_MAC)
#define KLF_DLL_EXT_LIST (QStringList()<<"*.so"<<"*.dylib")
#else
#define KLF_DLL_EXT_LIST (QStringList()<<"*.so")
#endif


class KLFMainWin;
class QApplication;

// SOME DECLARATIONS FOR ADD-ONS

class KLF_EXPORT KLFAddOnInfo
{
public:
  /** Reads RCC file \c rccfpath and parses its rccinfo/info.xml, etc.
   * sets all fields to correct values and \ref isfresh to \c isFresh . */
  KLFAddOnInfo(QString rccfpath, bool isFresh = false);

  /** Create a copy of the add-on info structure \c other */
  KLFAddOnInfo(const KLFAddOnInfo& o);

  struct KLF_EXPORT PluginSysInfo {
    /** The directory in question */
    QString dir;
    /** minimum Qt version required to load the plugin, in version string format (eg. "4.4"). An
     * empty qtminversion disables version check. */
    QString qtminversion;
    /** minimum KLatexFormula version required to load the plugin, in version string format (eg.
     * "4.4"). An empty qtminversion disables version check. */
    QString klfminversion;
    /** The required value of \ref KLFSysInfo::osString() */
    QString os;
    /** The architecture(s) on which this plugin can run \ref KLFSysInfo::arch(). For
     * Mac OS X universal binaries this can be comma-separated list of architectures. */
    QString arch;

    bool isCompatibleWithCurrentSystem() const;
  };

  ~KLFAddOnInfo();

  /** Directory in which the RCC file resides */
  QString dir() const;
  /** Name of the RCC file (no path) */
  QString fname() const;
  /** in principle: <tt>absdir(dir()) + "/" + fname()</tt> */
  QString fpath() const;
  /** local file: can be removed (e.g. not in a global path /usr/share/... ) */
  bool islocal() const;

  /** the info in the add-on's info.xml file */
  QString title() const;
  /** the info in the add-on's info.xml file */
  QString author() const;
  /** the info in the add-on's info.xml file */
  QString description() const;
  /** the info in the add-on's info.xml file */
  QString klfminversion() const;

  //! where in the resource tree this rcc resource data is mounted
  QString rccmountroot() const;
    
  /** The list of plugins provided by this add-on (list of files
   * \c ":/plugins/[<dir>/]<plugin-name>*.so|dll").
   *
   * This list stores full file names relative to plugin dir in add-on (e.g. \c "libskin.so" or
   * \c "linux-x86-klf3.1.1/libskin.so") .
   *
   * See also \ref localPluginList().
   */
  QStringList pluginList() const;

  PluginSysInfo pluginSysInfo(const QString& plugin) const;

  QString pluginLocalSubDirName(const QString& plugin) const;

  /** A list of locally (ie. in <tt>~/.klatexformula/plugins/</tt>) installed plugins coming
   * from this add-on.
   *
   * The path is relative to <tt>~/.klatexformula/plugins/</tt>.
   *
   * \note Only plugins for the current os/architecture is returned. */
  QStringList localPluginList() const;


  /** The list of translation files provided by this add-on (list of files <tt>:/i18n/<b></b>*.qm</tt>)
   * This list stores full file names without the path (e.g. \c "klf_fr.qm") */
  QStringList translations() const;

  /** The list of user scripts provided by the add-on. User scripts are located at
   * <tt>:/userscripts/</tt>, and are installed to the main system (to be executable!).
   * This lists the filenames only without the directory name. */
  QStringList userScripts() const;

  /** Fresh file: add-on imported during this execution; ie. KLatexFormula needs to be restarted
   * for this add-on to take effect. The constructor sets this value to \c FALSE, set it manually
   * to \c TRUE if needed (e.g. in KLFSettings). */
  bool isfresh() const;

  /** List of errors that occurred with this add-on (now only used for reporting plugin load errors) */
  QStringList errors() const;


  /** \internal
   * */
  void addError(const QString& s);

private:

  class Private;
  Private *d;

  //  friend void main_load_plugins(QApplication *app, KLFMainWin *mainWin);

  void initPlugins();
};

KLF_EXPORT extern QList<KLFAddOnInfo> klf_addons;
KLF_EXPORT extern bool klf_addons_canimport;

KLF_EXPORT QDebug& operator<<(QDebug& str, const KLFAddOnInfo::PluginSysInfo& i);



// SOME DEFINITIONS FOR PLUGINS

class KLFPluginGenericInterface;

struct KLFPluginInfo
{
  QString name;
  QString author;
  QString title;
  QString description;

  QString fpath;
  QString fname;

  KLFPluginGenericInterface * instance;
};


KLF_EXPORT extern QList<KLFPluginInfo> klf_plugins;






/** \brief Small minimalist structure to store basic information about
 * available translations.
 *
 * Intended for settings dialog to read.
 *
 * To manage translation files, see \ref KLFI18nFile.
 */
struct KLF_EXPORT KLFTranslationInfo
{
  KLFTranslationInfo() : hasnicetranslatedname(false) { }

  QString localename;
  QString translatedname;
  /** \brief TRUE if the translatedname was provided by translator itself
   *
   * and FALSE if the name was guessed. */
  bool hasnicetranslatedname;
};

/** a list of locale names available for KLatexFormula */
KLF_EXPORT extern QList<KLFTranslationInfo> klf_avail_translations;

/** A list of instances of currently installed translators. */
KLF_EXPORT extern QList<QTranslator*> klf_translators;


/** \brief Small structure to store information for a translation file (.qm)
 *
 * Intented as (temporary) helper to manage translation files. Used e.g.
 * in \ref klf_reload_translations().
 *
 * To see a list of available translations accessible within the whole
 * program, see \ref KLFTranslationInfo and \ref klf_avail_translations.
 */
class KLF_EXPORT KLFI18nFile
{
public:
  QString fpath;
  /** \brief Translation file base name (e.g. 'klf' for klf_fr.qm) */
  QString name;
  /** \brief Locale Name (e.g. "fr" or "fr_CH") */
  QString locale;
  /** \brief  how specific the locale is (e.g. ""->0 , "fr"->1, "fr_CH"->2 ) */
  int locale_specificity;

  /** Initialize this structure to the translation file \c filepath. */
  KLFI18nFile(QString filepath);
};



KLF_EXPORT void klf_add_avail_translation(KLFI18nFile i18nfile);


/** Call this at startup or upon language change */
KLF_EXPORT void klf_reload_translations(QCoreApplication *app, const QString& currentlocale);



// eg. baseFileName="cmdl-help"  extension=".txt" will look for
// "cmdl-help_fr_CH.txt", "cmdl-help_fr.txt", "cmdl-help.txt" assuming current locale is "fr_CH"
KLF_EXPORT QString klfFindTranslatedDataFile(const QString& baseFileName, const QString& extension);




/** \brief Current datastream compatibility klatexformula version
 *
 * This value is updated to the new version of KLF whenever a change in the format of the
 * QDataStream's occur.
 *
 * This is used notably in local styles list and symbols cache.
 *
 * \note This does NOT affect legacy \c ".klf" library files
 */
#define KLF_DATA_STREAM_APP_VERSION  "3.3"
//                                --- don't forget to update below too! ---

/** \brief 'Major' version part of \ref KLF_DATA_STREAM_APP_VERSION. */
#define KLF_DATA_STREAM_APP_VERSION_MAJ  3
/** \brief 'Minor' version part of \ref KLF_DATA_STREAM_APP_VERSION. */
#define KLF_DATA_STREAM_APP_VERSION_MIN  3


/** \brief Obtain the KLF version stream operations on \c d have to be compatible with.
 *
 * \returns the KLatexFormula verison as a QString, eg. \c "2.1"
 */
inline QString klfDataStreamAppVersion(const QDataStream& d)
{ return d.device()->property("klfDataStreamAppVersion").toString(); }


/** This function sets up the stream for writing data (for internal storage, eg.
 * styles list)
 *
 * This function will set the \c klfDataStreamAppVersion property on the QIODevice associated
 * with the given \c stream, in order that objects that are sent into the stream can know their
 * compatibility version by querying the stream with klfDataStreamAppVersion().
 */
KLF_EXPORT void klfDataStreamWriteHeader(QDataStream& stream, const QString headermagic);

/** Reads a stream in which a header was prepared with klfDataStreamWriteHeader().
 */
KLF_EXPORT bool klfDataStreamReadHeader(QDataStream& stream, const QStringList possibleHeaders,
					QString * readHeader = NULL, QString * readCompatKLFVersion = NULL) ;


// -- 
// user scripts

KLF_EXPORT void klf_reload_user_scripts();
extern QStringList klf_user_scripts;



#endif 
