/***************************************************************************
 *   file klfstyle.cpp
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
/* $Id: klfstyle.cpp 911 2014-08-10 22:24:01Z phfaist $ */

#include <math.h>

#include <QColor>

#include <klfutil.h>
#include <klfdatautil.h>

#include "klfstyle.h"


KLF_DECLARE_POBJ_TYPE(KLFStyle);



KLFStyle::BBoxExpand::BBoxExpand(double t, double r, double b, double l)
  : KLFPropertizedObject("KLFStyle::BBoxExpand"),
    top(this, Top, "top", t), right(this, Right, "right", r),
    bottom(this, Bottom, "bottom", b), left(this, Left, "left", l)
{
}

KLFStyle::BBoxExpand::BBoxExpand(const BBoxExpand& c)
  : KLFPropertizedObject("KLFStyle::BBoxExpand"),
    top(this, Top, "top", c.top), right(this, Right, "right", c.right),
    bottom(this, Bottom, "bottom", c.bottom), left(this, Left, "left", c.left)
{
}

// -----


KLFStyle::KLFStyle(QString nm, unsigned long fgcol, unsigned long bgcol, const QString& mmode,
		   const QString& pre, int dotsperinch, const BBoxExpand& bb, const QString& us,
		   const QVariantMap& usinput)
  : KLFPropertizedObject("KLFStyle"),
    name(this, Name, "name", nm), fontname(this, FontName, "fontname", QString()),
    fg_color(this, FgColor, "fg_color", fgcol), bg_color(this, BgColor, "bg_color", bgcol),
    mathmode(this, MathMode, "mathmode", mmode), preamble(this, Preamble, "preamble", pre),
    fontsize(this, FontSize, "fontsize", -1),
    dpi(this, DPI, "dpi", dotsperinch), vectorscale(this, VectorScale, "vectorscale", 1.0),
    overrideBBoxExpand(this, OverrideBBoxExpand, "overrideBBoxExpand", bb),
    userScript(this, UserScript, "userScript", us),
    userScriptInput(this, UserScriptInput, "userScriptInput", usinput)
{
}
KLFStyle::KLFStyle(const KLFBackend::klfInput& input)
  : KLFPropertizedObject("KLFStyle"),
    name(this, Name, "name", QString()), fontname(this, FontName, "fontname", QString()),
    fg_color(this, FgColor, "fg_color", input.fg_color), bg_color(this, BgColor, "bg_color", input.bg_color),
    mathmode(this, MathMode, "mathmode", input.mathmode), preamble(this, Preamble, "preamble", input.preamble),
    fontsize(this, FontSize, "fontsize", input.fontsize),
    dpi(this, DPI, "dpi", input.dpi), vectorscale(this, VectorScale, "vectorscale", input.vectorscale),
    overrideBBoxExpand(this, OverrideBBoxExpand, "overrideBBoxExpand", BBoxExpand()),
    userScript(this, UserScript, "userScript", input.userScript),
    userScriptInput(this, UserScriptInput, "userScriptInput", klfMapToVariantMap<QString>(input.userScriptParam))
{
  klfDbg("Note: Possible loss of information: KLFStyle(KLFBackend::klfInput)") ;
}
KLFStyle::KLFStyle(const KLFStyle& o)
  : KLFPropertizedObject("KLFStyle"),
    name(this, Name, "name", o.name), fontname(this, FontName, "fontname", o.fontname),
    fg_color(this, FgColor, "fg_color", o.fg_color), bg_color(this, BgColor, "bg_color", o.bg_color),
    mathmode(this, MathMode, "mathmode", o.mathmode), preamble(this, Preamble, "preamble", o.preamble),
    fontsize(this, FontSize, "fontsize", o.fontsize),
    dpi(this, DPI, "dpi", o.dpi), vectorscale(this, VectorScale, "vectorscale", o.vectorscale),
    overrideBBoxExpand(this, OverrideBBoxExpand, "overrideBBoxExpand", o.overrideBBoxExpand),
    userScript(this, UserScript, "userScript", o.userScript),
    userScriptInput(this, UserScriptInput, "userScriptInput", o.userScriptInput)
{
}


QByteArray KLFStyle::typeNameFor(const QString& pname) const
{
  klfDbg("pname is "<<pname) ;
  if (pname == "name")  return "QString";
  if (pname == "fontname") return "QString";
  if (pname == "fg_color")  return "uint";
  if (pname == "bg_color")  return "uint";
  if (pname == "mathmode")  return "QString";
  if (pname == "preamble")  return "QString";
  if (pname == "fontsize")  return "double";
  if (pname == "dpi")  return "int";
  if (pname == "vectorscale") return "double";
  if (pname == "overrideBBoxExpand")  return "KLFStyle::BBoxExpand";
  if (pname == "userScript")  return "QString";
  if (pname == "userScriptInput")  return "QVariantMap";
  qWarning()<<KLF_FUNC_NAME<<": Unknown property name "<<pname;
  return QByteArray();
}



KLF_EXPORT QDataStream& operator<<(QDataStream& stream, const KLFStyle::BBoxExpand& b)
{
  return stream << b.top() << b.right() << b.bottom() << b.left();
}

KLF_EXPORT QDataStream& operator>>(QDataStream& stream, KLFStyle::BBoxExpand& x)
{
  double t, r, b, l;
  stream >> t >> r >> b >> l;
  x.top = t; x.right = r; x.bottom = b; x.left = l;
  return stream;
}


static QString preamble_append_overridebboxexpand(const KLFStyle::BBoxExpand& bb)
{
  if (!bb.valid())
    return QString();
  return "\n%%% KLF_overrideBBoxExpand: " + QString::fromLatin1(klfSave(&bb, QLatin1String("TextVariantMap")));
}
static QString preamble_append_userscript(const QString& us)
{
  if (us.isEmpty())
    return QString();
  return "\n%%% KLF_userScript: " + QString::fromLatin1(klfDataToEscaped(us.toUtf8()));
}
static QString preamble_append_userscript_input(const QVariantMap& usinput)
{
  if (usinput.isEmpty())
    return QString();
  klfDbg("adding userscriptinput="<<usinput) ;
  return "\n%%% KLF_userScriptInput: " +
    QString::fromLatin1(klfSaveVariantToText(QVariant(usinput), true));
}
static QString preamble_append_font_fontsize_vscale(const QString& fontname, double fontsize, double vscale)
{
  QString s;
  if (!fontname.isEmpty())
    s += "\n%%% KLF_fontname: " + QString::fromLatin1(klfDataToEscaped(fontname.toUtf8()));
  if (fontsize > 0.001)
    s += "\n%%% KLF_fontsize: " + QString("%1").arg(fontsize, 0, 'f', 2);
  if (fabs(vscale - 1.0) > 1e-5)
    s += "\n%%% KLF_vectorscale: " + QString("%1").arg(vscale, 0, 'f', 4);
  return s;
}

static void set_xtra_from_preamble(KLFStyle * style)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("style="<<*style) ;

  QRegExp rx = QRegExp("\n%%%\\s*KLF_([a-zA-Z0-9_]*):\\s*(\\S[^\n]*)?");
  QString p = style->preamble;
  int pos = 0;
  while ((pos = rx.indexIn(p, pos)) != -1) {
    QString what = rx.cap(1);
    QString value = rx.cap(2);
    klfDbg("KLFStyle: reading xtras: what="<<what<<"; value="<<value) ;
    if (what == "overrideBBoxExpand") {
      KLFStyle::BBoxExpand bb;
      bool ok = klfLoad(value.toLatin1(), &bb);
      klfDbg("bbox value string: "<<value) ;
      KLF_ASSERT_CONDITION(ok, "Failed to read bbox expand info: "<<value, pos += rx.matchedLength(); continue; ) ;
      style->overrideBBoxExpand = bb;
      p.replace(pos, rx.matchedLength(), "\n");
      ++pos;
      continue;
    }
    if (what == "userScript") {
      style->userScript = QString::fromUtf8(klfEscapedToData(value.toLatin1()));
      klfDbg("read user script: "<<style->userScript()) ;
      p.replace(pos, rx.matchedLength(), "");
      continue;
    }
    if (what == "userScriptInput") {
      style->userScriptInput = klfLoadVariantFromText(value.toLatin1(), "QVariantMap", "XML").toMap();
      klfDbg("user script input: "<<style->userScriptInput());
      p.replace(pos, rx.matchedLength(), "");
      continue;
    }
    if (what == "fontname") {
      style->fontname = QString::fromUtf8(klfEscapedToData(value.toLatin1()));
      klfDbg("font name: "<<(QString)style->fontname) ;
      p.replace(pos, rx.matchedLength(), "");
      continue;
    }
    if (what == "fontsize") {
      style->fontsize = value.toDouble();
      klfDbg("font size: "<<(double)style->fontsize) ;
      p.replace(pos, rx.matchedLength(), "");
      continue;
    }
    if (what == "vectorscale") {
      style->vectorscale = value.toDouble();
      klfDbg("vector scale: "<<(double)style->vectorscale) ;
      p.replace(pos, rx.matchedLength(), "");
      continue;
    }
    qWarning()<<KLF_FUNC_NAME<<": Warning: ignoring unknown preamble-xtra-information "<<what ;
    pos += rx.matchedLength();
  }
  style->preamble = p; // with the xtra info removed
}


KLF_EXPORT QDataStream& operator<<(QDataStream& stream, const KLFStyle& style)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  // yes, QIODevice inherits QObject and we can use dynamic properties...
  QString compat_klfversion = stream.device()->property("klfDataStreamAppVersion").toString();
  if (klfVersionCompare(compat_klfversion, "3.1") <= 0) {
    KLFStyle::KLFLegacyStyle oldstyle = KLFStyle::KLFLegacyStyle::fromNewStyle(style);
    klfDbg("saving style to stream: style="<<style<<"; thepreamble="<<oldstyle.preamble) ;
    return stream << oldstyle;
  } else if (klfVersionCompare(compat_klfversion, "3.3") < 0) {
    return stream << style.name() << (quint32)style.fg_color << (quint32)style.bg_color
		  << style.mathmode()
		  << (style.preamble + "\n" +
		      preamble_append_userscript(style.userScript) +
		      preamble_append_userscript_input(style.userScriptInput) +
		      preamble_append_font_fontsize_vscale(style.fontname, style.fontsize, style.vectorscale))
		  << (quint16)style.dpi
		  << style.overrideBBoxExpand();
  } else {
    KLFStyle sty = style;
    // use Binary Properties saver. XML is useless, we're saving to binary stream
    QByteArray props = klfSave(&sty, QLatin1String("Binary"));
    klfDbg("got data: "<<props) ;
    // and save the data as a QByteArray
    return stream << props;
  }
}
KLF_EXPORT QDataStream& operator>>(QDataStream& stream, KLFStyle& style)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME+"(QDataStream,KLFStyle&)") ;
  QString compat_klfversion = stream.device()->property("klfDataStreamAppVersion").toString();
  if (klfVersionCompare(compat_klfversion, "3.1") <= 0) {
    KLFStyle::KLFLegacyStyle oldstyle;
    stream >> oldstyle;
    style = oldstyle.toNewStyle();
    return stream;
  } else if (klfVersionCompare(compat_klfversion, "3.3") < 0) {
    quint32 fg, bg;
    quint16 dpi;
    QString name, mathmode, preamble;
    KLFStyle::BBoxExpand bb;
    stream >> name;
    stream >> fg >> bg >> mathmode >> preamble >> dpi;
    stream >> bb;
    style.name = name;
    style.mathmode = mathmode;
    style.preamble = preamble;
    style.fg_color = fg;
    style.bg_color = bg;
    style.fontsize = -1;
    style.dpi = dpi;
    style.vectorscale = 1.0;
    style.overrideBBoxExpand = bb;
    style.userScript = QString();
    style.userScriptInput = QVariantMap();
    set_xtra_from_preamble(&style);
    return stream;
  } else {
    // use Compressed XML
    QByteArray data;
    stream >> data;
    klfDbg("loading from data="<<data) ;
    bool loadOk = klfLoad(data, &style); // guessed format from magic header
    KLF_ASSERT_CONDITION(loadOk, "Failed to load style data", return stream; ) ;
    return stream;
  }
}

KLF_EXPORT bool operator==(const KLFStyle& a, const KLFStyle& b)
{
  return a.name == b.name &&
    a.fg_color == b.fg_color &&
    a.bg_color == b.bg_color &&
    a.mathmode == b.mathmode &&
    a.preamble == b.preamble &&
    a.fontname == b.fontname &&
    fabs(a.fontsize - b.fontsize) < 0.001 &&
    a.dpi == b.dpi &&
    fabs(a.vectorscale - b.vectorscale) < 0.001 &&
    a.overrideBBoxExpand == b.overrideBBoxExpand &&
    a.userScript == b.userScript &&
    a.userScriptInput == b.userScriptInput ;
}





KLFStyle::KLFLegacyStyle KLFStyle::KLFLegacyStyle::fromNewStyle(const KLFStyle& newStyle)
{
  KLFStyle::KLFLegacyStyle oldstyle;
  oldstyle.name = newStyle.name;
  oldstyle.fg_color = newStyle.fg_color;
  oldstyle.bg_color = newStyle.bg_color;
  oldstyle.mathmode = newStyle.mathmode;
  oldstyle.preamble = (newStyle.preamble + "\n" +
                       preamble_append_overridebboxexpand(newStyle.overrideBBoxExpand) +
                       preamble_append_userscript(newStyle.userScript) +
                       preamble_append_userscript_input(newStyle.userScriptInput) +
                       preamble_append_font_fontsize_vscale(newStyle.fontname, newStyle.fontsize,
                                                            newStyle.vectorscale));
  oldstyle.dpi = newStyle.dpi;
  return oldstyle;
}

KLFStyle KLFStyle::KLFLegacyStyle::toNewStyle() const
{
  KLFStyle style;
  style.name = name;
  style.mathmode = mathmode;
  style.preamble = preamble;
  style.fg_color = fg_color;
  style.bg_color = bg_color;
  style.fontsize = -1;
  style.dpi = dpi;
  style.vectorscale = 1.0;
  style.overrideBBoxExpand = KLFStyle::BBoxExpand();
  style.userScript = QString();
  style.userScriptInput = QVariantMap();
  set_xtra_from_preamble(&style);
  return style;
}



KLF_EXPORT QDataStream& operator<<(QDataStream& stream, const KLFStyle::KLFLegacyStyle& oldstyle)
{
  // this is compatible with KLF 3.1
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("saving oldstyle to stream: oldstyle="<<oldstyle<<"; thepreamble="<<oldstyle.preamble) ;
  return stream << oldstyle.name << (quint32)oldstyle.fg_color << (quint32)oldstyle.bg_color
		  << oldstyle.mathmode
		  << oldstyle.preamble
		  << (quint16)oldstyle.dpi;
}


KLF_EXPORT QDataStream& operator>>(QDataStream& stream, KLFStyle::KLFLegacyStyle& oldstyle)
{
  // this is compatible with KLF 3.1
  quint32 fg, bg;
  quint16 dpi;
  QString name, mathmode, preamble;
  stream >> name;
  stream >> fg >> bg >> mathmode >> preamble >> dpi;
  oldstyle.name = name;
  oldstyle.mathmode = mathmode;
  oldstyle.preamble = preamble;
  oldstyle.fg_color = fg;
  oldstyle.bg_color = bg;
  oldstyle.dpi = dpi;
  return stream;
}



KLF_EXPORT bool operator==(const KLFStyle::KLFLegacyStyle& a, const KLFStyle::KLFLegacyStyle& b)
{
  return a.name == b.name &&
    a.fg_color == b.fg_color &&
    a.bg_color == b.bg_color &&
    a.mathmode == b.mathmode &&
    a.preamble == b.preamble &&
    a.dpi == b.dpi;
}

KLF_EXPORT QDebug& operator<<(QDebug& stream, const KLFStyle::KLFLegacyStyle& ostyle)
{
  return stream << "KLFLegacyStyle("<<ostyle.name<<", "
                <<klfFmtCC("fg:%08lx bg:%08lx, ", ostyle.fg_color, ostyle.bg_color)
                <<ostyle.mathmode<<", "<<ostyle.preamble<<", "<<ostyle.dpi<<")";
}
