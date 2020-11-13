/***************************************************************************
 *   file klfmain.cpp
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
/* $Id$ */

#include <QDebug>
#include <QString>
#include <QList>
#include <QObject>
#include <QDomDocument>
#include <QFile>
#include <QFileInfo>
#include <QResource>
#include <QDir>
#include <QTranslator>
#include <QLibraryInfo>
#include <QDateTime>

#include <klfutil.h>
#include <klfsysinfo.h>
#include <klfuserscript.h>
#include "klfconfig.h"
#include "klfmain.h"


KLF_EXPORT QList<KLFTranslationInfo> klf_avail_translations;

KLF_EXPORT QList<QTranslator*> klf_translators;


// -------




KLFI18nFile::KLFI18nFile(QString filepath)
{
  QFileInfo fi(filepath);
  QString fn = fi.fileName();
  QDir d = fi.absoluteDir();

  int firstunderscore = fn.indexOf('_');
  int endbasename = fn.endsWith(".qm") ? fn.length() - 3 : fn.length() ;
  if (firstunderscore == -1)
    firstunderscore = endbasename; // no locale part if no underscore
  // ---
  fpath = d.absoluteFilePath(fn);
  name = fn.mid(0, firstunderscore);
  locale = fn.mid(firstunderscore+1, endbasename-(firstunderscore+1));
  locale_specificity = (locale.split('_', QString::SkipEmptyParts)).size() ;
}




void klf_add_avail_translation(KLFI18nFile i18nfile)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("i18nfile.fpath="<<i18nfile.fpath<<" is translation to "<<i18nfile.locale) ;

  QFileInfo fi(i18nfile.fpath);

  klfDbg("fi.canonicalPath()="<<fi.canonicalPath()<<", Qt translations location="
	 <<QFileInfo(QLibraryInfo::location(QLibraryInfo::TranslationsPath)).canonicalFilePath()) ;

  if ( fi.canonicalPath() ==
       QFileInfo(QLibraryInfo::location(QLibraryInfo::TranslationsPath)).canonicalFilePath()
       || i18nfile.name == "qt" ) {
    // ignore Qt's translations as available languages (identified as being in Qt's
    // translation path or as a locally named qt_XX.qm
    return;
  }

  // Check if this locale is registered, and if it has a nice translated name.
  bool needsRegistration = true;
  bool needsNiceName = true;
  int alreadyRegisteredIndex = -1;

  int kk;
  for (kk = 0; kk < klf_avail_translations.size(); ++kk) {
    if (klf_avail_translations[kk].localename == i18nfile.locale) {
      needsRegistration = false;
      alreadyRegisteredIndex = kk;
      needsNiceName = ! klf_avail_translations[kk].hasnicetranslatedname;
      klfDbg("translation "<<i18nfile.locale<<" is already registered. needs nice name?="<< needsNiceName) ;
    }
  }

  klfDbg("Needs registration?="<<needsRegistration<<"; needs nice name?="<<needsNiceName) ;
  if ( ! needsRegistration && ! needsNiceName ) {
    // needs nothing more !
    return;
  }

  klfDbg("will load translation file "<<fi.completeBaseName()<<", abs path="<<fi.absolutePath()) ;

  // needs something (registration and/or nice name)
  QTranslator translator;
  translator.load(fi.completeBaseName(), fi.absolutePath(), "_", "."+fi.suffix());
  KLFTranslationInfo ti;
  ti.localename = i18nfile.locale;
  struct klf_qtTrNoop3 { const char *source; const char *comment; };
  klf_qtTrNoop3 lang
    = QT_TRANSLATE_NOOP3("QObject", "English (US)",
			 "[[The Language (possibly with Country) you are translating to, e.g. `Deutsch']]");
  ti.translatedname = translator.translate("QObject", lang.source, lang.comment);
  ti.hasnicetranslatedname = true;
  if (ti.translatedname == "English" || ti.translatedname.isEmpty()) {
    QLocale lc(i18nfile.locale);
    QString s;
    if ( i18nfile.locale.indexOf("_") != -1 ) {
      // has country information in locale
      s = QString("%1 (%2)").arg(QLocale::languageToString(lc.language()))
	.arg(QLocale::countryToString(lc.country()));
    } else {
      s = QString("%1").arg(QLocale::languageToString(lc.language()));
    }
    ti.translatedname = s;
    ti.hasnicetranslatedname = false;
  }
  if (needsRegistration)
    klf_avail_translations.append(ti);
  else if (needsNiceName && ti.hasnicetranslatedname)
    klf_avail_translations[alreadyRegisteredIndex] = ti;
}


KLF_EXPORT void klf_reload_translations(QCoreApplication *app, const QString& currentLocale)
{
  // refresh and load all translations. Translations are files found in the form
  //   :/i18n/<name>_<locale>.qm   or   homeconfig/i18n/<name>_<locale>.qm
  //
  // this function may be called at run-time to change language.

  int j, k;

  // clear any already set translators
  for (k = 0; k < klf_translators.size(); ++k) {
    app->removeTranslator(klf_translators[k]);
    delete klf_translators[k];
  }
  klf_translators.clear();

  // we will find all possible .qm files and store them in this structure for easy access
  // structure is indexed by name, then locale specificity
  QMap<QString, QMap<int, QList<KLFI18nFile> > > i18nFiles;
  // a list of names. this is redundant for  i18nFiles.keys()
  QSet<QString> names;

  QStringList i18ndirlist;
  // add any add-on specific translations
//  for (k = 0; k < klf_addons.size(); ++k) {
//    i18ndirlist << klf_addons[k].rccmountroot()+"/i18n";
//  }
  i18ndirlist << ":/i18n"
	      << klfconfig.homeConfigDirI18n
	      << klfconfig.globalShareDir+"/i18n"
	      << QLibraryInfo::location(QLibraryInfo::TranslationsPath);

  for (j = 0; j < i18ndirlist.size(); ++j) {
    // explore this directory; we expect a list of *.qm files
    QDir i18ndir(i18ndirlist[j]);
    if ( ! i18ndir.exists() )
      continue;
    QStringList files = i18ndir.entryList(QStringList() << QString::fromLatin1("*.qm"), QDir::Files);
    for (k = 0; k < files.size(); ++k) {
      KLFI18nFile i18nfile(i18ndir.absoluteFilePath(files[k]));
      //      qDebug("Found i18n file %s (name=%s,locale=%s,lc-spcif.=%d)", qPrintable(i18nfile.fpath),
      //	     qPrintable(i18nfile.name), qPrintable(i18nfile.locale), i18nfile.locale_specificity);
      i18nFiles[i18nfile.name][i18nfile.locale_specificity] << i18nfile;
      names << i18nfile.name;
      klfDbg("Found translation "<<i18nfile.fpath);
      klf_add_avail_translation(i18nfile);
    }
  }

  // get locale
  QString lc = currentLocale;
  if (lc.isEmpty())
    lc = "en_US";
  QStringList lcparts = lc.split("_");

  //  qDebug("Required locale is %s", qPrintable(lc));

  // a list of translation files to load (absolute paths)
  QStringList translationsToLoad;

  // now, load a suitable translator for each encountered name.
  for (QSet<QString>::const_iterator it = names.begin(); it != names.end(); ++it) {
    QString name = *it;
    QMap< int, QList<KLFI18nFile> > translations = i18nFiles[name];
    int specificity = lcparts.size();  // start with maximum specificity for required locale
    while (specificity >= 0) {
      // try to load a translation matching this specificity and locale
      QString testlocale = QStringList(lcparts.mid(0, specificity)).join("_");
      //      qDebug("Testing locale string %s...", qPrintable(testlocale));
      // search list:
      QList<KLFI18nFile> list = translations[specificity];
      for (j = 0; j < list.size(); ++j) {
	if (list[j].locale == testlocale) {
	  //	  qDebug("Found translation file.");
	  // got matching translation file ! Load it !
	  translationsToLoad << list[j].fpath;
	  // and stop searching translation files for this name (break while condition);
	  specificity = -1;
	}
      }
      // If we didn't find a suitable translation, try less specific locale name
      specificity--;
    }
  }
  // now we have a full list of translation files to load stored in  translationsToLoad .

  // Load Translations:
  for (j = 0; j < translationsToLoad.size(); ++j) {
    // load this translator
    //    qDebug("Loading translator %s for %s", qPrintable(translationsToLoad[j]), qPrintable(lc));
    QTranslator *translator = new QTranslator(app);
    QFileInfo fi(translationsToLoad[j]);
    //    qDebug("translator->load(\"%s\", \"%s\", \"_\", \"%s\")", qPrintable(fi.completeBaseName()),
    //	   qPrintable(fi.absolutePath()),  qPrintable("."+fi.suffix()));
    bool res = translator->load(fi.completeBaseName(), fi.absolutePath(), "_", "."+fi.suffix());
    if ( res ) {
      app->installTranslator(translator);
      klf_translators << translator;
    } else {
      qWarning("Failed to load translator %s.", qPrintable(translationsToLoad[j]));
      delete translator;
    }
  }
}




KLF_EXPORT QString klfFindTranslatedDataFile(const QString& baseFileName, const QString& extension)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  QString loc = klfconfig.UI.locale;
  QStringList suffixes;
  suffixes << "_"+loc
	   << "_"+loc.section('_',0,0)
	   << "";

  QStringList basedatadirs = QStringList()
    << klfconfig.homeConfigDir + "/data/"
    << klfconfig.globalShareDir + "/data/"
    << ":/data/";

  foreach (QString suffix, suffixes) {
    klfDbg("trying suffix=" << suffix) ;
    foreach (QString basedatadir, basedatadirs) {
      QString fn = basedatadir + baseFileName + suffix + extension;
      klfDbg("trying basedatadir=" << basedatadir << ": fn=" << fn) ;
      if (QFile::exists(fn)) {
        return fn;
      }
    }
  }

  klfWarning("Can't find neither translated nor original file for " << baseFileName+extension << " ! ") ;
  return QString();
}




KLF_EXPORT void klfDataStreamWriteHeader(QDataStream& stream, const QString headermagic)
{
  // QIODevice inherits QObject ... use dynamic properties
  stream.device()->setProperty("klfDataStreamAppVersion",
			       QVariant::fromValue<QString>(KLF_DATA_STREAM_APP_VERSION));

  // header always written in QDataStream version Qt_3_3
  stream.setVersion(QDataStream::Qt_3_3);
  stream << headermagic
	 << (qint16)KLF_DATA_STREAM_APP_VERSION_MAJ
	 << (qint16)KLF_DATA_STREAM_APP_VERSION_MIN
	 << (qint16)QDataStream::Qt_4_4;
  stream.setVersion(QDataStream::Qt_4_4);
  // stream is ready to be written to

}

KLF_EXPORT bool klfDataStreamReadHeader(QDataStream& stream, const QStringList possibleHeaders,
					QString *readHeader, QString *readCompatKLFVersion)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  QString s;
  stream.setVersion(QDataStream::Qt_3_3);
  stream >> s;
  if (!possibleHeaders.contains(s) || stream.status() != QDataStream::Ok) {
    klfDbg("Read bad header: "<<s) ;
    if (readHeader != NULL)
      *readHeader = QString();
    return false;
  }
  if (readHeader != NULL)
    *readHeader = s;

  // read KLF-compat writing version
  qint16 vmaj, vmin;
  stream >> vmaj >> vmin;
  if (stream.status() != QDataStream::Ok) {
    if (readCompatKLFVersion)
      *readCompatKLFVersion = QString();
    return false;
  }
  klfDbg("read app compat version = "<<vmaj<<"."<<vmin) ;

  QString compatKLFVersion = QString("%1.%2").arg(vmaj).arg(vmin);

  if (vmaj > klfVersionMaj() || (vmaj == klfVersionMaj() && vmin > klfVersionMin())) {
    if (readCompatKLFVersion != NULL)
      *readCompatKLFVersion = compatKLFVersion; 
    return false;
  }

  // decide on QDataStream version
  if (vmaj <= 2) { // 2.x: version # not saved into stream, use Qt_3_3
    stream.setVersion(QDataStream::Qt_3_3);
  } else { // 3.x+: read version # from stream and set it
    qint16 version;
    stream >> version;
    stream.setVersion(version);
  }

  // set the compatibility version for reading data
  // QIODevice inherits QObject ... use dynamic properties
  stream.device()->setProperty("klfDataStreamAppVersion", QVariant::fromValue<QString>(compatKLFVersion));

  // the stream is ready to read data from
  return true;
}



// -----

// user scripts

KLF_EXPORT QStringList klf_user_scripts;


void klf_reload_user_scripts()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  QStringList pathlist;
  pathlist << klfconfig.homeConfigDirUserScripts
	   << klfconfig.globalShareDir+"/userscripts"
	   << klfconfig.BackendSettings.userScriptAddPath;

  //  int i, j;
  int k;

  // replace ~/ by $HOME
  for (k = 0; k < pathlist.size(); ++k) {
    if (pathlist[k].startsWith("~/"))
      pathlist[k] = QDir::homePath() + pathlist[k].mid(1);
  }

  klfDbg("looking for user scripts in "<<pathlist) ;

  // First, look to see if there are any user scripts to install from add-on resources

  // for (k = 0; k < klf_addons.size(); ++k) {
  //   QStringList uscripts = klf_addons[k].userScripts();
  //   for (j = 0; j < uscripts.size(); ++j) {
  //     // maybe install this user script
  //     // so check at filesystem locations to see if it's there
  //     QString foundpath = QString();
  //     QString resfn = klf_addons[k].rccmountroot() + "/userscripts/" + uscripts[j];
  //     QString locfn = klfconfig.homeConfigDirUserScripts + "/" + uscripts[j];
  //     for (i = 0; i < pathlist.size(); ++i) {
  //       QString s = pathlist[i]+"/"+uscripts[j];
  //       klfDbg("testing "<<s) ;
  //       if (QFile::exists(s)) { // found
  //         foundpath = s;
  //         break;
  //       }
  //     }
  //     bool needsinstall = false;
  //     if (foundpath.isEmpty()) {
  //       needsinstall = true;
  //     } else {
  //       // compare modification times to see whether we need to reupdate userscript
  //       QDateTime installeduserscript_dt = QFileInfo(foundpath).lastModified();
  //       QDateTime resourceuserscript_dt = QFileInfo(klf_addons[k].fpath()).lastModified();
  //       klfDbg("Comparing resource datetime ("<<qPrintable(resourceuserscript_dt.toString())
  //              <<") with installed userscript datetime ("<<qPrintable(installeduserscript_dt.toString())<<")") ;
  //       needsinstall = (installeduserscript_dt.isNull() || resourceuserscript_dt.isNull() ||
  //       		( resourceuserscript_dt > installeduserscript_dt ));
  //     }
  //     if (!needsinstall)
  //       continue;
  //     // now install that user script
  //     if (QFile::exists(locfn)) QFile::remove(locfn);
  //     // copy userscript to local userscript dir
  //     klfDbg( "\tcopy "<<resfn<<" to "<<locfn ) ;
  //     bool res = QFile::copy( resfn , locfn );
  //     if ( ! res ) {
  //       klf_addons[k].addError(QObject::tr("Failed to install userscript '%1' locally.",
  //       				   "[[userscript error message]]").arg(uscripts[j]));
  //       qWarning("Unable to copy plugin '%s' to local directory!", qPrintable(uscripts[j]));
  //       continue;
  //     } else {
  //       QFile::setPermissions(locfn, QFile::ReadOwner|QFile::WriteOwner|QFile::ExeOwner|
  //       		      QFile::ReadUser|QFile::WriteUser|QFile::ExeUser|
  //       		      QFile::ReadGroup|QFile::ExeGroup|QFile::ReadOther|QFile::ExeOther);
  //       klfDbg("Copied userscript "<<resfn<<" to local directory "<<locfn<<".") ;
  //     }

  //     if (!foundpath.isEmpty() && !needsinstall)
  //       locfn = foundpath;
  //   } // loop over add-on's userscripts
  // } // loop over add-ons

  // now find all user scripts

  KLFUserScriptInfo::clearCacheAll();

  klf_user_scripts.clear();
  for (int kkl = 0; kkl < pathlist.size(); ++kkl) {

    QFileInfoList flist = QDir(pathlist[kkl]).entryInfoList(QStringList()<<"*.klfuserscript", QDir::Dirs);
    // filter out some unwanted entries
    for (int j = 0; j < flist.size(); ++j) {
      QFileInfo fi = flist[j];
      QString fn = fi.fileName();
      if (!fn.endsWith(".klfuserscript")) {
        // should not happen, we already set a name filter in .entryInfoList(...)
        klfDbg("Ignoring item in user script directory (not *.klfuserscript): " << fn) ;
        continue;
      }
      klfDbg("User script: "<<fn) ;
      klf_user_scripts << fi.canonicalFilePath();
    }
  }
  klfDbg("Searched in path="<<pathlist<<"; scripts="<<klf_user_scripts) ;
}

