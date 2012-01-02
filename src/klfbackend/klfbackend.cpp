/***************************************************************************
 *   file klfbackend.cpp
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
/* $Id: klfbackend.cpp 748 2012-01-01 15:06:40Z phfaist $ */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include <qapplication.h>
#include <qregexp.h>
#include <qfile.h>
#include <qdatetime.h>
#include <qtextstream.h>
#include <qbuffer.h>
#include <qdir.h>


#include "klfblockprocess.h"
#include "klfbackend.h"

// write Qt 3/4 compatible code
#include "klfqt34common.h"


/** \mainpage
 *
 * <div style="width: 60%; padding: 0 20%; text-align: justify; line-height: 150%">
 * This documentation is the API documentation for the KLatexFormula library backend that
 * you may want to use in your programs. It is a GPL-licensed library based on QT 3 or QT 4 that
 * converts a LaTeX equation given as text into graphics, specifically PNG, EPS or PDF (and the
 * image is available as a QImage&mdash;so any format supported by Qt is available.
 *
 * Some utilities to save the output (in various formats) to a file or a QIODevice
 * are provided, see KLFBackend::saveOutputToFile() and KLFBackend::saveOutputToDevice().
 *
 * All the core functionality is based in the class \ref KLFBackend . Some extra general utilities are
 * available in \ref klfdefs.h , such as klfVersionCompare(), \ref KLFSysInfo, \ref klfDbg,
 * klfFmt(), klfSearchFind(), etc.
 *
 * This library will compile indifferently on QT 3 and QT 4 with the same source code.
 * The base API is the same, although some specific overloaded functions may differ or not be available
 * in either version for Qt 3 or Qt 4. Those members are documented as such.
 *
 * To compile with Qt 4, you may add \c KLFBACKEND_QT4 to your defines, ie. pass option \c -DKLFBACKEND_QT4
 * to gcc; however this is set automatically in \ref klfdefs.h if Qt 4 is detected.
 *
 * This library has been tested to work in non-GUI applications (ie. FALSE in QApplication constructor,
 * or with QCoreApplication in Qt 4).
 *</div>
 */




// some standard guess settings for system configurations

#ifdef KLF_EXTRA_SEARCH_PATHS
#  define EXTRA_PATHS_PRE	KLF_EXTRA_SEARCH_PATHS ,
#  define EXTRA_PATHS		KLF_EXTRA_SEARCH_PATHS
#else
#  define EXTRA_PATHS_PRE
#  define EXTRA_PATHS
#endif


#if defined(Q_OS_WIN32) || defined(Q_OS_WIN64)
QStringList progLATEX = QStringList() << "latex.exe";
QStringList progDVIPS = QStringList() << "dvips.exe";
QStringList progGS = QStringList() << "gswin32c.exe" << "mgs.exe";
QStringList progEPSTOPDF = QStringList() << "epstopdf.exe";
static const char * standard_extra_paths[] = {
  EXTRA_PATHS_PRE
  "C:\\Program Files\\MiKTeX*\\miktex\\bin",
  "C:\\Program Files\\gs\\gs*\\bin",
  NULL
};
#elif defined(Q_WS_MAC)
QStringList progLATEX = QStringList() << "latex";
QStringList progDVIPS = QStringList() << "dvips";
QStringList progGS = QStringList() << "gs";
QStringList progEPSTOPDF = QStringList() << "epstopdf";
static const char * standard_extra_paths[] = {
  EXTRA_PATHS_PRE
  "/usr/texbin:/usr/local/bin:/sw/bin:/sw/usr/bin",
  NULL
};
#else
QStringList progLATEX = QStringList() << "latex";
QStringList progDVIPS = QStringList() << "dvips";
QStringList progGS = QStringList() << "gs";
QStringList progEPSTOPDF = QStringList() << "epstopdf";
static const char * standard_extra_paths[] = {
  EXTRA_PATHS_PRE
  NULL
};
#endif



// ---------------------------------

KLFBackend::KLFBackend()
{
}


// Utility function
QString progErrorMsg(QString progname, int exitstatus, QString stderrstr, QString stdoutstr)
{
  QString stdouthtml = stdoutstr;
  QString stderrhtml = stderrstr;
  stdouthtml.replace("&", "&amp;");
  stdouthtml.replace("<", "&lt;");
  stdouthtml.replace(">", "&gt;");
  stderrhtml.replace("&", "&amp;");
  stderrhtml.replace("<", "&lt;");
  stderrhtml.replace(">", "&gt;");

  if (stderrstr.isEmpty() && stdoutstr.isEmpty())
    return QObject::tr("<p><b>%1</b> reported an error (exit status %2). No Output was generated.</p>",
		       "KLFBackend")
	.arg(progname).arg(exitstatus);
  if (stderrstr.isEmpty())
    return
      QObject::tr("<p><b>%1</b> reported an error (exit status %2). Here is full stdout output:</p>\n"
		  "<pre>\n%3</pre>", "KLFBackend")
      .arg(progname).arg(exitstatus).arg(stdouthtml);
  if (stdoutstr.isEmpty())
    return
      QObject::tr("<p><b>%1</b> reported an error (exit status %2). Here is full stderr output:</p>\n"
		  "<pre>\n%3</pre>", "KLFBackend")
      .arg(progname).arg(exitstatus).arg(stderrhtml);
  
  return QObject::tr("<p><b>%1</b> reported an error (exit status %2). Here is full stderr output:</p>\n"
		     "<pre>\n%3</pre><p>And here is full stdout output:</p><pre>\n%4</pre>", "KLFBackend")
    .arg(progname).arg(exitstatus).arg(stderrhtml).arg(stdouthtml);
}


/* * Internal.
 * \internal */
struct cleanup_caller {
  QString tempfname;
  cleanup_caller(QString fn) : tempfname(fn) { }
  ~cleanup_caller() {
    KLFBackend::cleanup(tempfname);
  }
};

QString __klf_expand_env_vars(const QString& envexpr)
{
  QString s = envexpr;
  QRegExp rx("\\$(?:(\\$|(?:[A-Za-z0-9_]+))|\\{([A-Za-z0-9_]+)\\})");
  int i = 0;
  while ( (i = rx.rx_indexin_i(s, i)) != -1 ) {
    // match found, replace it
    QString envvarname = rx.cap(1);
    if (envvarname.isEmpty() || envvarname == QLatin1String("$")) {
      // note: empty variable name expands to a literal '$'
      s.replace(i, rx.matchedLength(), QLatin1String("$"));
      i += 1;
      continue;
    }
    const char *svalue = getenv(qPrintable(envvarname));
    QString qsvalue = (svalue != NULL) ? QString::fromLocal8Bit(svalue) : QString();
    s.replace(i, rx.matchedLength(), qsvalue);
    i += qsvalue.length();
  }
  // replacements performed
  return s;
}

void __klf_append_replace_env_var(QStringList *list, const QString& var, const QString& line)
{
  // search for declaration of var in list
  int k;
  for (k = 0; k < (int)list->size(); ++k) {
    if (list->operator[](k).startsWith(var+QString("="))) {
      list->operator[](k) = line;
      return;
    }
  }
  // declaration not found, just append
  list->append(line);
}



KLFBackend::klfOutput KLFBackend::getLatexFormula(const klfInput& in, const klfSettings& settings)
{
  // ALLOW ONLY ONE RUNNING getLatexFormula() AT A TIME 
  QMutexLocker mutexlocker(&__mutex);

  int k;

  qDebug("%s: %s: KLFBackend::getLatexFormula() called. latex=%s", KLF_FUNC_NAME, KLF_SHORT_TIME,
	 qPrintable(in.latex));

  // get full, expanded exec environment
  QStringList execenv = klf_cur_environ();
  for (k = 0; k < (int)settings.execenv.size(); ++k) {
    int eqpos = settings.execenv[k].s_indexOf(QChar('='));
    if (eqpos == -1) {
      qWarning("%s: badly formed environment definition in `environ': %s", KLF_FUNC_NAME,
	       qPrintable(settings.execenv[k]));
      continue;
    }
    QString varname = settings.execenv[k].mid(0, eqpos);
    QString newenvdef = __klf_expand_env_vars(settings.execenv[k]);
    __klf_append_replace_env_var(&execenv, varname, newenvdef);
  }

  klfDbg("execution environment for sub-processes:\n"+execenv.join("\n")) ;
  
  klfOutput res;
  res.status = KLFERR_NOERROR;
  res.errorstr = QString();
  res.result = QImage();
  res.pngdata_raw = QByteArray();
  res.pngdata = QByteArray();
  res.epsdata = QByteArray();
  res.pdfdata = QByteArray();
  res.input = in;
  res.settings = settings;

  // PROCEDURE:
  // - generate LaTeX-file
  // - latex --> get DVI file
  // - dvips -E file.dvi it to get an EPS file
  // - expand BBox by editing EPS file (if applicable)
  // - outline fonts with gs (if applicable)
  // - Run gs:	gs -dNOPAUSE -dSAFER -dEPSCrop -r600 -dTextAlphaBits=4 -dGraphicsAlphaBits=4
  //               -sDEVICE=pngalpha|png16m -sOutputFile=xxxxxx.png -q -dBATCH xxxxxx.eps
  //   to eventually get PNG data
  // - if epstopdfexec is not empty, run epstopdf and get PDF file.

  QString tempfname = settings.tempdir + "/klatexformulatmp" KLF_VERSION_STRING "-"
    + QDateTime::currentDateTime().toString("hh-mm-ss");

  QString fnTex = tempfname + ".tex";
  QString fnDvi = tempfname + ".dvi";
  QString fnRawEps = tempfname + "-raw.eps";
  QString fnBBCorrEps = tempfname + "-bbcorr.eps";
  QString fnOutlFontsEps = tempfname + "-outlfonts.eps";
  QString fnFinalEps = settings.outlineFonts ? fnOutlFontsEps : fnBBCorrEps;
  QString fnPng = tempfname + ".png";
  QString fnPdf = tempfname + ".pdf";

  // upon destruction (on stack) of this object, cleanup() will be
  // automatically called as wanted
  cleanup_caller cleanupcallerinstance(tempfname);

#ifdef KLFBACKEND_QT4
  QString latexsimplified = in.latex.s_trimmed();
#else
  QString latexsimplified = in.latex.stripWhiteSpace();
#endif
  if (latexsimplified.isEmpty()) {
    res.errorstr = QObject::tr("You must specify a LaTeX formula!", "KLFBackend");
    res.status = KLFERR_MISSINGLATEXFORMULA;
    return res;
  }

  QString latexin;
  if (!in.bypassTemplate) {
    if (in.mathmode.contains("...") == 0) {
      res.status = KLFERR_MISSINGMATHMODETHREEDOTS;
      res.errorstr = QObject::tr("The math mode string doesn't contain '...'!", "KLFBackend");
      return res;
    }
    latexin = in.mathmode;
    latexin.replace("...", in.latex);
  }

  {
    QFile file(fnTex);
    bool r = file.open(dev_WRITEONLY);
    if ( ! r ) {
      res.status = KLFERR_TEXWRITEFAIL;
      res.errorstr = QObject::tr("Can't open file for writing: '%1'!", "KLFBackend")
	.arg(fnTex);
      return res;
    }
    QTextStream stream(&file);
    if (!in.bypassTemplate) {
      stream << "\\documentclass{article}\n"
	     << "\\usepackage[dvips]{color}\n"
	     << in.preamble << "\n"
	     << "\\begin{document}\n"
	     << "\\thispagestyle{empty}\n"
	     << QString("\\definecolor{klffgcolor}{rgb}{%1,%2,%3}\n").arg(qRed(in.fg_color)/255.0)
	.arg(qGreen(in.fg_color)/255.0).arg(qBlue(in.fg_color)/255.0)
	     << QString("\\definecolor{klfbgcolor}{rgb}{%1,%2,%3}\n").arg(qRed(in.bg_color)/255.0)
	.arg(qGreen(in.bg_color)/255.0).arg(qBlue(in.bg_color)/255.0)
	     << ( (qAlpha(in.bg_color)>0) ? "\\pagecolor{klfbgcolor}\n" : "" )
	     << "{\\color{klffgcolor} " << latexin << " }\n"
	     << "\\end{document}\n";
    } else {
      stream << in.latex;
    }
  }

  { // execute latex

    KLFBlockProcess proc;
    QStringList args;

    proc.setWorkingDirectory(settings.tempdir);

    args << settings.latexexec << dir_native_separators(fnTex);

    qDebug("%s: %s:  about to exec latex...", KLF_FUNC_NAME, KLF_SHORT_TIME) ;
    bool r = proc.startProcess(args, execenv);
    qDebug("%s: %s:  latex returned.", KLF_FUNC_NAME, KLF_SHORT_TIME) ;

    if (!r) {
      res.status = KLFERR_NOLATEXPROG;
      res.errorstr = QObject::tr("Unable to start Latex program %1!", "KLFBackend")
	.arg(settings.latexexec);
      return res;
    }
    if (!proc.processNormalExit()) {
      res.status = KLFERR_LATEXNONORMALEXIT;
      res.errorstr = QObject::tr("Latex was killed!", "KLFBackend");
      return res;
    }
    if (proc.processExitStatus() != 0) {
      res.status = KLFERR_PROGERR_LATEX;
      res.errorstr = progErrorMsg("LaTeX", proc.processExitStatus(), proc.readStderrString(),
				  proc.readStdoutString());
      return res;
    }

    if (!QFile::exists(fnDvi)) {
      res.status = KLFERR_NODVIFILE;
      res.errorstr = QObject::tr("DVI file didn't appear after having called Latex!", "KLFBackend");
      return res;
    }

  } // end of 'latex' block

  { // execute dvips -E

    KLFBlockProcess proc;
    QStringList args;
    args << settings.dvipsexec << "-E" << dir_native_separators(fnDvi)
         << "-o" << dir_native_separators(fnRawEps);

    qDebug("%s: %s:  about to dvips... %s", KLF_FUNC_NAME, KLF_SHORT_TIME, qPrintable(args.join(" "))) ;
    bool r = proc.startProcess(args, execenv);
    qDebug("%s: %s:  dvips returned.", KLF_FUNC_NAME, KLF_SHORT_TIME) ;

    if ( ! r ) {
      res.status = KLFERR_NODVIPSPROG;
      res.errorstr = QObject::tr("Unable to start dvips!\n", "KLFBackend");
      return res;
    }
    if ( !proc.processNormalExit() ) {
      res.status = KLFERR_DVIPSNONORMALEXIT;
      res.errorstr = QObject::tr("Dvips was mercilessly killed!\n", "KLFBackend");
      return res;
    }
    if ( proc.processExitStatus() != 0) {
      res.status = KLFERR_PROGERR_DVIPS;
      res.errorstr = progErrorMsg("dvips", proc.processExitStatus(), proc.readStderrString(),
				  proc.readStdoutString());
      return res;
    }
    if (!QFile::exists(fnRawEps)) {
      res.status = KLFERR_NOEPSFILE;
      res.errorstr = QObject::tr("EPS file didn't appear after dvips call!\n", "KLFBackend");
      return res;
    }

    // add some space on bounding-box to avoid some too tight bounding box bugs
    // read eps file
    QFile epsfile(fnRawEps);
    r = epsfile.open(dev_READONLY);
    if ( ! r ) {
      res.status = KLFERR_EPSREADFAIL;
      res.errorstr = QObject::tr("Can't read file '%1'!\n", "KLFBackend").arg(fnRawEps);
      return res;
    }
    /** \todo Hi-Res bounding box adjustment. Shouldn't be too hard to do, but needs tests to see
     * how this works... [ Currently: only integer-valued BoundingBox: is adjusted. ] */
    QByteArray epscontent = epsfile.readAll();
#ifdef KLFBACKEND_QT4
    QByteArray epscontent_s = epscontent;
    int i = epscontent_s.indexOf("%%BoundingBox: ");
#else
    QCString epscontent_s(epscontent.data(), epscontent.size());
    int i = epscontent_s.find("%%BoundingBox: ");
#endif
    // process file data and transform it
    if ( i == -1 ) {
      res.status = KLFERR_NOEPSBBOX;
      res.errorstr = QObject::tr("File '%1' does not contain line \"%%BoundingBox: ... \" !",
				 "KLFBackend").arg(fnRawEps);
      return res;
    }
    int ax, ay, bx, by;
    char temp[250];
    const int k = i;
    i += strlen("%%BoundingBox:");
    int n = sscanf(epscontent_s.data()+i, "%d %d %d %d", &ax, &ay, &bx, &by);
    if ( n != 4 ) {
      res.status = KLFERR_BADEPSBBOX;
      res.errorstr = QObject::tr("file %1: Line %%BoundingBox: can't read values!\n", "KLFBackend")
	.arg(fnRawEps);
      return res;
    }
    // grow bbox by settings.Xborderoffset points
    // Don't forget: '%' in printf has special meaning (!) -> double percent signs '%'->'%%'
    sprintf(temp, "%%%%BoundingBox: %d %d %d %d",
	    (int)(ax-settings.lborderoffset+0.5),
	    (int)(ay-settings.bborderoffset+0.5),
	    (int)(bx+settings.rborderoffset+0.5),
	    (int)(by+settings.tborderoffset+0.5));
    QString chunk = QString::fromLocal8Bit(epscontent_s.data()+k);
    QRegExp rx("^%%BoundingBox: [0-9]+ [0-9]+ [0-9]+ [0-9]+");
    rx.rx_indexin(chunk);
    int l = rx.matchedLength();
    epscontent_s.replace(k, l, temp);

    // write content back to second file
    QFile epsgoodfile(fnBBCorrEps);
    r = epsgoodfile.open(dev_WRITEONLY);
    if ( ! r ) {
      res.status = KLFERR_EPSWRITEFAIL;
      res.errorstr = QObject::tr("Can't write to file '%1'!\n", "KLFBackend")
	.arg(fnBBCorrEps);
      return res;
    }
    epsgoodfile.dev_write(epscontent_s);

    if ( ! settings.outlineFonts ) {
      res.epsdata.ba_assign(epscontent_s);
    }
    // res.epsdata is now set.

    qDebug("%s: %s:  eps bbox set.", KLF_FUNC_NAME, KLF_SHORT_TIME) ;    

  } // end of block "make EPS"

  if (settings.outlineFonts) {
    // run 'gs' to outline fonts
    KLFBlockProcess proc;
    QStringList args;
    args << settings.gsexec << "-dNOCACHE" << "-dNOPAUSE" << "-dSAFER" << "-dEPSCrop"
	 << "-sDEVICE=pswrite" << "-sOutputFile="+dir_native_separators(fnOutlFontsEps)
	 << "-q" << "-dBATCH" << dir_native_separators(fnBBCorrEps);

    qDebug("%s: %s: about to gs (for outline fonts)...\n%s", KLF_FUNC_NAME, KLF_SHORT_TIME,
	   qPrintable(args.join(" ")));
    bool r = proc.startProcess(args, execenv);
    qDebug("%s: %s:  gs returned (for outline fonts).", KLF_FUNC_NAME, KLF_SHORT_TIME) ;    
  
    if ( ! r ) {
      res.status = KLFERR_NOGSPROG;
      res.errorstr = QObject::tr("Unable to start gs!\n", "KLFBackend");
      return res;
    }
    if ( !proc.processNormalExit() ) {
      res.status = KLFERR_GSNONORMALEXIT;
      res.errorstr = QObject::tr("gs died abnormally!\n", "KLFBackend");
      return res;
    }
    if ( proc.processExitStatus() != 0) {
      res.status = KLFERR_PROGERR_GS_OF;
      res.errorstr = progErrorMsg("gs", proc.processExitStatus(), proc.readStderrString(),
				  proc.readStdoutString());
      return res;
    }
    if (!QFile::exists(fnOutlFontsEps)) {
      res.status = KLFERR_NOEPSFILE_OF;
      res.errorstr = QObject::tr("EPS file (with outlined fonts) didn't appear after call to gs!\n",
				 "KLFBackend");
      return res;
    }

    // get and save outlined EPS to memory
    QFile ofepsfile(fnOutlFontsEps);
    r = ofepsfile.open(dev_READONLY);
    if ( ! r ) {
      res.status = KLFERR_EPSREADFAIL_OF;
      res.errorstr = QObject::tr("Unable to read file %1!\n", "KLFBackend")
	.arg(fnOutlFontsEps);
      return res;
    }
    res.epsdata = ofepsfile.readAll();
    ofepsfile.close();
    // res.epsdata is now set to the outlined-fonts version
  }

  { // run 'gs' to get png
    KLFBlockProcess proc;
    QStringList args;
    args << settings.gsexec << "-dNOPAUSE" << "-dSAFER" << "-dEPSCrop"
	 << "-r"+QString::number(in.dpi) << "-dTextAlphaBits=4"
	 << "-dGraphicsAlphaBits=4";
    if (qAlpha(in.bg_color) > 0) { // we're forcing a background color
      args << "-sDEVICE=png16m";
    } else {
      args << "-sDEVICE=pngalpha";
    }
    args << "-sOutputFile="+dir_native_separators(fnPng) << "-q" << "-dBATCH"
         << dir_native_separators(fnFinalEps);

    qDebug("%s: %s:  about to gs... %s", KLF_FUNC_NAME, KLF_SHORT_TIME, qPrintable(args.join(" "))) ;
    bool r = proc.startProcess(args, execenv);
    qDebug("%s: %s:  gs returned.", KLF_FUNC_NAME, KLF_SHORT_TIME) ;    
  
    if ( ! r ) {
      res.status = KLFERR_NOGSPROG;
      res.errorstr = QObject::tr("Unable to start gs!\n", "KLFBackend");
      return res;
    }
    if ( !proc.processNormalExit() ) {
      res.status = KLFERR_GSNONORMALEXIT;
      res.errorstr = QObject::tr("gs died abnormally!\n", "KLFBackend");
      return res;
    }
    if ( proc.processExitStatus() != 0) {
      res.status = KLFERR_PROGERR_GS;
      res.errorstr = progErrorMsg("gs", proc.processExitStatus(), proc.readStderrString(),
				  proc.readStdoutString());
      return res;
    }
    if (!QFile::exists(fnPng)) {
      res.status = KLFERR_NOPNGFILE;
      res.errorstr = QObject::tr("PNG file didn't appear after call to gs!\n", "KLFBackend");
      return res;
    }

    // get and save PNG to memory
    QFile pngfile(fnPng);
    r = pngfile.open(dev_READONLY);
    if ( ! r ) {
      res.status = KLFERR_PNGREADFAIL;
      res.errorstr = QObject::tr("Unable to read file %1!\n", "KLFBackend")
	.arg(fnPng);
      return res;
    }
    res.pngdata_raw = pngfile.readAll();
    pngfile.close();
    // res.pngdata_raw is now set.
    res.result.loadFromData(res.pngdata_raw, "PNG");

    // store some meta-information into result
    res.result.img_settext("AppVersion", QString::fromLatin1("KLatexFormula " KLF_VERSION_STRING));
    res.result.img_settext("Application",
		 QObject::tr("Created with KLatexFormula version %1", "KLFBackend::saveOutputToFile"));
    res.result.img_settext("Software", QString::fromLatin1("KLatexFormula " KLF_VERSION_STRING));
    res.result.img_settext("InputLatex", in.latex);
    res.result.img_settext("InputMathMode", in.mathmode);
    res.result.img_settext("InputPreamble", in.preamble);
    res.result.img_settext("InputFgColor", QString("rgb(%1, %2, %3)").arg(qRed(in.fg_color))
		   .arg(qGreen(in.fg_color)).arg(qBlue(in.fg_color)));
    res.result.img_settext("InputBgColor", QString("rgba(%1, %2, %3, %4)").arg(qRed(in.bg_color))
		   .arg(qGreen(in.bg_color)).arg(qBlue(in.bg_color))
		   .arg(qAlpha(in.bg_color)));
    res.result.img_settext("InputDPI", QString::number(in.dpi));
    res.result.img_settext("SettingsTBorderOffset", QString::number(settings.tborderoffset));
    res.result.img_settext("SettingsRBorderOffset", QString::number(settings.rborderoffset));
    res.result.img_settext("SettingsBBorderOffset", QString::number(settings.bborderoffset));
    res.result.img_settext("SettingsLBorderOffset", QString::number(settings.lborderoffset));
    res.result.img_settext("SettingsOutlineFonts", settings.outlineFonts?QString("true"):QString("false"));
  }

  { // create "final" PNG data
#ifdef KLFBACKEND_QT4
    QBuffer buf(&res.pngdata);
#else
    QBuffer buf(res.pngdata);
#endif
    buf.open(dev_WRITEONLY);
    bool r = res.result.save(&buf, "PNG");
    if (!r) {
      qWarning("%s: Error: Can't save \"final\" PNG data.", KLF_FUNC_NAME);
      res.pngdata.ba_assign(res.pngdata_raw);
    }
  }

  if (!settings.epstopdfexec.isEmpty()) {
    // if we have epstopdf functionality, then we'll take advantage of it to generate pdf:
    KLFBlockProcess proc;
    QStringList args;
    args << settings.epstopdfexec << dir_native_separators(fnFinalEps)
	 << ("--outfile="+dir_native_separators(fnPdf));

    qDebug("%s: %s:  about to epstopdf... %s", KLF_FUNC_NAME, KLF_SHORT_TIME, qPrintable(args.join(" "))) ;    
    bool r = proc.startProcess(args, execenv);
    qDebug("%s: %s:  epstopdf returned.", KLF_FUNC_NAME, KLF_SHORT_TIME) ;    

    if ( ! r ) {
      res.status = KLFERR_NOEPSTOPDFPROG;
      res.errorstr = QObject::tr("Unable to start epstopdf!\n", "KLFBackend");
      return res;
    }
    if ( !proc.processNormalExit() ) {
      res.status = KLFERR_EPSTOPDFNONORMALEXIT;
      res.errorstr = QObject::tr("epstopdf died nastily!\n", "KLFBackend");
      return res;
    }
    if ( proc.processExitStatus() != 0) {
      res.status = KLFERR_PROGERR_EPSTOPDF;
      res.errorstr = progErrorMsg("epstopdf", proc.processExitStatus(), proc.readStderrString(),
				  proc.readStdoutString());
      return res;
    }
    if (!QFile::exists(fnPdf)) {
      qDebug("%s: %s: pdf file '%s' didn't appear after epstopdf!", KLF_FUNC_NAME, KLF_SHORT_TIME,
	     qPrintable(fnPdf));
      res.status = KLFERR_NOPDFFILE;
      res.errorstr = QObject::tr("PDF file didn't appear after call to epstopdf!\n", "KLFBackend");
      return res;
    }

    // get and save PDF to memory
    QFile pdffile(fnPdf);
    r = pdffile.open(dev_READONLY);
    if ( ! r ) {
      res.status = KLFERR_PDFREADFAIL;
      res.errorstr = QObject::tr("Unable to read file %1!\n", "KLFBackend").arg(fnPdf);
      return res;
    }
    res.pdfdata = pdffile.readAll();

  }

  qDebug("%s: %s:  end of function.", KLF_FUNC_NAME, KLF_SHORT_TIME) ;    

  return res;
}


void KLFBackend::cleanup(QString tempfname)
{
  const char *skipcleanup = getenv("KLFBACKEND_LEAVE_TEMP_FILES");
  if (skipcleanup != NULL && (*skipcleanup == '1' || *skipcleanup == 't' || *skipcleanup == 'T' ||
			      *skipcleanup == 'y' || *skipcleanup == 'Y'))
    return; // skip cleaning up temp files

  if (QFile::exists(tempfname+".tex")) QFile::remove(tempfname+".tex");
  if (QFile::exists(tempfname+".dvi")) QFile::remove(tempfname+".dvi");
  if (QFile::exists(tempfname+".aux")) QFile::remove(tempfname+".aux");
  if (QFile::exists(tempfname+".log")) QFile::remove(tempfname+".log");
  if (QFile::exists(tempfname+".toc")) QFile::remove(tempfname+".toc");
  if (QFile::exists(tempfname+".eps")) QFile::remove(tempfname+".eps");
  if (QFile::exists(tempfname+"-good.eps")) QFile::remove(tempfname+"-good.eps");
  if (QFile::exists(tempfname+"-raw.eps")) QFile::remove(tempfname+"-raw.eps");
  if (QFile::exists(tempfname+"-bbcorr.eps")) QFile::remove(tempfname+"-bbcorr.eps");
  if (QFile::exists(tempfname+"-outlfonts.eps")) QFile::remove(tempfname+"-outlfonts.eps");
  if (QFile::exists(tempfname+".png")) QFile::remove(tempfname+".png");
  if (QFile::exists(tempfname+".pdf")) QFile::remove(tempfname+".pdf");
}

// static private mutex object
QMutex KLFBackend::__mutex;

KLF_EXPORT bool operator==(const KLFBackend::klfInput& a, const KLFBackend::klfInput& b)
{
  return a.latex == b.latex &&
    a.mathmode == b.mathmode &&
    a.preamble == b.preamble &&
    a.fg_color == b.fg_color &&
    a.bg_color == b.bg_color &&
    a.dpi == b.dpi;
}


bool KLFBackend::saveOutputToDevice(const klfOutput& klfoutput, QIODevice *device,
				    const QString& fmt, QString *errorStringPtr)
{
  QString format = fmt.s_trimmed().s_toUpper();

  // now choose correct data source and write to fout
  if (format == "EPS" || format == "PS") {
    device->dev_write(klfoutput.epsdata);
  } else if (format == "PNG") {
    device->dev_write(klfoutput.pngdata);
  } else if (format == "PDF") {
    if (klfoutput.pdfdata.isEmpty()) {
      QString error = QObject::tr("PDF format is not available!\n",
				  "KLFBackend::saveOutputToFile");
      qWarning("%s", qPrintable(error));
      if (errorStringPtr != NULL)
	errorStringPtr->operator=(error);
      return false;
    }
    device->dev_write(klfoutput.pdfdata);
 } else {
    bool res = klfoutput.result.save(device, format.s_toLatin1());
    if ( ! res ) {
      QString errstr = QObject::tr("Unable to save image in format `%1'!",
				   "KLFBackend::saveOutputToDevice").arg(format);
      qWarning("%s", qPrintable(errstr));
      if (errorStringPtr != NULL)
	*errorStringPtr = errstr;
      return false;
    }
  }

  return true;
}

bool KLFBackend::saveOutputToFile(const klfOutput& klfoutput, const QString& fileName,
				  const QString& fmt, QString *errorStringPtr)
{
  QString format = fmt;
  // determine format first
  if (format.isEmpty() && !fileName.isEmpty()) {
    QFileInfo fi(fileName);
    if ( ! fi.fi_suffix().isEmpty() )
      format = fi.fi_suffix();
  }
  if (format.isEmpty())
    format = QLatin1String("PNG");
  format = format.s_trimmed().s_toUpper();
  // got format. choose output now and prepare write
  QFile fout;
  if (fileName.isEmpty() || fileName == "-") {
    if ( ! fout.f_open_fp(stdout) ) {
      QString error = QObject::tr("Unable to open stderr for write! Error: %1\n",
				  "KLFBackend::saveOutputToFile").arg(fout.f_error());
      qWarning("%s", qPrintable(error));
      if (errorStringPtr != NULL)
	*errorStringPtr = error;
      return false;
    }
  } else {
    fout.f_setFileName(fileName);
    if ( ! fout.open(dev_WRITEONLY) ) {
      QString error = QObject::tr("Unable to write to file `%1'! Error: %2\n",
				  "KLFBackend::saveOutputToFile")
	.arg(fileName).arg(fout.f_error());
      qWarning("%s", qPrintable(error));
      if (errorStringPtr != NULL)
	*errorStringPtr = error;
      return false;
    }
  }

  return saveOutputToDevice(klfoutput, &fout, format, errorStringPtr);
}


bool KLFBackend::detectSettings(klfSettings *settings, const QString& extraPath)
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;

  QStringList stdextrapaths;
  int k, j;
  for (k = 0; standard_extra_paths[k] != NULL; ++k) {
    stdextrapaths.append(standard_extra_paths[k]);
  }
  QString extra_paths = stdextrapaths.join(QString("")+KLF_PATH_SEP);
  if (!extraPath.isEmpty())
    extra_paths += KLF_PATH_SEP + extraPath;

  // temp dir
#ifdef KLFBACKEND_QT4
  settings->tempdir = QDir::fromNativeSeparators(QDir::tempPath());
#else
# if defined(Q_OS_UNIX) || defined(Q_OS_LINUX) || defined(Q_OS_DARWIN) || defined(Q_OS_MACX)
  settings->tempdir = "/tmp";
# elif defined(Q_OS_WIN32)
  settings->tempdir = getenv("TEMP");
# else
  settings->tempdir = QString();
# endif
#endif

  // sensible defaults
  settings->lborderoffset = 1;
  settings->tborderoffset = 1;
  settings->rborderoffset = 1;
  settings->bborderoffset = 1;
  
  // find executables
  struct { QString * target_setting; QStringList prog_names; }  progs_to_find[] = {
    { & settings->latexexec, progLATEX },
    { & settings->dvipsexec, progDVIPS },
    { & settings->gsexec, progGS },
    { & settings->epstopdfexec, progEPSTOPDF },
    { NULL, QStringList() }
  };
  // replace @executable_path in extra_paths
  klfDbg(klfFmtCC("Our base extra paths are: %s", qPrintable(extra_paths))) ;
  QString ourextrapaths = extra_paths;
  ourextrapaths.replace("@executable_path", qApp->applicationDirPath());
  klfDbg(klfFmtCC("Our extra paths are: %s", qPrintable(ourextrapaths))) ;
  // and actually search for those executables
  for (k = 0; progs_to_find[k].target_setting != NULL; ++k) {
    klfDbg("Looking for "+progs_to_find[k].prog_names.join(" or ")) ;
    for (j = 0; j < (int)progs_to_find[k].prog_names.size(); ++j) {
      klfDbg("Testing `"+progs_to_find[k].prog_names[j]+"'") ;
      *progs_to_find[k].target_setting
	= klfSearchPath(progs_to_find[k].prog_names[j], ourextrapaths);
      if (!progs_to_find[k].target_setting->isEmpty()) {
	klfDbg("Found! at `"+ *progs_to_find[k].target_setting+"'") ;
	break; // found a program
      }
    }
  }

  klf_detect_execenv(settings);

  bool result_failure =
    settings->tempdir.isEmpty() || settings->latexexec.isEmpty() || settings->dvipsexec.isEmpty() ||
    settings->gsexec.isEmpty(); // NOTE:  settings->epstopdfexec.isEmpty() is NOT a failure

  return !result_failure;
}


/** \brief detects any additional settings to environment variables
 *
 * Detects whether the given values of latex, dvips, gs and epstopdf in the
 * given (initialized) settings \c settings need extra environment set,
 * and sets the \c execenv member of \c settings accordingly.
 *
 * Note that the environment settings already existing in \c settings->execenv are
 * kept; only those variables for which new values are detected are updated, or if
 * new declarations are needed they are appended.
 *
 * \note KLFBackend::detectSettings() already calls this function, you don't
 *   have to call this function manually in that case.
 *
 * \return TRUE (success) or FALSE (failure). Currently there is no reason
 * for failure, and returns always TRUE (as of 3.2.1).
 */
KLF_EXPORT bool klf_detect_execenv(KLFBackend::klfSettings *settings)
{
  // detect mgs.exe as ghostscript and setup its environment properly
  QFileInfo gsfi(settings->gsexec);
  if (gsfi.fileName() == "mgs.exe") {
    QString mgsenv = QString("MIKTEX_GS_LIB=")
      + dir_native_separators(gsfi.fi_absolutePath()+"/../../ghostscript/base")
      + ";"
      + dir_native_separators(gsfi.fi_absolutePath()+"/../../fonts");
    __klf_append_replace_env_var(& settings->execenv, "MIKTEX_GS_LIB", mgsenv);
    klfDbg("Adjusting environment for mgs.exe: `"+mgsenv+"'") ;
  }

#ifdef Q_WS_MAC
  // make sure that epstopdf's path is in PATH because it wants to all gs
  // (eg fink distributions)
  if (!settings->epstopdfexec.isEmpty()) {
    QFileInfo epstopdf_fi(settings->epstopdfexec);
    QString execenvpath = QString("PATH=%1:$PATH").arg(epstopdf_fi.fi_absolutePath());
    __klf_append_replace_env_var(& settings->execenv, "PATH", execenvpath);
  }
#endif

  return true;
}
