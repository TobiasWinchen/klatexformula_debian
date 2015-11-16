/***************************************************************************
 *   file klflibentryeditor.h
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
/* $Id: klflibentryeditor.h 698 2011-08-08 08:28:12Z phfaist $ */

#ifndef KLFLIBENTRYEDITOR_H
#define KLFLIBENTRYEDITOR_H

#include <QWidget>
#include <QComboBox>

#include <klflib.h>



namespace Ui { class KLFLibEntryEditor; }

class KLF_EXPORT KLFLibEntryEditor : public QWidget
{
  Q_OBJECT
public:
  KLFLibEntryEditor(QWidget *parent = NULL);
  virtual ~KLFLibEntryEditor();

  void addCategorySuggestions(const QStringList& categorylist);

  virtual bool eventFilter(QObject *object, QEvent *event);

  inline bool metaInfoModified() const { return pMetaInfoModified; }

signals:

  /** Emitted when user clicks the "Apply" button after having edited category/tags, or hits return.
   * The \c props are given as a map of KLFLibEntry-property IDs with their corresponding new values.
   */
  void metaInfoChanged(const QMap<int,QVariant>& props);

  void restoreStyle(const KLFStyle& style);

public slots:

  void displayEntry(const KLFLibEntry& entry);

  /** Handles the selection of multiple items properly by displaying
   * [Multiple items selected] whenever needed. */
  void displayEntries(const QList<KLFLibEntry>& entries);

  /** Globally enable or disable input. Individual fields may still be disabled
   * for example if more than one entry is selected.
   *
   * \warning Changes take effect upon next call of \ref displayEntries() or
   *   \ref displayEntry() */
  void setInputEnabled(bool enabled);

  void retranslateUi(bool alsoBaseUi = true);

  void slotCopy();

protected slots:

  void slotUpdateFromCbx(QComboBox *cbx);

  void on_btnApplyChanges_clicked();
  void on_btnRestoreStyle_clicked();
  /** Updates the changes done to category and tags, for which the corresponding argument is \c true.
   * eg. <tt>slotApplyChanges(true,false)</tt> will update category but not tags.
   */
  void slotApplyChanges() { slotApplyChanges(true, true); }
  void slotApplyChanges(bool category, bool tags);

  void slotModified(bool modif = true);

  void slotCbxSaveCurrentCompletion(QComboBox *cbx);
  void slotCbxCleanUpCompletions(QComboBox *cbx);

private:
  Ui::KLFLibEntryEditor *u;

  bool pInputEnabled;

  bool pMetaInfoModified;

  KLFStyle pCurrentStyle;

  //  void updateEditText(QComboBox *editWidget, const QString& newText);
  void displayStyle(bool valid, const KLFStyle& style);
};







#endif
