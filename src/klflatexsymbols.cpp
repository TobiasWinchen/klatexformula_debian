/***************************************************************************
 *   file klflatexsymbols.cpp
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
/* $Id: klflatexsymbols.cpp 888 2014-07-24 17:25:51Z phfaist $ */

#include <stdio.h>

#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QScrollArea>
#include <QRegExp>
#include <QList>
#include <QStringList>
#include <QProgressDialog>
#include <QGridLayout>
#include <QPushButton>
#include <QHash>
#include <QStackedWidget>
#include <QLineEdit>
#include <QMessageBox>
#include <QScrollBar>
#include <QApplication>
#include <QCloseEvent>
#include <QDebug>
#include <QPainter>
#include <QPlastiqueStyle>

#include <QDomDocument>
#include <QDomElement>

#include <klfbackend.h>

#include <ui_klflatexsymbols.h>

#include <klfpixmapbutton.h>
#include <klfrelativefont.h>

#include "klfmain.h"
#include "klfconfig.h"
#include "klflatexsymbols.h"

#include "klflatexsymbols_p.h"



// ------------------

KLF_EXPORT QDebug operator<<(QDebug str, const KLFLatexSymbol& s)
{
  return str << "KLFLatexSymbol["<<s.symbol
	     <<(s.symbol_option?"[]":"")
	     <<(s.symbol_numargs>0?qPrintable("{"+QString::number(s.symbol_numargs)+"x}"):"")
	     <<";preview="<<(s.previewlatex.size()?qPrintable("`"+s.previewlatex+"'"):"")
	     <<";preamblelist/size="<<s.preamble.size()
	     <<";txtmd="<<s.textmode<<";hidden="<<s.hidden
	     <<";kw="<<s.keywords;
}


KLFLatexSymbol::KLFLatexSymbol(const QDomElement& e)
  : symbol(), preamble(), textmode(false), bbexpand(), hidden(false)
{
  // preamble
  QDomNodeList usepackagelist = e.elementsByTagName("usepackage");
  int k;
  for (k = 0; k < usepackagelist.size(); ++k) {
    QString package = usepackagelist.at(k).toElement().attribute("name");
    if (package[0] == '[' || package[0] == '{')
      preamble.append(QString::fromAscii("\\usepackage%1").arg(package));
    else
      preamble.append(QString::fromAscii("\\usepackage{%1}").arg(package));
  }
  QDomNodeList preamblelinelist = e.elementsByTagName("preambleline");
  for (k = 0; k < preamblelinelist.size(); ++k) {
    preamble.append(preamblelinelist.at(k).toElement().text());
  }
  // textmode
  if (e.attribute("textmode") == "true")
    textmode = true;
  else
    textmode = false;

  if (e.elementsByTagName("hidden").size() > 0)
    hidden = true;

  // bb offset
  QDomNodeList bblist = e.elementsByTagName("bb");
  if (bblist.size() > 1) {
    fprintf(stderr, "WARNING: Expected at most single <bb expand=\"..\"/> item!\n");
  }
  if (bblist.size()) {
    sscanf(bblist.at(0).toElement().attribute("expand").toLatin1().constData(), "%d,%d,%d,%d",
	   &bbexpand.t, &bbexpand.r, &bbexpand.b, &bbexpand.l);
  }

  // latex code
  QDomNodeList latexlist = e.elementsByTagName("latex");
  if (latexlist.size() != 1) {
    fprintf(stderr, "WARNING: Expected single <latex>...</latex> in symbol entry!\n");
  }
  if (latexlist.size() == 0)
    return;
  QRegExp rx_bool("\\s*(y(?:es)?|t(?:rue)?|on|1)|(n(?:o)?|f(?:alse)?|off|0)\\s*", Qt::CaseInsensitive);

  QDomElement latexnode = latexlist.at(0).toElement();
  symbol = latexnode.text();
  QString symopt = latexnode.attribute("option", "no");
  bool boolok = rx_bool.exactMatch(symopt);
  if (!boolok) {
    qWarning()<<KLF_FUNC_NAME<<": Bad boolean format "<<symopt;
    symbol_option = false;
  } else {
    // if cap(1) is non-empty, TRUE; if cap(2) is non-empty then FALSE
    symbol_option = rx_bool.cap(1).size();
  }
  QString nargs = latexnode.attribute("numargs", "0");
  bool uintok = true;
  symbol_numargs = nargs.toUInt(&uintok);
  if (!uintok) {
    qWarning()<<KLF_FUNC_NAME<<": Bad integer format "<<nargs;
  }

  // latex preview code
  QDomNodeList previewlatexlist = e.elementsByTagName("preview-latex");
  if (previewlatexlist.size() > 1) {
    fprintf(stderr, "WARNING: Expected at most one <preview-latex>...</preview-latex> in symbol entry!\n");
  }
  if (previewlatexlist.size() > 0) {
    QDomElement previewlatexnode = previewlatexlist.at(0).toElement();
    previewlatex = previewlatexnode.text();
  } else {
    previewlatex = QString();
  }

  // keywords
  QDomNodeList keywordslist = e.elementsByTagName("keywords");
  for (k = 0; k < keywordslist.size(); ++k) {
    QStringList kw;
    kw = keywordslist.at(k).toElement().text().split(QRegExp("\\s+"), QString::SkipEmptyParts);
    keywords << kw;
  }

  klfDbg("read symbol "<<symbol<<" hidden="<<hidden);
}

QString KLFLatexSymbol::symbolWithArgs() const
{
  QString s = symbol;
  if (symbol_option)
    s += "[]";
  int n = symbol_numargs;
  while (n--)
    s += "{}";
  return s;
}

QString KLFLatexSymbol::latexCodeForPreview() const
{
  if (previewlatex.size())
    return previewlatex;

  static const QString dummyopt = "n";
  static const QStringList dummyargs =
    QStringList()<<"x"<<"y"<<"z"<<"w"<<"a"<<"b"<<"c"<<"d"<<"e"<<"f";
  int k;

  QString s = symbol;
  if (symbol_option)
    s += "["+dummyopt+"]";
  for (k = 0; k < symbol_numargs; ++k)
    s += "{"+dummyargs[k%dummyargs.size()]+"}";

  return s;
}


KLF_EXPORT bool operator==(const KLFLatexSymbol& a, const KLFLatexSymbol& b)
{
  return a.symbol == b.symbol &&
    a.symbol_numargs == b.symbol_numargs &&
    a.symbol_option == b.symbol_option &&
    a.previewlatex == b.previewlatex &&
    a.textmode == b.textmode &&
    a.preamble == b.preamble &&
    a.bbexpand.t == b.bbexpand.t &&
    a.bbexpand.r == b.bbexpand.r &&
    a.bbexpand.b == b.bbexpand.b &&
    a.bbexpand.l == b.bbexpand.l &&
    a.hidden == b.hidden;
}

// KLF_EXPORT bool operator<(const KLFLatexSymbol& a, const KLFLatexSymbol& b)
// {
//   if (a.symbol != b.symbol)
//     return a.symbol < b.symbol;
//   if (a.textmode != b.textmode)
//     return a.textmode < b.textmode;
//   if (a.preamble.size() != b.preamble.size())
//     return a.preamble.size() < b.preamble.size();
//   int k;
//   for (k = 0; k < a.preamble.size(); ++k)
//     if (a.preamble[k] != b.preamble[k])
//       return a.preamble[k] < b.preamble[k];
//   // a and b seem to be equal
//   return false;
// }

QDataStream& operator<<(QDataStream& stream, const KLFLatexSymbol& s)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  return stream << s.symbol
		<< (quint8)s.symbol_option
		<< (quint8)s.symbol_numargs
		<< s.previewlatex
		<< s.preamble
		<< (quint8)s.textmode
		<< (qint16)s.bbexpand.t << (qint16)s.bbexpand.r << (qint16)s.bbexpand.b << (qint16)s.bbexpand.l
		<< (quint8)s.hidden
		<< s.keywords;
}

QDataStream& operator>>(QDataStream& stream, KLFLatexSymbol& s)
{
  quint8 symopt, symnargs, textmode, hidden;
  struct { qint16 t, r, b, l; } readbbexpand;
  stream >> s.symbol
	 >> symopt
	 >> symnargs
	 >> s.previewlatex
	 >> s.preamble
	 >> textmode
	 >> readbbexpand.t >> readbbexpand.r >> readbbexpand.b >> readbbexpand.l
	 >> hidden
	 >> s.keywords;
  s.symbol_option = symopt;
  s.symbol_numargs = symnargs;
  s.bbexpand.t = readbbexpand.t;
  s.bbexpand.r = readbbexpand.r;
  s.bbexpand.b = readbbexpand.b;
  s.bbexpand.l = readbbexpand.l;
  s.textmode = textmode;
  s.hidden = hidden;
  return stream;
}



// -----------------------------------------------------------


KLFLatexSymbolsCache * KLFLatexSymbolsCache::staticCache = NULL;

static QString __rel_cache_file = QString();
static QString relcachefile()
{
  if (__rel_cache_file.isEmpty())
    __rel_cache_file =
      QString("/symbolspixmapcache-klf%1").arg(KLF_DATA_STREAM_APP_VERSION);
  return __rel_cache_file;
}

KLFLatexSymbolsCache::KLFLatexSymbolsCache()
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;

  d = new KLFLatexSymbolsCachePrivate;

  // load the cache

  QStringList cachefiles;
  cachefiles
    << klfconfig.homeConfigDir+relcachefile()
    << ":/data/symbolspixmapcache_base" ;
  int k;
  bool ok = false;
  for (k = 0; !ok && k < cachefiles.size(); ++k) {
    // ??: do the two attempts here apply a datastream version to the header only, or to
    //     the data too?
    klfDbg("trying to load from "<<cachefiles[k]) ;
    ok = (  loadCacheFrom(cachefiles[k], QDataStream::Qt_4_4)
	    == KLFLatexSymbolsCache::Ok  ) ;
    if (!ok) {
      klfDbg("trying to load from "<<cachefiles[k]<<" with default header datastream version") ;
      ok = (  loadCacheFrom(cachefiles[k], -1)
	      == KLFLatexSymbolsCache::Ok  ) ;
    }
  }
  if ( ! ok ) {
    qWarning() << KLF_FUNC_NAME << ": error finding and reading cache file!";
  }

  d->flag_modified = false;
}

KLFLatexSymbolsCache::~KLFLatexSymbolsCache()
{
  delete d;
}

int KLFLatexSymbolsCache::loadCacheStream(QDataStream& stream)
{
  QString readHeader;
  QString readCompatKLFVersion;
  bool r = klfDataStreamReadHeader(stream, QStringList()<<"KLATEXFORMULA_SYMBOLS_PIXMAP_CACHE",
				   &readHeader, &readCompatKLFVersion);
  if (!r) {
    klfDbg("failed to read symbolscache data header. readHeader="<<readHeader
	   <<", readcompatklfver="<<readCompatKLFVersion) ;
    if (readHeader.isEmpty() || readCompatKLFVersion.isEmpty())
      return BadHeader;
    // otherwise, it's a bad version error
    return BadVersion;
  }

  // stream is now ready to read

  stream >> d->cache;

  d->flag_modified = false;
  return 0;
}

int KLFLatexSymbolsCache::saveCacheStream(QDataStream& stream)
{
  klfDataStreamWriteHeader(stream, "KLATEXFORMULA_SYMBOLS_PIXMAP_CACHE");
  // stream is now ready to be written
  stream << d->cache;
  d->flag_modified = false;
  return 0;
}

bool KLFLatexSymbolsCache::cacheNeedsSave() const
{
  return d->flag_modified;
}

QPixmap KLFLatexSymbolsCache::getPixmap(const KLFLatexSymbol& sym, bool fromcacheonly)
{
  klfDbg("sym.symbol="<<sym.symbol<<" fromCacheOnly="<<fromcacheonly) ;
  klfDbg("full symbol: "<<sym) ;

  if (d->cache.contains(sym)) {
    klfDbg("Found symbol in cache! pixmap is null="<<d->cache[sym].pix.isNull()
	   <<"; sym.preamble="<<sym.preamble.join(";"));
    return d->cache[sym].pix;
  }

  if (fromcacheonly) {
    // if we weren't able to load it from cache, show failed icon
    return QPixmap(":/pics/badsym.png");
  }

  {
    KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME+"/clean cache duplicate test") ;
    // clean cache: make sure there are no two duplicate symbols (this is for the popup hint parser,
    // so that it doesn't detect old symbols in the cache)
    // This is done only if fromcache is false, so as to perform the check only on first pass
    // when generating the symbol cache.
    QHash<KLFLatexSymbol,SymbolInfo>::iterator it = d->cache.begin();
    while (it != d->cache.end()) {
      klfDbg("Testing symbol "<<it.key().symbol<<",preamble="<<it.key().preamble.join(",")
	     << "for being a duplicate of "<<sym.symbol);
      if (it.key().symbol == sym.symbol && !it.value().confirmed) {
	klfDbg("erasing duplicate marked unconfirmed.");
	it = d->cache.erase(it); // erase old symbol entry
      } else {
	++it;
      }
    }
  }

  if (sym.hidden) {
    // special treatment for hidden symbols
    // insert a QPixmap() into cache and return it
    klfDbg("symbol is hidden. Assigning NULL pixmap.") ;
    d->cache[sym].pix = QPixmap();
    return QPixmap();
  }

  const float mag = 4.0;

  KLFBackend::klfInput in;
  in.latex = sym.latexCodeForPreview();
  in.mathmode = sym.textmode ? "..." : "\\[ ... \\]";
  in.preamble = sym.preamble.join("\n")+"\n";
  in.fg_color = qRgb(0,0,0);
  in.bg_color = qRgba(255,255,255,0); // transparent Bg
  in.dpi = (int)(mag * 150);

  d->backendsettings.epstopdfexec = ""; // don't waste time making PDF, we don't need it
  d->backendsettings.tborderoffset = sym.bbexpand.t;
  d->backendsettings.rborderoffset = sym.bbexpand.r;
  d->backendsettings.bborderoffset = sym.bbexpand.b;
  d->backendsettings.lborderoffset = sym.bbexpand.l;

  KLFBackend::klfOutput out = KLFBackend::getLatexFormula(in, d->backendsettings);

  if (out.status != 0) {
    qWarning()
      <<KLF_FUNC_NAME
      <<QString(":ERROR: Can't generate preview for symbol %1 : status %2 !\n\tError: %3\n")
      .arg(sym.symbol).arg(out.status).arg(out.errorstr);
    return QPixmap(":/pics/badsym.png");
  } else {
    klfDbg("successfully got pixmap for symbol "<<sym.symbol<<".") ;
  }

  d->flag_modified = true;

  QImage scaled = out.result.scaled((int)(out.result.width() / mag),
				    (int)(out.result.height() / mag),
				    Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
  QPixmap pix = QPixmap::fromImage(scaled);
  d->cache[sym].pix = pix;
  d->cache[sym].confirmed = true;

  klfDbg("Ran getLatexFormula(), got the pixmap. Returning.") ;

  return pix;
}

int KLFLatexSymbolsCache::precacheList(const QList<KLFLatexSymbol>& list, bool userfeedback,
				       QWidget *parent)
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;

  QProgressDialog *pdlg = NULL;

  if (userfeedback) {
    pdlg = new QProgressDialog(QObject::tr("Please wait while generating symbol previews ... "),
			       QObject::tr("Skip"), 0, list.size()-1, parent);
    /** TODO: we should do a first pass to see which symbols are missing, then
     * on a second pass generate those symbols with a progress dialog... */
    //  pdlg->setMinimumDuration(15000);
    pdlg->setWindowModality(Qt::WindowModal);
    pdlg->setModal(true);
    pdlg->setValue(0);
  }

  for (int i = 0; i < list.size(); ++i) {
    if (userfeedback) {
      // get events for cancel button (for example)
      qApp->processEvents();
      if (pdlg->wasCanceled()) {
	delete pdlg;
	return 1;
      }
      pdlg->setValue(i);
    }
    getPixmap(list[i], false);
  }

  if (userfeedback) {
    delete pdlg;
  }

  return 0;
};



void KLFLatexSymbolsCache::setBackendSettings(const KLFBackend::klfSettings& settings)
{
  d->backendsettings = settings;
}

KLFLatexSymbol KLFLatexSymbolsCache::findSymbol(const QString& symbolCode)
{
  for (QHash<KLFLatexSymbol,SymbolInfo>::const_iterator it = d->cache.begin();
       it != d->cache.end(); ++it) {
    if (it.key().symbol == symbolCode) {
      return it.key();
    }
  }
  return KLFLatexSymbol();
}

QList<KLFLatexSymbol> KLFLatexSymbolsCache::findSymbols(const QString& symbolCode)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  QList<KLFLatexSymbol> slist;
  for (QHash<KLFLatexSymbol,SymbolInfo>::const_iterator it = d->cache.begin();
       it != d->cache.end(); ++it) {
    if (it.key().symbol == symbolCode) {
      klfDbg("found symbol: "<<it.key());
      slist << it.key();
    }
  }
  return slist;
}

QStringList KLFLatexSymbolsCache::symbolCodeList()
{
  QStringList l;
  for (QHash<KLFLatexSymbol,SymbolInfo>::const_iterator it = d->cache.begin();
       it != d->cache.end(); ++it)
    l << it.key().symbol;
  return l;
}

QPixmap KLFLatexSymbolsCache::findSymbolPixmap(const QString& symbolCode)
{
  KLFLatexSymbol sym = findSymbol(symbolCode);
  if (sym.symbol.isEmpty()) {
    // invalid symbol
    qWarning()<<KLF_FUNC_NAME<<": Can't find symbol "<<symbolCode<<".";
    return QPixmap();
  }
  // return the pixmap from cache
  return d->cache[sym].pix;
}

void KLFLatexSymbolsCache::debugDump()
{
#ifdef KLF_DEBUG
  klfDbg("dumping cache.");
  QHash<KLFLatexSymbol,SymbolInfo>::const_iterator it;
  int k;
  for (it = d->cache.begin(), k = 0; it != d->cache.end(); ++it, ++k) {
    const KLFLatexSymbol& s = it.key();
    const QPixmap& p = it.value().pix;
    qDebug("  #%4d  %s (%s)  prembl/sz=%d  txtmd=%s  hid=%s  pix/sz=(%d,%d) confirmed=%s", k,
	   qPrintable(s.symbolWithArgs()), qPrintable(s.previewlatex),
	   s.preamble.size(), s.textmode?"yes":"no", s.hidden?"yes":"no",
	   p.width(), p.height(), it.value().confirmed?"yes":"no"
	   );
  }
#endif
}





// private
int KLFLatexSymbolsCache::loadCacheFrom(const QString& fname, int version)
{
  QFile f(fname);
  if ( ! f.open(QIODevice::ReadOnly) ) {
    klfDbg("Failed to open "<<fname) ;
    return -1;
  }
  QDataStream ds(&f);
  if (version >= 0)
    ds.setVersion(version);
  int r = loadCacheStream(ds);
  return r;
}

// static
KLFLatexSymbolsCache * KLFLatexSymbolsCache::theCache()
{
  if (staticCache == NULL) {
    staticCache = new KLFLatexSymbolsCache;
  }
  return staticCache;
}
// static
void KLFLatexSymbolsCache::saveTheCache()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  if (staticCache->cacheNeedsSave()) {
    QString s = klfconfig.homeConfigDir + relcachefile();
    QFile f(s);
    if ( ! f.open(QIODevice::WriteOnly) ) {
      qWarning() << KLF_FUNC_NAME<< "Can't save cache to file "<< s << "!";
      return;
    }
    QDataStream ds(&f);
    ds.setVersion(QDataStream::Qt_4_4);
    staticCache->saveCacheStream(ds);
    klfDbg("Saved cache to file "<<s);
  }
}




// -----------------------------------------------------------





KLFLatexSymbolsView::KLFLatexSymbolsView(const QString& category, QWidget *parent)
  : QScrollArea(parent), _category(category)
{
  mFrame = new QWidget(this);

  setWidgetResizable(true);

  //  mFrame->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
  //  mFrame->setFrameShadow(QFrame::Sunken);
  //  mFrame->setFrameShape(QFrame::Box);
  mFrame->setObjectName("frmSymbolList");
  //   mFrame->setFrameShadow(QFrame::Plain);
  //   mFrame->setFrameShape(QFrame::NoFrame);
  //   mFrame->setMidLineWidth(0);
  //   mFrame->setLineWidth(0);

  mLayout = 0;
  mSpacerItem = 0;

  setWidget(mFrame);
}

void KLFLatexSymbolsView::setSymbolList(const QList<KLFLatexSymbol>& symbols)
{
  _symbols.clear();
  appendSymbolList(symbols);
}
void KLFLatexSymbolsView::appendSymbolList(const QList<KLFLatexSymbol>& symbols)
{
  // filter out hidden symbols
  int k;
  for (k = 0; k < symbols.size(); ++k)
    if ( ! symbols[k].hidden )
      _symbols.append(symbols[k]);
}

void KLFLatexSymbolsView::buildDisplay()
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;
#ifdef Q_WS_MAC
  QStyle *myStyle = new QPlastiqueStyle;
  QPalette pal = palette();
  pal.setColor(QPalette::Window, QColor(206,207,233));
  pal.setColor(QPalette::Base, QColor(206,207,233));
  pal.setColor(QPalette::Button, QColor(206,207,233));
#endif
  mLayout = new QGridLayout(mFrame);
  int i, k;
  for (i = 0; i < _symbols.size(); ++i) {
    QPixmap p = KLFLatexSymbolsCache::theCache()->getPixmap(_symbols[i]);
    KLFPixmapButton *btn = new KLFPixmapButton(p, mFrame);
#ifdef Q_WS_MAC
    btn->setStyle(myStyle);
    btn->setPalette(pal);
#endif
    btn->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred));
    btn->setProperty("symbol", QVariant::fromValue<int>(i));
    btn->setProperty("gridpos", QPoint(-1,-1));
    btn->setProperty("gridcolspan", -1);
    btn->setProperty("myWidth", p.width() + 4);
    QString symcode = _symbols[i].symbol;
    if (_symbols[i].symbol_option)
      symcode += "[]";
    for (k = 0; _symbols[i].symbol_numargs > 0 && k < _symbols[i].symbol_numargs; ++k) {
      symcode += "{}";
    }
    QString tooltiptext =
      "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\""
      " \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
      "<html><head><meta name=\"qrichtext\" content=\"1\" />"
      "<style type=\"text/css\">\n"
      "p { white-space: pre-wrap; padding: 0px; margin: 0px 0px 2px 0px; }\n"
      "pre { padding: 0px; margin: 0px 0px 2px 0px; }\n"
      "</style>"
      "</head>"
      "<body>\n"
      "<p style=\"white-space: pre\">"+tr("LaTeX code:")+" <b><tt>"+symcode+"</tt></b>"+
      (_symbols[i].textmode?tr(" [in text mode]"):QString(""))+
      +"</p>";
    if (_symbols[i].preamble.size())
      tooltiptext += "<p>"+tr("Requires:")+"<b><pre>" +
	_symbols[i].preamble.join("\n")+"</pre></b></p>";
    if (_symbols[i].keywords.size())
      tooltiptext += "<p>"+tr("Key Words:")+"<pre><b>" +
	_symbols[i].keywords.join("</b>, <b>")+"</b></pre></p>";
    tooltiptext += "</body></html>";
    btn->setToolTip(tooltiptext);
    //klfDbg("tooltip text is "<<tooltiptext);
    connect(btn, SIGNAL(clicked()), this, SLOT(slotSymbolActivated()));
    mSymbols.append(btn);
  }
  mSpacerItem = new QSpacerItem(1, 1, QSizePolicy::Fixed, QSizePolicy::Expanding);
  recalcLayout();
}

void KLFLatexSymbolsView::recalcLayout()
{
  int row = 0, col = 0, colspan;
  int n = klfconfig.UI.symbolsPerLine;
  int quantawidth = 55; // hard-coded here
  //  printf("DEBUG: n=%d, quantawidth=%d\n", n, quantawidth);
  int i;
  // now add them all again as needed
  for (i = 0; i < mSymbols.size(); ++i) {
    colspan = 1 + mSymbols[i]->property("myWidth").toInt() / quantawidth;
    if (colspan < 1)
      colspan = 1;

    if (colspan > n)
      colspan = n;
    if (col + colspan > n) {
      row++;
      col = 0;
    }
    if (mSymbols[i]->property("gridpos") != QPoint(row, col) ||
	mSymbols[i]->property("gridcolspan") != colspan) {
      //      printf("DEBUG: %d: setting to (%d,%d)+(1,%d)\n", i, row, col, colspan);
      mSymbols[i]->setProperty("gridpos", QPoint(row, col));
      mSymbols[i]->setProperty("gridcolspan", colspan);
      mLayout->removeWidget(mSymbols[i]);
      mLayout->addWidget(mSymbols[i], row, col, 1, colspan);
    }
    col += colspan;
    if (col >= n) {
      row++;
      col = 0;
    }
  }
  // remove spacer and add it again
  mLayout->removeItem(mSpacerItem);
  mLayout->addItem(mSpacerItem, row+1, 0);

  setMinimumWidth(mFrame->sizeHint().width() + verticalScrollBar()->width() + 2);
}


void KLFLatexSymbolsView::slotSymbolActivated()
{
  QObject *s = sender();
  int i = s->property("symbol").toInt();
  if (i < 0 || i >= _symbols.size())
    qWarning()<<KLF_FUNC_NAME<<": Inavlid symbol index "<<i;
  else
    emit symbolActivated(_symbols[i]);
}


static bool stringlist_contains(const QStringList& l, const QString& qs, Qt::CaseSensitivity cs)
{
  QRegExp qsrx(".*"+QRegExp::escape(qs)+".*", cs);
  return l.indexOf(qsrx) >= 0;
}

bool KLFLatexSymbolsView::symbolMatches(int symbol, const QString& curqstr)
{
  if (symbol < 0 || symbol >= mSymbols.size())
    return false;

  int symIndex = mSymbols[symbol]->property("symbol").toInt();
  if (symIndex < 0 || symIndex >= _symbols.size()) {
    qWarning()<<KLF_FUNC_NAME<<": Inavlid symbol index "<<symIndex;
    return false;
  }

  // (X)Emacs-style: presence of capital letter triggers case sensitive search
  Qt::CaseSensitivity cs = (curqstr.contains(QRegExp("[A-Z]")) ? Qt::CaseSensitive : Qt::CaseInsensitive) ;

  if ( _symbols[symIndex].symbol.contains(curqstr, cs) ||
       stringlist_contains(_symbols[symIndex].preamble, curqstr, cs) ||
       stringlist_contains(_symbols[symIndex].keywords, curqstr, cs) ) {
    klfDbg("found match at "<<symIndex<<": "<<_symbols[symIndex].symbol) ;
    return true;
  }
  return false;
}

void KLFLatexSymbolsView::highlightSearchMatches(int currentMatch, const QString& curqstr)
{
  QString stylesheets[] = {
    // don't affect tooltips: give KLFPixmapButton { } scopes
    "",
    "KLFPixmapButton { background-color: rgb(180,180,255); }",
    "KLFPixmapButton { background-color: rgb(0,0,255); }"
  };

  if (currentMatch == -1) {
    // abort search
    stylesheets[0] = stylesheets[1] = stylesheets[2] = QString();
  }
  int k;
  for (k = 0; k < mSymbols.size(); ++k) {
    int which = 0;
    if (k == currentMatch)
      which = 2;
    else if (curqstr.size() && symbolMatches(k, curqstr))
      which = 1;
    mSymbols[k]->setStyleSheet(stylesheets[which]);
  }  
}





KLFLatexSymbols::KLFLatexSymbols(QWidget *parent, const KLFBackend::klfSettings& baseSettings)
  : QWidget(
#if defined(Q_WS_WIN) || defined(Q_WS_MAC)
	    0 /* parent */
#else
	    parent /* 0 */
#endif
	    , /*Qt::Tool*/ Qt::Window /*0*/)
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;
  Q_UNUSED(parent) ;

  u = new Ui::KLFLatexSymbols;
  u->setupUi(this);
  setAttribute(Qt::WA_StyledBackground);

  // add our search bar
  pSearchBar = u->searchbar; //new KLFSearchBar(this);
  KLF_DEBUG_ASSIGN_REF_INSTANCE(pSearchBar, "latexsymbols-searchbar") ;
  //  pSearchBar->setShowOverlayMode(true);
  pSearchBar->registerShortcuts(this);
  pSearchBar->setShowHideButton(true);
  connect(pSearchBar, SIGNAL(escapePressed()), pSearchBar, SLOT(hide()));

  pSearchable = new KLFLatexSymbolsSearchable(this);
  pSearchBar->setSearchTarget(pSearchable);

  u->btnInsertSearchCurrent->hide();

  klfDbg("prepared search bar.") ;

  KLFLatexSymbolsCache::theCache()->setBackendSettings(baseSettings);

  // read our config and create the UI
  read_symbols_create_ui();

  slotShowCategory(0);

  KLFRelativeFont *relfont = new KLFRelativeFont(u->cbxCategory);
  relfont->setRelPointSize(+2);

  connect(u->cbxCategory, SIGNAL(highlighted(int)), this, SLOT(slotShowCategory(int)));
  connect(u->cbxCategory, SIGNAL(activated(int)), this, SLOT(slotShowCategory(int)));
  connect(u->btnClose, SIGNAL(clicked()), this, SLOT(close()));

  connect(u->btnInsertSearchCurrent, SIGNAL(clicked()),
	  pSearchable, SLOT(slotInsertCurrentMatch()));

#ifdef Q_WS_MAC
  // Cmd+Space is used by spotlight, so use Ctrl+Space (on Qt/Mac, Ctrl key is mapped to Qt::META)
  u->btnInsertSearchCurrent->setShortcut(QKeySequence(Qt::META+Qt::Key_Space));
#endif

  slotKlfConfigChanged();
}

void KLFLatexSymbols::retranslateUi(bool alsoBaseUi)
{
  if (alsoBaseUi)
    u->retranslateUi(this);
}

KLFLatexSymbols::~KLFLatexSymbols()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  KLFLatexSymbolsCache::saveTheCache();
}

void KLFLatexSymbols::emitInsertSymbol(const KLFLatexSymbol& symbol)
{
  emit insertSymbol(symbol);
}

void KLFLatexSymbols::slotKlfConfigChanged()
{
  pSearchBar->setEmacsStyleBackspace(klfconfig.UI.emacsStyleBackspaceSearch);
}


void KLFLatexSymbols::read_symbols_create_ui()
{
  klfDbgT("called.") ;

  // create our UI
  u->cbxCategory->clear();
  QGridLayout *lytstk = new QGridLayout(u->frmStackContainer);
  stkViews = new QStackedWidget(u->frmStackContainer);
  lytstk->addWidget(stkViews, 0, 0);

  mViews.clear();

  // find collection of XML files
  QStringList fxmllist;
  // in the following directories
  QStringList fxmldirs;
  fxmldirs << klfconfig.homeConfigDir + "/conf/latexsymbols.d/"
	   << klfconfig.globalShareDir + "/conf/latexsymbols.d/"
	   << ":/conf/latexsymbols.d/";

  klfDbgT("starting to collect XML files from dirs "<<fxmldirs) ;

  // collect all XML files
  int k, j;
  for (k = 0; k < fxmldirs.size(); ++k) {
    QDir fxmldir(fxmldirs[k]);
    QStringList xmllist = fxmldir.entryList(QStringList()<<"*.xml", QDir::Files);
    for (j = 0; j < xmllist.size(); ++j)
      fxmllist << fxmldir.absoluteFilePath(xmllist[j]);
  }
  klfDbgT("files collected: "<<fxmllist) ;
  //   if (fxmllist.isEmpty()) {
  //     // copy legacy XML file into the home latexsymbols.d directory
  //     QDir("/").mkpath(klfconfig.homeConfigDir+"/conf/latexsymbols.d");
  //     if (QFile::exists(klfconfig.homeConfigDir+"/latexsymbols.xml")) {
  //       QFile::copy(klfconfig.homeConfigDir+"/latexsymbols.xml",
  // 		  klfconfig.homeConfigDir+"/conf/latexsymbols.d/mylatexsymbols.xml");
  //       fxmllist << klfconfig.homeConfigDir+"/conf/latexsymbols.d/mylatexsymbols.xml";
  //     } else {
  //       QFile::copy(":/data/latexsymbols.xml",
  // 		  klfconfig.homeConfigDir+"/conf/latexsymbols.d/defaultlatexsymbols.xml");
  //       fxmllist << klfconfig.homeConfigDir+"/conf/latexsymbols.d/defaultlatexsymbols.xml";
  //     }
  //   }

  klfDbgT("got xml files, ensured not empty; fxmllist="<<fxmllist) ;

  // this will be a full list of symbols to feed to the cache
  QList<KLFLatexSymbol> allsymbols;

  // same indexes as in mViews[]
  QStringList categoryTitleLangs;

  // now read the file list
  for (k = 0; k < fxmllist.size(); ++k) {
    KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME+"/fxmllist["+('0'+k)+"]");
    klfDbg("reading XML file="<<fxmllist[k]);

    QString fn = fxmllist[k];
    QFile file(fn);
    if ( ! file.open(QIODevice::ReadOnly) ) {
      qWarning()<<KLF_FUNC_NAME<<": Error: Can't open latex symbols XML file "<<fn<<": "<<file.errorString()<<"!";
      continue;
    }

    QDomDocument doc("latexsymbols");
    QString errMsg; int errLine, errCol;
    bool r = doc.setContent(&file, false, &errMsg, &errLine, &errCol);
    if (!r) {
      qWarning()<<KLF_FUNC_NAME<<": Error parsing file "<<fn<<": "<<errMsg<<" at line "<<errLine<<", col "<<errCol;
      continue;
    }
    file.close();
    
    QDomElement root = doc.documentElement();
    if (root.nodeName() == "latexsymbollist") {
      if (!fn.endsWith("conf/latexsymbols.d/defaultlatexsymbols.xml"))
        klfWarning("Ignoring old symbols definition file "<<fn<<".");
      continue;
    }
    if (root.nodeName() != "klf-symbol-list") {
      qWarning("%s: Error parsing XML for latex symbols from file `%s': unexpected root tag `%s'.", KLF_FUNC_NAME,
	       qPrintable(fn), qPrintable(root.nodeName()));
      continue;
    }
    QString minklfversion = root.attribute("minklfversion", "3.3.0a");
    if (klfVersionCompare(minklfversion, KLF_VERSION_STRING) > 0) {
      qWarning("%s: ignoring XML latex symbols definition file `%s' which requires KLF version %s.",
	       KLF_FUNC_NAME, qPrintable(fn), qPrintable(minklfversion));
      continue;
    }

    QDomNode n;
    for (n = root.firstChild(); ! n.isNull(); n = n.nextSibling()) {
      QDomElement e = n.toElement(); // try to convert the node to an element.
      if ( e.isNull() || n.nodeType() != QDomNode::ElementNode )
	continue;
      if ( e.nodeName() != "category" ) {
	qWarning("WARNING in parsing XML : ignoring unexpected tag `%s'!\n",
		 qPrintable(e.nodeName()));
	continue;
      }
      // read category
      QString heading = e.attribute("name");
      QString categoryTitle;
      // xml:lang attribute for category title is obsolete, we use now Qt-linguist translated value...
      QString curCategoryTitleLang;
      QList<KLFLatexSymbol> l;
      QDomNode esym;
      for (esym = e.firstChild(); ! esym.isNull(); esym = esym.nextSibling() ) {
	if ( esym.isNull() || esym.nodeType() != QDomNode::ElementNode )
	  continue;
	QDomElement eesym = esym.toElement();
	klfDbg("read element "<<esym.nodeName());
	if ( eesym.nodeName() == "category-title" ) {
	  // xml:lang attribute for category title is obsolete, we use now Qt-linguist translated value...
	  QString lang = eesym.hasAttribute("xml:lang") ? eesym.attribute("xml:lang") : QString() ;
	  klfDbg("<category-title>: lang="<<lang<<"; hasAttribute(xml:lang)="<<eesym.hasAttribute("xml:lang")
		 <<"; current category-title="<<categoryTitle<<",lang="<<curCategoryTitleLang) ;
	  if (categoryTitle.isEmpty()) {
	    // no category title yet
	    if (lang.isEmpty() || lang.startsWith(klfconfig.UI.locale) || klfconfig.UI.locale().startsWith(lang)) {
	      // correct locale
	      categoryTitle = qApp->translate("xmltr_latexsymbols", eesym.text().toUtf8().constData(),
					      "[[tag: <category-title>]]", QCoreApplication::UnicodeUTF8);
	      curCategoryTitleLang = lang;
	    }
	    // otherwise skip this tag
	  } else {
	    // see if this locale is correct and more specific
	    if ( (lang.startsWith(klfconfig.UI.locale) || klfconfig.UI.locale().startsWith(lang)) &&
		 (curCategoryTitleLang.isEmpty() || lang.startsWith(curCategoryTitleLang) ) ) {
	      // then keep it and replace the other
	      categoryTitle = eesym.text();
	      curCategoryTitleLang = lang;
	    }
	    // otherwise skip this tag
	  }
	  continue;
	}
	if ( esym.nodeName() != "sym" ) {
	  qWarning("%s: WARNING in parsing XML : ignoring unexpected tag `%s' in category `%s'!\n",
		   KLF_FUNC_NAME, qPrintable(esym.nodeName()), qPrintable(heading));
	  continue;
	}
	// read symbol
	KLFLatexSymbol sym(eesym);
	l.append(sym);
	allsymbols.append(sym);
      }
      // and add this category, or append to existing category
      KLFLatexSymbolsView * view = NULL;
      for (j = 0; j < mViews.size(); ++j) {
	if (mViews[j]->category() == heading) {
	  view = mViews[j];
	  break;
	}
      }
      if (view == NULL) {
	// category does not yet exist
	view = new KLFLatexSymbolsView(heading, stkViews);
	connect(view, SIGNAL(symbolActivated(const KLFLatexSymbol&)),
		this, SIGNAL(insertSymbol(const KLFLatexSymbol&)));
	mViews.append(view);
	stkViews->addWidget(view);
	if (categoryTitle.isEmpty())
	  categoryTitle = heading;
	u->cbxCategory->addItem(categoryTitle, heading);
	categoryTitleLangs << curCategoryTitleLang;
      } else {
	// possibly update the title if a better translation is available
	if (!categoryTitle.isEmpty() &&
	    (categoryTitleLangs[j].isEmpty() || curCategoryTitleLang.startsWith(categoryTitleLangs[j]))) {
	  // update the title
	  u->cbxCategory->setItemText(j, categoryTitle);
	} else {
	  // keep old title
	}
      }

      view->appendSymbolList(l);
    } // iterate over categories in XML file
  } // iterate over XML files

  // pre-cache all our symbols
  KLFLatexSymbolsCache::theCache()->precacheList(allsymbols, true, this);

#ifdef KLF_DEBUG
  // and dump symbol cache
  KLFLatexSymbolsCache::theCache()->debugDump();
#endif

  int i;
  for (i = 0; i < mViews.size(); ++i) {
    mViews[i]->buildDisplay();
  }

}

void KLFLatexSymbols::slotShowCategory(int c)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  // called by combobox
  stkViews->setCurrentIndex(c);

  klfDbg("current index="<<c) ;
}

void KLFLatexSymbols::closeEvent(QCloseEvent *e)
{
  e->accept();
}

void KLFLatexSymbols::showEvent(QShowEvent *e)
{
  QWidget::showEvent(e);
}


bool KLFLatexSymbols::event(QEvent *e)
{
  if (e->type() == QEvent::Polish) {
    u->cbxCategory->setMinimumHeight(u->cbxCategory->sizeHint().height()+5);
  }
  if (e->type() == QEvent::KeyPress) {
    QKeyEvent *ke = (QKeyEvent*)e;
    if (ke->key() == Qt::Key_F7 && ke->modifiers() == 0) {
      hide();
      e->accept();
      return true;
    }
  }
  return QWidget::event(e);
}

