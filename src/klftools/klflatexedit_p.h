/***************************************************************************
 *   file klflatexedit_p.h
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
/* $Id: klflatexedit_p.h 828 2012-08-19 22:12:53Z phfaist $ */

#include "klflatexedit.h"


struct ParenItem {
  ParenItem(int i = -1, bool isopen = true, const QString& ps = QString(),
	    const QString& mod = QString())
    : isopening(isopen), pos(i), beginpos(i), endpos(i), highlight(false), parenstr(ps), modifier(mod)
  {
  }

  ParenItem(QList<int> ibeginposend, bool isopen = true, const QString& ps = QString(),
	    const QString& mod = QString())
    : isopening(isopen), highlight(false), parenstr(ps), modifier(mod)
  {
    int i = 0;
    beginpos = pos = endpos = -1;
    if (i < ibeginposend.size()) { beginpos = ibeginposend[i]; ++i; }
    if (i < ibeginposend.size()) { pos = ibeginposend[i]; ++i; }
    if (i < ibeginposend.size()) { endpos = ibeginposend[i]; }
    if (pos < beginpos) pos = beginpos;
    if (endpos < pos) endpos = pos;
  }

  int isopening;
  int pos;
  int beginpos;
  int endpos;
  bool highlight;
  QString parenstr;
  QString modifier; //!< possible modifier, eg. "\left" for "\left(", or "\" for "\{"

  inline int length() const { return endpos-beginpos; }
  inline int poslength() const { return endpos-pos; }
  inline int beginposlength() const { return endpos-beginpos; }

  inline int caretHoverPos() const
  {  return isopening ? pos : endpos;   }

  inline bool matches(const ParenItem& other)
  {
    if (isopening == other.isopening)
      return false;
    return isopening
      ? match(parenstr, modifier, other.parenstr, other.modifier)
      : match(other.parenstr, other.modifier, parenstr, modifier) ;
  }


  /** Returns TRUE if paren string \c a_ch matches the paren string \c b_ch with their respective modifiers.
   * a_ch/a_mod is expected to be an opening paren, and b_* a closing paren. Note that b_mod should be given
   * '\\right', not '\\left'.   */
  static bool match(const QString& a_ch, const QString& a_mod, const QString& b_ch, const QString& b_mod)
  {
    QString trbmod = b_mod;
    // translate modifiers
    int ind = ParsedBlock::parenSpecs.closeParenModifiers().indexOf(trbmod);
    if (ind >= 0) {
      trbmod = ParsedBlock::parenSpecs.openParenModifiers()[ind];
    }

    if (a_mod != trbmod) // if modifiers do not match, this is a mismatch.
      return false;

    if ( a_mod == "\\left" && (a_ch == "." || b_ch == ".") ) {
      // special case with \left( blablabla \right.  or  \left. blablabla \right)
      // so report a match
      return true;
    }
    QList<KLFLatexParenSpecs::ParenSpec> pslist = ParsedBlock::parenSpecs.parenSpecList();
    int k;
    for (k = 0; k < pslist.size() && k < pslist.size(); ++k) {
      if (a_ch == pslist[k].open)
	return  (b_ch == pslist[k].close);
      if (b_ch == pslist[k].close)
	return false; // because a_ch != openParenList[k]
    }
    if (a_ch == b_ch)
      return true;

    // otherwise, mismatch
    return false;
  }

private:
  typedef KLFLatexSyntaxHighlighter::ParsedBlock ParsedBlock;
};


struct LonelyParenItem : public ParenItem
{
  LonelyParenItem(const ParenItem& p = ParenItem(), int umpos = -1)
    : ParenItem(p), unmatchedpos(umpos) { }

  int unmatchedpos;

};









// -----------------------------



class KLFLatexEditPrivate : public QObject
{
  Q_OBJECT
public:
  KLF_PRIVATE_QOBJ_HEAD(KLFLatexEdit, QObject)
  {
    mDropHandler = NULL;
    pHeightHintLines = -1;
  }

  KLFLatexSyntaxHighlighter *mSyntaxHighlighter;

  /** This is used to open data if needed */
  KLFDropDataHandler *mDropHandler;

  int pHeightHintLines;

public slots:
  void slotInsertFromActionSender();

};
