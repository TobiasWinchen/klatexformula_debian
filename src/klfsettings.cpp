/***************************************************************************
 *   file klfsettings.cpp
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
/* $Id: klfsettings.cpp 603 2011-02-26 23:14:55Z phfaist $ */

#include <stdlib.h>

#include <QDialog>
#include <QCheckBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QFontDatabase>
#include <QFontDialog>
#include <QString>
#include <QMessageBox>
#include <QFileInfo>
#include <QDir>
#include <QFileDialog>
#include <QWhatsThis>
#include <QResource>
#include <QEvent>
#include <QListView> // QListView::LeftToRight|TopToBottom
#include <QMouseEvent>
#include <QDesktopServices>

#include <klfcolorchooser.h>
#include <klfpathchooser.h>

#include <klfbackend.h>

#include <ui_klfsettings.h>

#include "klfmain.h"
#include "klfmainwin.h"
#include "klfconfig.h"
#include "klfmime.h"
#include "klfpluginiface.h"
#include "klflatexedit.h"
#include "klfsettings.h"


#define KLFSETTINGS_ROLE_PLUGNAME (Qt::UserRole + 5300)
#define KLFSETTINGS_ROLE_PLUGINDEX (KLFSETTINGS_ROLE_PLUGNAME + 1)

#define KLFSETTINGS_ROLE_ADDONINDEX (Qt::UserRole + 5400)


#define REG_SH_TEXTFORMATENSEMBLE(x) \
  _textformats.append( TextFormatEnsemble( & klfconfig.SyntaxHighlighter.fmt##x , \
					   u->colSH##x, u->colSH##x##Bg , u->chkSH##x##B , u->chkSH##x##I ) );




KLFSettings::KLFSettings(KLFMainWin* parent)
  : QDialog(parent)
{
  u = new Ui::KLFSettings;
  u->setupUi(this);
  setObjectName("KLFSettings");

  _mainwin = parent;

  pUserSetDefaultAppFont = false;

  u->cbxLibIconViewFlow->setEnumValues(QList<int>()<<QListView::TopToBottom<<QListView::LeftToRight,
				       QStringList()<<tr("Top to Bottom")<<tr("Left to Right"));

  reset();

  u->btns->clear();

  //   QPushButton *b;
  //   b = new QPushButton(QIcon(":/pics/closehide.png"), QString("cancel"), u->btns);
  //   u->btns->addButton(b, QDialogButtonBox::RejectRole);
  //   connect(b, SIGNAL(clicked()), this, SLOT(reject()));
  //   b = new QPushButton(QIcon(":/pics/apply.png"), QString("apply"), u->btns);
  //   u->btns->addButton(b, QDialogButtonBox::ApplyRole);
  //   connect(b, SIGNAL(clicked()), this, SLOT(apply()));
  //   b = new QPushButton(QIcon(":/pics/ok.png"), QString("ok"), u->btns);
  //   u->btns->addButton(b, QDialogButtonBox::AcceptRole);
  //   connect(b, SIGNAL(clicked()), this, SLOT(accept()));
  QAbstractButton *b;
  b = u->btns->addButton(QDialogButtonBox::Cancel);
  b->setIcon(QIcon(":/pics/closehide.png"));
  connect(b, SIGNAL(clicked()), this, SLOT(reject()));
  b = u->btns->addButton(QDialogButtonBox::Apply);
  b->setIcon(QIcon(":/pics/apply.png"));
  connect(b, SIGNAL(clicked()), this, SLOT(apply()));
  b = u->btns->addButton(QDialogButtonBox::Ok);
  b->setIcon(QIcon(":/pics/ok.png"));
  connect(b, SIGNAL(clicked()), this, SLOT(accept()));

  populateLocaleCombo();
  populateExportProfilesCombos();

  // set some smaller fonts for small titles
  QFont f = this->font();
  f.setPointSize(QFontInfo(f).pointSize() - 1);
  u->lblSHForeground->setFont(f);
  u->lblSHBackground->setFont(f);

  connect(u->btnPathsReset, SIGNAL(clicked()), this, SLOT(setDefaultPaths()));

  connect(u->lstPlugins, SIGNAL(itemSelectionChanged()), this, SLOT(refreshPluginSelected()));
  connect(u->lstAddOns, SIGNAL(itemSelectionChanged()), this, SLOT(refreshAddOnSelected()));
  connect(u->btnImportAddOn, SIGNAL(clicked()), this, SLOT(importAddOn()));
  connect(u->btnRemoveAddOn, SIGNAL(clicked()), this, SLOT(removeAddOn()));
  //  connect(u->btnRemovePlugin, SIGNAL(clicked()), this, SLOT(removePlugin()));

  u->lstPlugins->installEventFilter(this);
  u->lstPlugins->viewport()->installEventFilter(this);
  u->lstAddOns->installEventFilter(this);
  u->lstAddOns->viewport()->installEventFilter(this);

  connect(u->btnAppFont, SIGNAL(clicked()), this, SLOT(slotChangeFontSender()));
  connect(u->btnEditorFont, SIGNAL(clicked()), this, SLOT(slotChangeFontSender()));
  connect(u->btnPreambleFont, SIGNAL(clicked()), this, SLOT(slotChangeFontSender()));

  // prepare some actions as shortcuts for standard fonts
  QFontDatabase fdb;
  u->aFontCMU->setEnabled( fdb.families().contains("CMU Sans Serif") );
  pFontBasePresetActions["CMU"] = u->aFontCMU;
  pFontBasePresetActions["TT"] = u->aFontTT;
  pFontBasePresetActions["Std"] = u->aFontStd;
  pFontButtons["AppFont"] = u->btnAppFont;
  pFontButtons["EditorFont"] = u->btnEditorFont;
  pFontButtons["PreambleFont"] = u->btnPreambleFont;
  QAction *a = NULL;
  QMenu *fontPresetMenu = NULL;
  QVariantMap vmap;
  // remember: action text/icon/font/... is set in retranslateUi().
  // -- AppFont --
  fontPresetMenu = new QMenu(this);
  a = new QAction(this);
  vmap["Action"] = "CMU";
  vmap["Font"] = klfconfig.defaultCMUFont;
  vmap["Button"] = QVariant("AppFont");
  a->setData(QVariant(vmap));
  a->setEnabled(u->aFontCMU->isEnabled());
  fontPresetMenu->addAction(a);
  connect(a, SIGNAL(triggered()), this, SLOT(slotChangeFontPresetSender()));
  pFontSetActions << a;
  a = new QAction(this);
  vmap["Action"] = "Std";
  vmap["Font"] = klfconfig.defaultStdFont;
  vmap["Button"] = QVariant("AppFont");
  vmap["isSystemDefaultAppFont"] = QVariant(true);
  a->setData(QVariant(vmap));
  connect(a, SIGNAL(triggered()), this, SLOT(slotChangeFontPresetSender()));
  pFontSetActions << a;
  fontPresetMenu->addAction(a);
  u->btnAppFontChoose->setMenu(fontPresetMenu);
  // -- EditorFont --
  fontPresetMenu = new QMenu(this);
  a = new QAction(this);
  vmap["Action"] = "TT";
  vmap["Font"] = klfconfig.defaultTTFont;
  vmap["Button"] = QVariant("EditorFont");
  a->setData(QVariant(vmap));
  connect(a, SIGNAL(triggered()), this, SLOT(slotChangeFontPresetSender()));
  pFontSetActions << a;
  fontPresetMenu->addAction(a);
  a = new QAction(this);
  vmap["Action"] = "CMU";
  vmap["Font"] = klfconfig.defaultCMUFont;
  vmap["Button"] = QVariant("EditorFont");
  a->setData(QVariant(vmap));
  a->setEnabled(u->aFontCMU->isEnabled());
  connect(a, SIGNAL(triggered()), this, SLOT(slotChangeFontPresetSender()));
  pFontSetActions << a;
  fontPresetMenu->addAction(a);
  a = new QAction(this);
  vmap["Action"] = "Std";
  vmap["Font"] = klfconfig.defaultStdFont;
  vmap["Button"] = QVariant("EditorFont");
  a->setData(QVariant(vmap));
  connect(a, SIGNAL(triggered()), this, SLOT(slotChangeFontPresetSender()));
  pFontSetActions << a;
  fontPresetMenu->addAction(a);
  u->btnEditorFontChoose->setMenu(fontPresetMenu);
  // -- PreambleFont --
  fontPresetMenu = new QMenu(this);
  a = new QAction(this);
  vmap["Action"] = "TT";
  vmap["Font"] = klfconfig.defaultTTFont;
  vmap["Button"] = QVariant("PreambleFont");
  a->setData(QVariant(vmap));
  connect(a, SIGNAL(triggered()), this, SLOT(slotChangeFontPresetSender()));
  pFontSetActions << a;
  fontPresetMenu->addAction(a);
  a = new QAction(this);
  vmap["Action"] = "CMU";
  vmap["Font"] = klfconfig.defaultCMUFont;
  vmap["Button"] = QVariant("PreambleFont");
  a->setData(QVariant(vmap));
  a->setEnabled(u->aFontCMU->isEnabled());
  connect(a, SIGNAL(triggered()), this, SLOT(slotChangeFontPresetSender()));
  pFontSetActions << a;
  fontPresetMenu->addAction(a);
  a = new QAction(this);
  vmap["Action"] = "Std";
  vmap["Font"] = klfconfig.defaultStdFont;
  vmap["Button"] = QVariant("PreambleFont");
  a->setData(QVariant(vmap));
  connect(a, SIGNAL(triggered()), this, SLOT(slotChangeFontPresetSender()));
  pFontSetActions << a;
  fontPresetMenu->addAction(a);
  u->btnPreambleFontChoose->setMenu(fontPresetMenu);


  REG_SH_TEXTFORMATENSEMBLE(Keyword);
  REG_SH_TEXTFORMATENSEMBLE(Comment);
  REG_SH_TEXTFORMATENSEMBLE(ParenMatch);
  REG_SH_TEXTFORMATENSEMBLE(ParenMismatch);
  REG_SH_TEXTFORMATENSEMBLE(LonelyParen);

  u->btnImportAddOn->setEnabled(klf_addons_canimport);
  u->btnRemoveAddOn->setEnabled(klf_addons_canimport);

  refreshAddOnList();
  refreshAddOnSelected();
  refreshPluginSelected();


  // remove default Qt Designer Page
  QWidget * w = u->tbxPluginsConfig->widget(u->tbxPluginsConfig->currentIndex());
  u->tbxPluginsConfig->removeItem(u->tbxPluginsConfig->currentIndex());
  delete w;

  u->lstPlugins->setColumnWidth(0, 185);

  // dont load plugin data here as this dialog is created BEFORE plugins are loaded
  _pluginstuffloaded = false;

  retranslateUi(false);
}

void KLFSettings::retranslateUi(bool alsoBaseUi)
{
  if (alsoBaseUi)
    u->retranslateUi(this);

  // translate our preset actions
  int k;
  for (k = 0; k < pFontSetActions.size(); ++k) {
    QAction *a = pFontSetActions[k];
    QVariantMap vmap = a->data().toMap();
    QString refAKey = vmap["Action"].toString();
    KLF_ASSERT_CONDITION(pFontBasePresetActions.contains(refAKey),
			 "Base Reference Preset Action not found: "<<refAKey<<" ?!?",
			 continue ) ;
    QAction *refA = pFontBasePresetActions[refAKey];
    a->setText(refA->text());
    a->setIcon(refA->icon());
    a->setToolTip(refA->toolTip());
    QFont f = vmap["Font"].value<QFont>();
    a->setFont(f);
  }
}


KLFSettings::~KLFSettings()
{
  delete u;
}

void KLFSettings::populateLocaleCombo()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  u->cbxLocale->clear();
  // application language : populate combo box
  u->cbxLocale->addItem( QLatin1String("English Default"), QVariant::fromValue<QString>("en_US") );
  int k;
  for (k = 0; k < klf_avail_translations.size(); ++k) {
    KLFTranslationInfo ti = klf_avail_translations[k];
    u->cbxLocale->addItem( ti.translatedname, QVariant(ti.localename) );
    klfDbg("Added translation "<< ti.translatedname <<" ("<<ti.localename<<")") ;
  }

  // Select the current locale. This is also done in reset(), but these lines are needed here
  // for when this function is called within importAddOn().
  k = u->cbxLocale->findData(klfconfig.UI.locale);
  if (k == -1) {
    k = 0;
  }
  u->cbxLocale->setCurrentIndex(k);
}

void KLFSettings::populateExportProfilesCombos()
{
  QList<KLFMimeExportProfile> eplist = KLFMimeExportProfile::exportProfileList();

  u->cbxCopyExportProfile->clear();
  u->cbxDragExportProfile->clear();
  int k;
  for (k = 0; k < eplist.size(); ++k) {
    u->cbxCopyExportProfile->addItem(eplist[k].description(), QVariant(eplist[k].profileName()));
    u->cbxDragExportProfile->addItem(eplist[k].description(), QVariant(eplist[k].profileName()));
  }
}


void KLFSettings::show()
{
  klfDbg("show called.") ;
  populateExportProfilesCombos();

  reset();

  if (!_pluginstuffloaded)
    initPluginControls();
  else
    resetPluginControls();

  QDialog::show();
}


/** \internal */
#define __KLF_SHOW_SETTINGS_CONTROL( tab , focuswidget )	\
  u->tabs->setCurrentWidget( u->tab );				\
  u->focuswidget->setFocus(Qt::OtherFocusReason);

void KLFSettings::showControl(int control)
{
  switch (control) {
  case AppLanguage:
    __KLF_SHOW_SETTINGS_CONTROL(tabAppearance, cbxLocale) ;
    break;
  case AppFonts:
    __KLF_SHOW_SETTINGS_CONTROL(tabAppearance, btnAppFont) ;
    break;
  case Preview:
    __KLF_SHOW_SETTINGS_CONTROL(tabAppearance, chkEnableRealTimePreview) ;
    break;
  case TooltipPreview:
    __KLF_SHOW_SETTINGS_CONTROL(tabAppearance, chkEnableToolTipPreview) ;
    break;
  case SyntaxHighlighting:
    __KLF_SHOW_SETTINGS_CONTROL(tabSyntaxHighlighting, chkSHEnable) ;
    break;
  case ExecutablePaths:
    __KLF_SHOW_SETTINGS_CONTROL(tabAdvanced, pathTempDir) ;
    break;
  case ExpandEPSBBox:
    __KLF_SHOW_SETTINGS_CONTROL(tabAdvanced, spnLBorderOffset) ;
    break;
  case ExportProfiles:
    __KLF_SHOW_SETTINGS_CONTROL(tabAdvanced, cbxCopyExportProfile) ;
    break;
  case LibrarySettings:
    __KLF_SHOW_SETTINGS_CONTROL(tabLibBrowser, chkLibRestoreURLs) ;
    break;
  case ManageAddOns:
    __KLF_SHOW_SETTINGS_CONTROL(tabAddOns, lstAddOns) ;
    break;
  case ManagePlugins:
    __KLF_SHOW_SETTINGS_CONTROL(tabAddOns, lstPlugins) ;
    break;
  case PluginsConfig:
    __KLF_SHOW_SETTINGS_CONTROL(tabPlugins, tbxPluginsConfig->currentWidget()) ;
    break;
  default:
    qWarning()<<KLF_FUNC_NAME<<": unknown control number requested : "<<control;
  }
}

/** \internal */
#define __KLF_SETTINGS_TEST_STR_CONTROL( controlName, controlNum )	\
  if (controlName == QLatin1String(#controlNum)) {			\
    showControl(controlNum);						\
    return;								\
  }

void KLFSettings::showControl(const QString& controlName)
{
  __KLF_SETTINGS_TEST_STR_CONTROL( controlName, AppLanguage ) ;
  __KLF_SETTINGS_TEST_STR_CONTROL( controlName, AppFonts ) ;
  __KLF_SETTINGS_TEST_STR_CONTROL( controlName, Preview ) ;
  __KLF_SETTINGS_TEST_STR_CONTROL( controlName, TooltipPreview ) ;
  __KLF_SETTINGS_TEST_STR_CONTROL( controlName, SyntaxHighlighting ) ;
  __KLF_SETTINGS_TEST_STR_CONTROL( controlName, ExecutablePaths ) ;
  __KLF_SETTINGS_TEST_STR_CONTROL( controlName, ExpandEPSBBox ) ;
  __KLF_SETTINGS_TEST_STR_CONTROL( controlName, LibrarySettings ) ;
  __KLF_SETTINGS_TEST_STR_CONTROL( controlName, ManageAddOns ) ;
  __KLF_SETTINGS_TEST_STR_CONTROL( controlName, ManagePlugins ) ;
  __KLF_SETTINGS_TEST_STR_CONTROL( controlName, PluginsConfig ) ;
}

static bool treeMaybeUnselect(QTreeWidget *tree, QEvent *event)
{
  // tree is non-NULL as ensured by caller.

  if (event->type() != QEvent::MouseButtonPress)
    return false;
  QMouseEvent * e = (QMouseEvent*) event;
  if (e->button() != Qt::LeftButton)
    return false;
  QTreeWidgetItem *itemAtClick = tree->itemAt(e->pos());
  if ( itemAtClick ) {
    // user clicked on an item, let Qt handle the event and select item etc.
    return false;
  }
  // user clicked out of an item, change Qt's default behavior and un-select all items.
  QList<QTreeWidgetItem*> selitems = tree->selectedItems();
  int k;
  for (k = 0; k < selitems.size(); ++k) {
    selitems[k]->setSelected(false);
  }
  return true;
}

bool KLFSettings::eventFilter(QObject *object, QEvent *event)
{
  // test for one the the treeWidgets
  QTreeWidget * tree = NULL;
  if (object == u->lstPlugins || object == u->lstPlugins->viewport())
    tree = u->lstPlugins;
  if (object == u->lstAddOns || object == u->lstAddOns->viewport())
    tree = u->lstAddOns;
  
  if ( tree )
    if ( treeMaybeUnselect(tree, event) )
      return true;
  return QDialog::eventFilter(object, event);
}

void KLFSettings::reset()
{
  int k;
  KLFBackend::klfSettings s = _mainwin->currentSettings();

  k = u->cbxLocale->findData(klfconfig.UI.locale);
  if (k == -1) {
    k = 0;
  }
  u->cbxLocale->setCurrentIndex(k);

  u->pathTempDir->setPath(QDir::toNativeSeparators(s.tempdir));
  u->pathLatex->setPath(s.latexexec);
  u->pathDvips->setPath(s.dvipsexec);
  u->pathGs->setPath(s.gsexec);
  u->pathEpstopdf->setPath(s.epstopdfexec);
  u->chkEpstopdf->setChecked( ! s.epstopdfexec.isEmpty() );
  /** \todo .... these settings should be shown in double when the corresponding
   * functionality will be implemented in klfbackend. */
  u->spnLBorderOffset->setValue( (int)(s.lborderoffset+0.5) );
  u->spnTBorderOffset->setValue( (int)(s.tborderoffset+0.5) );
  u->spnRBorderOffset->setValue( (int)(s.rborderoffset+0.5) );
  u->spnBBorderOffset->setValue( (int)(s.bborderoffset+0.5) );
  u->chkOutlineFonts->setChecked( s.outlineFonts );

  u->chkSHEnable->setChecked(klfconfig.SyntaxHighlighter.configFlags
			  &  KLFLatexSyntaxHighlighter::Enabled);
  u->chkSHHighlightParensOnly->setChecked(klfconfig.SyntaxHighlighter.configFlags
				       &  KLFLatexSyntaxHighlighter::HighlightParensOnly);
  u->chkSHHighlightLonelyParen->setChecked(klfconfig.SyntaxHighlighter.configFlags
					&  KLFLatexSyntaxHighlighter::HighlightLonelyParen);

  for (k = 0; k < _textformats.size(); ++k) {
    if (_textformats[k].fmt->hasProperty(QTextFormat::ForegroundBrush))
      _textformats[k].fg->setColor(_textformats[k].fmt->foreground().color());
    else
      _textformats[k].fg->setColor(QColor());
    if (_textformats[k].fmt->hasProperty(QTextFormat::BackgroundBrush))
      _textformats[k].bg->setColor(_textformats[k].fmt->background().color());
    else
      _textformats[k].bg->setColor(QColor());
    if (_textformats[k].fmt->hasProperty(QTextFormat::FontWeight))
      _textformats[k].chkB->setChecked(_textformats[k].fmt->fontWeight() > 60);
    else
      _textformats[k].chkB->setCheckState(Qt::PartiallyChecked);
    if (_textformats[k].fmt->hasProperty(QTextFormat::FontItalic))
      _textformats[k].chkI->setChecked(_textformats[k].fmt->fontItalic());
    else
      _textformats[k].chkI->setCheckState(Qt::PartiallyChecked);
  }

  pUserSetDefaultAppFont = klfconfig.UI.useSystemAppFont;
  u->btnAppFont->setFont(klfconfig.UI.applicationFont);
  u->btnAppFont->setProperty("selectedFont", QVariant(klfconfig.UI.applicationFont));
  u->btnEditorFont->setFont(klfconfig.UI.latexEditFont);
  u->btnEditorFont->setProperty("selectedFont", QVariant(klfconfig.UI.latexEditFont));
  u->btnPreambleFont->setFont(klfconfig.UI.preambleEditFont);
  u->btnPreambleFont->setProperty("selectedFont", QVariant(klfconfig.UI.preambleEditFont));

  u->chkEnableRealTimePreview->setChecked(klfconfig.UI.enableRealTimePreview);
  u->spnPreviewWidth->setValue(klfconfig.UI.labelOutputFixedSize.width());
  u->spnPreviewHeight->setValue(klfconfig.UI.labelOutputFixedSize.height());

  u->chkEnableToolTipPreview->setChecked(klfconfig.UI.enableToolTipPreview);
  u->spnToolTipMaxWidth->setValue(klfconfig.UI.previewTooltipMaxSize.width());
  u->spnToolTipMaxHeight->setValue(klfconfig.UI.previewTooltipMaxSize.height());

  u->chkShowHintPopups->setChecked(klfconfig.UI.showHintPopups);
  u->chkClearLatexOnly->setChecked(klfconfig.UI.clearLatexOnly);
  u->chkGlowEffect->setChecked(klfconfig.UI.glowEffect);

  int copyi = u->cbxCopyExportProfile->findData(QVariant(klfconfig.UI.copyExportProfile));
  u->cbxCopyExportProfile->setCurrentIndex(copyi);
  int dragi = u->cbxDragExportProfile->findData(QVariant(klfconfig.UI.dragExportProfile));
  u->cbxDragExportProfile->setCurrentIndex(dragi);
  //  u->chkShowExportProfilesLabel->setChecked(klfconfig.UI.showExportProfilesLabel);
  u->chkMenuExportProfileAffectsDrag->setChecked(klfconfig.UI.menuExportProfileAffectsDrag);
  u->chkMenuExportProfileAffectsCopy->setChecked(klfconfig.UI.menuExportProfileAffectsCopy);

  u->chkLibRestoreURLs->setChecked(klfconfig.LibraryBrowser.restoreURLs);
  u->chkLibConfirmClose->setChecked(klfconfig.LibraryBrowser.confirmClose);
  u->chkLibHistoryTagCopyToArchive->setChecked(klfconfig.LibraryBrowser.historyTagCopyToArchive);
  //  u->chkLibGroupSubCategories->setChecked(klfconfig.LibraryBrowser.groupSubCategories);
  u->cbxLibIconViewFlow->setSelectedValue(klfconfig.LibraryBrowser.iconViewFlow);
}


void KLFSettings::initPluginControls()
{
  if (_pluginstuffloaded)
    return;
  _pluginstuffloaded = true;

  int j;
  int n_pluginconfigpages = 0;
  QTreeWidgetItem *litem;
  for (j = 0; j < klf_plugins.size(); ++j) {
    QString name = klf_plugins[j].name;
    QString title = klf_plugins[j].title;
    QString description = klf_plugins[j].description;
    KLFPluginGenericInterface *instance = klf_plugins[j].instance;
    
    litem = new QTreeWidgetItem(u->lstPlugins);
    litem->setCheckState(0,
			 klfconfig.Plugins.pluginConfig[name]["__loadenabled"].toBool() ?
			 Qt::Checked : Qt::Unchecked);
    litem->setText(0, title);
    
    litem->setData(0, KLFSETTINGS_ROLE_PLUGNAME, name);
    litem->setData(0, KLFSETTINGS_ROLE_PLUGINDEX, j);

    mPluginListItems[name] = litem;

    if ( instance != NULL ) {
      mPluginConfigWidgets[name] = instance->createConfigWidget( NULL );
      u->tbxPluginsConfig->addItem( mPluginConfigWidgets[name] , QIcon(":/pics/bullet22.png"), title );
      KLFPluginConfigAccess pconfa = klfconfig.getPluginConfigAccess(name);
      instance->loadFromConfig(mPluginConfigWidgets[name], &pconfa);
      n_pluginconfigpages++;
    }
  }
  if (n_pluginconfigpages == 0) {
    QLabel * lbl;
    lbl = new QLabel(tr("No Plugins have been loaded. Please install and enable individual plugins "
			"first, then come back to this page to configure them."), u->tbxPluginsConfig);
    lbl->hide();
    lbl->setWordWrap(true);
    lbl->setMargin(20);
    u->tbxPluginsConfig->addItem(lbl, tr("No Plugins Loaded"));
  }
}

void KLFSettings::resetPluginControls()
{
  // go through all plugins, and load their configs into their corresponding config widget
  // and see if they are loaded (corresponding checkbox)
  int k;
  for (k = 0; k < klf_plugins.size(); ++k) {
    QString name = klf_plugins[k].name;
    KLFPluginGenericInterface *instance = klf_plugins[k].instance;

    KLF_ASSERT_CONDITION(mPluginListItems.contains(name),
			 "Plugin "<<name<<" does not have its corresponding check item!",
			 continue ;) ;

    mPluginListItems[name]->setCheckState(0,
					  klfconfig.Plugins.pluginConfig[name]["__loadenabled"].toBool() ?
					  Qt::Checked : Qt::Unchecked);

    if (instance != NULL) {
      if (!mPluginConfigWidgets.contains(name)) {
	qWarning()<<KLF_FUNC_NAME<<": Plugin "<<name<<" does not have its config widget !?!?!";
	continue;
      }
      QWidget *widget = mPluginConfigWidgets[name];
      // load the config into the widget
      KLFPluginConfigAccess pconfa = klfconfig.getPluginConfigAccess(name);
      instance->loadFromConfig(widget, &pconfa);
    }
  }
}

void KLFSettings::refreshPluginSelected()
{
  QList<QTreeWidgetItem*> sel = u->lstPlugins->selectedItems();
  if (sel.size() != 1) {
    //    u->btnRemovePlugin->setEnabled(false);
    u->lblPluginInfo->setText("");
    return;
  }
  int k = sel[0]->data(0, KLFSETTINGS_ROLE_PLUGINDEX).toInt();
  if (k < 0 || k >= klf_plugins.size()) {
    //    u->btnRemovePlugin->setEnabled(false);
    u->lblPluginInfo->setText("");
    return;
  }

  //  u->btnRemovePlugin->setEnabled(true);
  int smallpointsize = QFontInfo(font()).pointSize() - 1;
  u->lblPluginInfo->setText(tr("<p style=\"-qt-block-indent: 0; text-indent: 0px; margin-bottom: 0px;\">\n"
			       "<tt>Name:</tt> <span style=\"font-weight:600;\">%1</span><br />\n"
			       "<tt>Author:</tt> <span style=\"font-weight:600;\">%2</span><br />\n"
			       "<tt>Description:</tt></p>\n"
			       "<p style=\"font-weight: 600; margin-top: 2px; margin-left: 25px;"
			       "   margin-bottom: 0px;\">%3</p>\n"
			       "<p style=\"-qt-block-indent: 0; text-indent: 0px; margin-top: 2px;\">\n"
			       "<tt>File Location:</tt> <span style=\"font-size: %4pt;\">%5</span>\n")
			    .arg(Qt::escape(klf_plugins[k].title)).arg(Qt::escape(klf_plugins[k].author))
			    .arg(Qt::escape(klf_plugins[k].description))
			    .arg(smallpointsize)
			    .arg(Qt::escape(QDir::toNativeSeparators(QFileInfo(klf_plugins[k].fpath)
								     .canonicalFilePath())))
			    );
}

void KLFSettings::removePlugin()
{
  // THIS FUNCTION IS NO LONGER USED. PLUGINS ARE AUTOMATICALLY REMOVED WHEN THE CORRESPONDING
  // ADD-ON IS REMOVED. THIS FUNCTION IS KEPT IN CASE I CHANGE SOMETHING IN THE FUTURE.

  QList<QTreeWidgetItem*> sel = u->lstPlugins->selectedItems();
  if (sel.size() != 1) {
    qWarning("KLFSettings::removePlugin: No Selection or many selection");
    return;
  }
  QTreeWidgetItem * selectedItem = sel[0];
  int k = selectedItem->data(0, KLFSETTINGS_ROLE_PLUGINDEX).toInt();
  if (k < 0 || k >= klf_plugins.size()) {
    qWarning("KLFSettings::removePlugin: Error: What's going on?? k=%d > klf_plugins.size=%d", k, klf_plugins.size());
    return;
  }

  QMessageBox confirmdlg(this);
  confirmdlg.setIcon(QMessageBox::Warning);
  confirmdlg.setWindowTitle(tr("Remove Plugin?"));
  confirmdlg.setText(tr("<qt>Are you sure you want to remove Plugin <i>%1</i>?</qt>").arg(klf_plugins[k].title));
  confirmdlg.setDetailedText(tr("The Plugin File %1 will be removed from disk.").arg(klf_plugins[k].fpath));
  confirmdlg.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
  confirmdlg.setEscapeButton(QMessageBox::Cancel);
  confirmdlg.setDefaultButton(QMessageBox::Cancel);

  int confirmation = confirmdlg.exec();
  if (confirmation != QMessageBox::Yes) {
    // action cancelled by user
    return;
  }

  bool r = QFile::remove(klf_plugins[k].fpath);

  if ( r ) {
    QMessageBox::information(this, tr("Remove Plugin"),
			     tr("<p style=\"-qt-block-indent: 0; text-indent: 0px;\">Please note the following:<br />\n"
				"<ul><li>You need to restart KLatexFormula for changes to take effect\n"
				"<li>If this plugin was privided in an add-on, you need to remove the corresponding "
				"add-on too or the plugin will be automatically re-installed."
				"</p>"));
    // remove plugin list item
    delete selectedItem;
  } else {
    qWarning("Failed to remove plugin '%s'", qPrintable(klf_plugins[k].fpath));
    QMessageBox::critical(this, tr("Error"), tr("Failed to remove Plugin."));
  }

  refreshAddOnList();
  refreshAddOnSelected();
}

void KLFSettings::removePlugin(const QString& fname)
{
  int k;
  klfDbg("removing plugin "<<fname<<" from plugins. Dumping plugin list: ");
  for (k = 0; k < klf_plugins.size(); ++k) {
    klfDbg("  --> plugin: fname="<<klf_plugins[k].fname) ;
  }

  for (k = 0; k < klf_plugins.size() && klf_plugins[k].fname != fname; ++k)
    ;
  if (k < 0 || k >= klf_plugins.size()) {
    qWarning("KLFSettings::removePlugin: internal error: didn't find plugin name %s", qPrintable(fname));
    return;
  }

  bool r = QFile::remove(klf_plugins[k].fpath);

  if ( r ) {
    // find corresponding tree widget item
    QTreeWidgetItemIterator it(u->lstPlugins);
    while (*it) {
      if ( (*it)->data(0, KLFSETTINGS_ROLE_PLUGINDEX).toInt() == k ) {
	// remove plugin list item
	delete (*it);
	break;
      }
      ++it;
    }
  } else {
    qWarning("Failed to remove plugin '%s'", qPrintable(klf_plugins[k].fpath));
    QMessageBox::critical(this, tr("Error"), tr("Failed to remove Plugin %1.").arg(klf_plugins[k].title));
  }
}


bool KLFSettings::setDefaultFor(const QString& progname, const QString& guessedprog, bool required,
				KLFPathChooser *destination)
{
  QString progpath = guessedprog;
  if (progpath.isEmpty()) {
    if (QFileInfo(destination->path()).isExecutable()) {
      // field already has a valid value, don't touch it and don't complain
      return true;
    }
    if ( ! required )
      return false;
    QMessageBox msgbox(QMessageBox::Critical, tr("Error"), tr("Could not find %1 executable !")
		       .arg(progname),  QMessageBox::Ok);
    msgbox.setInformativeText(tr("Please check your installation and specify the path"
				 " to %1 executable manually if it is not installed"
				 " in $PATH.").arg(progname));
    msgbox.setDefaultButton(QMessageBox::Ok);
    msgbox.setEscapeButton(QMessageBox::Ok);
    msgbox.exec();
    return false;
  }

  destination->setPath(progpath);
  return true;
}

void KLFSettings::setDefaultPaths()
{
  KLFBackend::klfSettings defaultsettings;
  KLFBackend::detectSettings(&defaultsettings);
  if ( ! QFileInfo(u->pathTempDir->path()).isDir() )
    u->pathTempDir->setPath(QDir::toNativeSeparators(defaultsettings.tempdir));
  setDefaultFor("latex", defaultsettings.latexexec, true, u->pathLatex);
  setDefaultFor("dvips", defaultsettings.dvipsexec, true, u->pathDvips);
  setDefaultFor("gs", defaultsettings.gsexec, true, u->pathGs);
  bool r = setDefaultFor("epstopdf", defaultsettings.epstopdfexec, false, u->pathEpstopdf);
  u->chkEpstopdf->setChecked(r);
}


void KLFSettings::refreshAddOnList()
{
  u->lstAddOns->clear();
  u->lstAddOns->setColumnWidth(0, 160);

  // explore all addons
  int k;
  for (k = 0; k < klf_addons.size(); ++k) {
    QTreeWidgetItem *item = new QTreeWidgetItem(u->lstAddOns);
    item->setData(0, KLFSETTINGS_ROLE_ADDONINDEX, QVariant((int)k));

    item->setText(0, klf_addons[k].title());
    item->setText(1, klf_addons[k].description());

    // set background color to indicate if status is fresh,
    // and/or if plugin is installed locally
    if (klf_addons[k].isfresh()) {
      item->setBackground(0, QColor(200, 255, 200));
      item->setBackground(1, QColor(200, 255, 200));
    } /* // color locally installed plug-ins  [ don't, it's unaesthetic ! ]
	 else if (klf_addons[k].islocal()) {
	 item->setBackground(0, QColor(200, 200, 255));
	 item->setBackground(1, QColor(200, 200, 255));
	 } */
  }
}

void KLFSettings::refreshAddOnSelected()
{
  QList<QTreeWidgetItem*> sel = u->lstAddOns->selectedItems();
  if (sel.size() != 1) {
    u->lblAddOnInfo->setText("");
    u->btnRemoveAddOn->setEnabled(false);
    return;
  }
  int k = sel[0]->data(0, KLFSETTINGS_ROLE_ADDONINDEX).toInt();
  if (k < 0 || k >= klf_addons.size()) {
    u->lblAddOnInfo->setText("");
    u->btnRemoveAddOn->setEnabled(false);
    return;
  }

  // enable remove button only if this addon is "local", i.e. precisely removable
  u->btnRemoveAddOn->setEnabled(klf_addons[k].islocal());

  int smallpointsize = QFontInfo(font()).pointSize() - 1;
  u->lblAddOnInfo->setText(tr("<p style=\"-qt-block-indent: 0; text-indent: 0px; margin-bottom: 0px\">\n"
			   "<tt>Name:</tt> <span style=\"font-weight:600;\">%1</span><br />\n"
			   "<tt>Author:</tt> <span style=\"font-weight:600;\">%2</span><br />\n"
			   "<tt>Description:</tt></p>\n"
			    "<p style=\"font-weight: 600; margin-top: 2px; margin-left: 25px;"
			   "   margin-bottom: 0px;\">%3</p>\n"
			   "<p style=\"-qt-block-indent: 0; text-indent: 0px; margin-top: 2px;\">\n"
			   "<tt>File Name:</tt> <span style=\"font-size: %5pt;\">%4</span><br />\n"
			   "<tt>File Location:</tt> <span style=\"font-size: %5pt;\">%6</span><br />\n"
			   "<tt><i>%7</i></tt>").arg(Qt::escape(klf_addons[k].title()))
			.arg(Qt::escape(klf_addons[k].author())).arg(Qt::escape(klf_addons[k].description()))
			.arg(Qt::escape(klf_addons[k].fname()))
			.arg(smallpointsize)
			.arg(Qt::escape(QDir::toNativeSeparators(QFileInfo(klf_addons[k].dir())
								 .canonicalFilePath()) + QDir::separator()))
			.arg( klf_addons[k].islocal() ?
			      tr("Add-On installed locally") :
			      tr("Add-On installed globally on system") )
			);

}


void KLFSettings::importAddOn()
{
  QStringList efnames =
    QFileDialog::getOpenFileNames(this, tr("Please select add-on file(s) to import"),
				  QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation),
				  "Qt Resource Files (*.rcc)");
  int i;
  for (i = 0; i < efnames.size(); ++i) {
    importAddOn(efnames[i], false);
  }
  // display message to user to restart KLatexFormula, if needed
  if (i > 0) {
    QMessageBox::information(this, tr("Import"), tr("Please restart KLatexFormula for changes to take effect."));
  }
}

void KLFSettings::importAddOn(const QString& fileName, bool suggestRestart)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("fileName="<<fileName<<", suggestRestart="<<suggestRestart) ;

  QFileInfo fi(fileName);
  if (!fi.exists() || !fi.isReadable()) {
    QMessageBox::critical(this, tr("Error"), tr("File %1 cannot be accessed.").arg(fileName));
    return;
  }
  QString destination = klfconfig.homeConfigDirRCCResources+"/";
  QString destfpath = destination + QFileInfo(fileName).fileName();
  if ( QFile::exists(destfpath) ) {
    QMessageBox::critical(this, tr("Error"),
			  tr("An Add-On with the same file name has already been imported."));
    return;
  }
  bool r = QFile::copy(fileName, destfpath);
  if ( !r ) {
    QMessageBox::critical(this, tr("Error"), tr("Import of add-on file %1 failed.").arg(fileName));
    return;
  }

  // import succeeded, show the add-on as fresh.
  KLFAddOnInfo addoninfo(destfpath, true);
  if (!addoninfo.klfminversion().isEmpty() &&
      klfVersionCompareLessThan(KLF_VERSION_STRING, addoninfo.klfminversion())) {
    // add-on too recent
    QMessageBox::critical(this, tr("Error"),
			  tr("This add-on requires a more recent version of KLatexFormula.\n"
			     "Required version: %1\n"
			     "This version: %2").arg(addoninfo.klfminversion(), KLF_VERSION_STRING));
    return;
  }
  // if we have new translations, add them to our translation combo box, and prompt user to change to that
  // language. (It is highly reasonable that if he installed the translation add-on, it's to use it...!)
  int k;
  QStringList trlist = addoninfo.translations();
  KLFI18nFile *detectedI18nFile = NULL;
  for (k = 0; k < trlist.size(); ++k) {
    KLFI18nFile i18nfile(addoninfo.rccmountroot()+"/i18n/"+trlist[k]);
    if ( u->cbxLocale->findData(i18nfile.locale) == -1 ) {
      klfDbg("found translation: "<<i18nfile.locale) ;
      klf_add_avail_translation(i18nfile);
      if (detectedI18nFile == NULL)
	detectedI18nFile = new KLFI18nFile(i18nfile);
    }
  }
  if (detectedI18nFile != NULL) {
    klfDbg("translation(s) found. first one found was: "<<detectedI18nFile->locale) ;
    // update the translation list
    populateLocaleCombo();
    // find the translation
    for (k = 0; k < klf_avail_translations.size(); ++k) {
      if (klf_avail_translations[k].localename == detectedI18nFile->locale)
	break;
    }
    int cbxindex = u->cbxLocale->findData(detectedI18nFile->locale);
    if (k >= 0 && k < klf_avail_translations.size() && cbxindex >= 0) {
      if (QMessageBox::question(this, tr("Change Language"), tr("Change application language to <b>%1</b>?")
				.arg(klf_avail_translations[k].translatedname),
				QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes)
	  == QMessageBox::Yes) {
	u->cbxLocale->setCurrentIndex(cbxindex);
	apply();
	// a message warning the user to restart has already been displayed in the apply() above.
	suggestRestart = false;
      }
    }
    delete detectedI18nFile;
    detectedI18nFile = NULL;
  }
  klf_addons.append(addoninfo);
  refreshAddOnList();

  if (suggestRestart) {
    QMessageBox::information(this, tr("Import"), tr("Please restart KLatexFormula for changes to take effect."));
  }
}

void KLFSettings::removeAddOn()
{
  QList<QTreeWidgetItem*> sel = u->lstAddOns->selectedItems();
  if (sel.size() != 1) {
    qWarning("Expected single add-on selection for removal !");
    return;
  }

  int k = sel[0]->data(0, KLFSETTINGS_ROLE_ADDONINDEX).toInt();
  if (k < 0 || k >= klf_addons.size()) {
    // what's going on ???
    return;
  }

  QMessageBox confirmdlg(this);
  confirmdlg.setIcon(QMessageBox::Warning);
  confirmdlg.setWindowTitle(tr("Remove Add-On?"));
  confirmdlg.setText(tr("<qt>Are you sure you want to remove Add-On <i>%1</i>?</qt>")
		     .arg(klf_addons[k].title()));
  QStringList ourplugins = klf_addons[k].localPluginList();
  QString msg;
  if (ourplugins.size()) {
    msg = tr("The Add-On File %1 will be removed from disk, along with plugin(s) %2.")
      .arg(klf_addons[k].fpath(), ourplugins.join(", "));
  } else {
    msg = tr("The Add-On File %1 will be removed from disk.").arg(klf_addons[k].fpath());
  }
  confirmdlg.setDetailedText(msg);
  confirmdlg.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
  confirmdlg.setEscapeButton(QMessageBox::Cancel);
  confirmdlg.setDefaultButton(QMessageBox::Cancel);

  int confirmation = confirmdlg.exec();
  if (confirmation != QMessageBox::Yes) {
    // action cancelled by user
    return;
  }

  bool r = QFile::remove(klf_addons[k].fpath());
  // remove all corresponding plugins too
  int j;
  if ( r ) {
    QMessageBox::information(this, tr("Remove Add-On"),
			     tr("Please restart KLatexFormula for changes to take effect."));
  } else {
    qWarning("Failed to remove add-on '%s'", qPrintable(klf_addons[k].fpath()));
    QMessageBox::critical(this, tr("Error"), tr("Failed to remove Add-On."));
    return;
  }

  // remove all corresponding plug-ins
  QStringList lplugins = klf_addons[k].localPluginList();
  for (j = 0; j < lplugins.size(); ++j)
    removePlugin(lplugins[j]);

  klf_addons.removeAt(k);

  refreshAddOnList();
  refreshAddOnSelected();
}

void KLFSettings::slotChangeFontPresetSender()
{
  QAction *a = qobject_cast<QAction*>(sender());
  if (a == 0)
    return;
  const QVariantMap vmap = a->data().toMap();
  klfDbg("Set font from action with data "<<vmap) ;
  QString btnkey = vmap["Button"].toString();
  KLF_ASSERT_CONDITION(pFontButtons.contains(btnkey), "Unknown button "<<btnkey<<" !", return ) ;
  QFont f = vmap["Font"].value<QFont>();
  slotChangeFont(pFontButtons[btnkey], f);
  if (vmap.contains("isSystemDefaultAppFont") && vmap["isSystemDefaultAppFont"].toBool()) {
    klfDbg("Set default application font.") ;
    pUserSetDefaultAppFont = true;
  }
}
void KLFSettings::slotChangeFontSender()
{
  QPushButton *w = qobject_cast<QPushButton*>(sender());
  if ( w == 0 )
    return;
  QFont fnt = QFontDialog::getFont(0, w->property("selectedFont").value<QFont>(), this);
  slotChangeFont(w, fnt);
}
void KLFSettings::slotChangeFont(QPushButton *w, const QFont& fnt)
{
  if ( w == 0 )
    return;
  w->setFont(fnt);
  w->setProperty("selectedFont", QVariant(fnt));
  if (w == u->btnAppFont)
    pUserSetDefaultAppFont = false;
}

void KLFSettings::apply()
{
  int k;
  // apply settings here

  // the settings object that we will fill, and set to _mainwin
  KLFBackend::klfSettings s = _mainwin->currentSettings();

  s.tempdir = QDir::fromNativeSeparators(u->pathTempDir->path());
  s.latexexec = u->pathLatex->path();
  s.dvipsexec = u->pathDvips->path();
  s.gsexec = u->pathGs->path();
  s.epstopdfexec = QString();
  if (u->chkEpstopdf->isChecked()) {
    s.epstopdfexec = u->pathEpstopdf->path();
  }
  // detect environment for those settings (in particular mgs.exe for ghostscript ...)

  klf_detect_execenv(&s);

  s.lborderoffset = u->spnLBorderOffset->value();
  s.tborderoffset = u->spnTBorderOffset->value();
  s.rborderoffset = u->spnRBorderOffset->value();
  s.bborderoffset = u->spnBBorderOffset->value();
  s.outlineFonts = u->chkOutlineFonts->isChecked();

  _mainwin->applySettings(s);

  if (u->chkSHEnable->isChecked())
    klfconfig.SyntaxHighlighter.configFlags |= KLFLatexSyntaxHighlighter::Enabled;
  else
    klfconfig.SyntaxHighlighter.configFlags &= ~KLFLatexSyntaxHighlighter::Enabled;
  if (u->chkSHHighlightParensOnly->isChecked())
    klfconfig.SyntaxHighlighter.configFlags |= KLFLatexSyntaxHighlighter::HighlightParensOnly;
  else
    klfconfig.SyntaxHighlighter.configFlags &= ~KLFLatexSyntaxHighlighter::HighlightParensOnly;
  if (u->chkSHHighlightLonelyParen->isChecked())
    klfconfig.SyntaxHighlighter.configFlags |= KLFLatexSyntaxHighlighter::HighlightLonelyParen;
  else
    klfconfig.SyntaxHighlighter.configFlags &= ~KLFLatexSyntaxHighlighter::HighlightLonelyParen;

  for (k = 0; k < _textformats.size(); ++k) {
    QColor c = _textformats[k].fg->color();
    if (c.isValid())
      _textformats[k].fmt->setForeground(c);
    else
      _textformats[k].fmt->clearForeground();
    c = _textformats[k].bg->color();
    if (c.isValid())
      _textformats[k].fmt->setBackground(c);
    else
      _textformats[k].fmt->clearBackground();
    Qt::CheckState b = _textformats[k].chkB->checkState();
    if (b == Qt::PartiallyChecked)
      _textformats[k].fmt->clearProperty(QTextFormat::FontWeight);
    else if (b == Qt::Checked)
      _textformats[k].fmt->setFontWeight(QFont::Bold);
    else
      _textformats[k].fmt->setFontWeight(QFont::Normal);
    Qt::CheckState it = _textformats[k].chkI->checkState();
    if (it == Qt::PartiallyChecked)
      _textformats[k].fmt->clearProperty(QTextFormat::FontItalic);
    else
      _textformats[k].fmt->setFontItalic( it == Qt::Checked );
  }

  // language settings
  QString localename = u->cbxLocale->itemData(u->cbxLocale->currentIndex()).toString();
  klfDbg("New locale name: "<<localename);
  bool localechanged = false;
  if (klfconfig.UI.locale != localename) {
    if ((!klfconfig.UI.locale.isEmpty() && klfconfig.UI.locale != "C" && klfconfig.UI.locale != "en_US") ||
	(!localename.isEmpty() && localename != "C" && localename != "en_US"))
      localechanged = true; // not just switching from "C" to "en_US" which is not a locale change...
  }
  klfconfig.UI.locale = localename;
  QLocale::setDefault(klfconfig.UI.locale);
  _mainwin->setApplicationLocale(localename);
  if (localechanged) {
    QMessageBox::information(this, tr("Language changed"),
			     tr("You may need to restart KLatexFormula for your new language "
				"settings to fully take effect."));
  }
  //  klf_main_do_the_change_of_locale_and_load_translators(...);
  //  QList<QWidget*> uilist;
  //  uilist << _mainwin << _mainwin->libraryBrowserWidget() << _mainwin->latexSymbolsWidget()
  //	 << _mainwin->styleManagerWidget() << this ;
  //  for (k = 0; k < uilist.size(); ++k) {
  //    uilist[k]->retranlsateUi(uilist[k]);
  //  }


  // font settings
  QFont curAppFont = klfconfig.UI.applicationFont;
  QFont newAppFont = u->btnAppFont->property("selectedFont").value<QFont>();
  if (curAppFont != newAppFont || pUserSetDefaultAppFont != klfconfig.UI.useSystemAppFont) {
    klfconfig.UI.useSystemAppFont = pUserSetDefaultAppFont;
    klfconfig.UI.applicationFont = newAppFont;
    if (klfconfig.UI.useSystemAppFont) {
      qApp->setFont(klfconfig.defaultStdFont);
      qApp->setFont(QFont());
    } else {
      qApp->setFont(klfconfig.UI.applicationFont);
    }
    // Style sheet refresh is needed to force font (?)
    qApp->setStyleSheet(qApp->styleSheet());
    _mainwin->refreshAllWindowStyleSheets();
  }
  klfconfig.UI.latexEditFont = u->btnEditorFont->property("selectedFont").value<QFont>();
  _mainwin->setTxtLatexFont(klfconfig.UI.latexEditFont);
  klfconfig.UI.preambleEditFont = u->btnPreambleFont->property("selectedFont").value<QFont>();
  _mainwin->setTxtPreambleFont(klfconfig.UI.preambleEditFont);

  klfconfig.UI.labelOutputFixedSize = QSize(u->spnPreviewWidth->value(), u->spnPreviewHeight->value());
  klfconfig.UI.enableRealTimePreview = u->chkEnableRealTimePreview->isChecked();

  klfconfig.UI.previewTooltipMaxSize = QSize(u->spnToolTipMaxWidth->value(), u->spnToolTipMaxHeight->value());
  klfconfig.UI.enableToolTipPreview = u->chkEnableToolTipPreview->isChecked();

  klfconfig.UI.showHintPopups = u->chkShowHintPopups->isChecked();
  klfconfig.UI.clearLatexOnly = u->chkClearLatexOnly->isChecked();
  klfconfig.UI.glowEffect = u->chkGlowEffect->isChecked();

  klfconfig.UI.copyExportProfile = 
    u->cbxCopyExportProfile->itemData(u->cbxCopyExportProfile->currentIndex()).toString();
  klfconfig.UI.dragExportProfile = 
    u->cbxDragExportProfile->itemData(u->cbxDragExportProfile->currentIndex()).toString();
  //  klfconfig.UI.showExportProfilesLabel = u->chkShowExportProfilesLabel->isChecked();
  klfconfig.UI.menuExportProfileAffectsDrag = u->chkMenuExportProfileAffectsDrag->isChecked();
  klfconfig.UI.menuExportProfileAffectsCopy = u->chkMenuExportProfileAffectsCopy->isChecked();

  klfconfig.LibraryBrowser.restoreURLs = u->chkLibRestoreURLs->isChecked();
  klfconfig.LibraryBrowser.confirmClose = u->chkLibConfirmClose->isChecked();
  klfconfig.LibraryBrowser.historyTagCopyToArchive = u->chkLibHistoryTagCopyToArchive->isChecked();
  //  klfconfig.LibraryBrowser.groupSubCategories = u->chkLibGroupSubCategories->isChecked();
  klfconfig.LibraryBrowser.iconViewFlow =  u->cbxLibIconViewFlow->selectedValue();

  // save plugin config
  bool warnneedrestart = false;
  QTreeWidgetItemIterator it(u->lstPlugins);
  while (*it) {
    int j = (*it)->data(0, KLFSETTINGS_ROLE_PLUGINDEX).toInt();
    QString name = (*it)->data(0, KLFSETTINGS_ROLE_PLUGNAME).toString();
    bool loadenable = ( (*it)->checkState(0) == Qt::Checked ) ;
    if (loadenable != klfconfig.Plugins.pluginConfig[name]["__loadenabled"])
      warnneedrestart = true;
    klfconfig.Plugins.pluginConfig[name]["__loadenabled"] = loadenable;

    if (klf_plugins[j].instance != NULL) {
      KLFPluginConfigAccess pconfa = klfconfig.getPluginConfigAccess(name);
      klf_plugins[j].instance->saveToConfig(mPluginConfigWidgets[name], &pconfa);
    }

    ++it;
  }
  if (warnneedrestart) {
    QMessageBox::information(this, tr("Restart KLatexFormula"),
			     tr("You need to restart KLatexFormula for your changes to take effect."));
  }

  _mainwin->refreshShowCorrectClearButton();
  _mainwin->saveSettings();

  // recalculate window sizes etc.
  _mainwin->refreshWindowSizes();

  // in case eg. the plugins re-change klfconfig in some way (skin does this for syntax highlighting)
  // -> refresh
  reset();
}

void KLFSettings::accept()
{
  // apply settings
  apply();
  // and exit dialog
  QDialog::accept();
}

void KLFSettings::help()
{
  QWhatsThis::enterWhatsThisMode();
}




