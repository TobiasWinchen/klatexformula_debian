/***************************************************************************
 *   file klfprogerr.h
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2014 by Philippe Faist
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
/* $Id: klfprogerr.h 903 2014-08-10 02:15:11Z phfaist $ */


#ifndef KLFPROGERR_H
#define KLFPROGERR_H

#include <QWidget>
#include <QDialog>
#include <QTextEdit>
#include <QEvent>

#include <klfdefs.h>

namespace Ui {
  class KLFProgErr;
}

/** A dialog box suitable to display the output of a program, usually because it returned
 * some kind of error.
 */
class KLF_EXPORT KLFProgErr : public QDialog
{
  Q_OBJECT
public:
  KLFProgErr(QWidget *parent, QString errtext);
  virtual ~KLFProgErr();

  QTextEdit *textEditWidget();

  /** convenience static function */
  static void showError(QWidget *parent, QString text);

  /** Attempt to extract a short version of an error in LaTeX output.
   *
   * \c str should be some LaTeX output with an error. If we weren't able to extract
   * anything, the full string is returned untouched.
   */
  static QString extractLatexError(const QString& str);

protected:
  virtual void showEvent(QShowEvent *e);

private:
  Ui::KLFProgErr *u;
};



#endif
