/***************************************************************************
 *   Copyright (c) 2011 Jrgen Riegel (juergen.riegel@web.de)               *
 *   Copyright (c) 2015 Eivind Kvedalen (eivind@kvedalen.name)             *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"

#ifndef _PreComp_
# include <QFile>
# include <QFileInfo>
# include <QImage>
# include <QString>
# include <QMenu>
#endif

#include "ViewProviderSpreadsheet.h"
#include "SpreadsheetView.h"

#include <Mod/Spreadsheet/App/Sheet.h>
#include <App/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Application.h>
#include <Gui/MainWindow.h>
#include <Gui/Command.h>
#include <Base/FileInfo.h>
#include <Base/Stream.h>
#include <Base/Console.h>
#include <sstream>

using namespace Gui;
using namespace SpreadsheetGui;
using namespace Spreadsheet;


PROPERTY_SOURCE(SpreadsheetGui::ViewProviderSheet, Gui::ViewProviderDocumentObject)

ViewProviderSheet::ViewProviderSheet()
    : Gui::ViewProviderDocumentObject()
{
}

ViewProviderSheet::~ViewProviderSheet()
{
    if (!view.isNull()) {
        Gui::getMainWindow()->removeWindow(view);
//        delete view;
    }
}

void ViewProviderSheet::setDisplayMode(const char* ModeName)
{
    ViewProviderDocumentObject::setDisplayMode(ModeName);
}

std::vector<std::string> ViewProviderSheet::getDisplayModes(void) const
{
    std::vector<std::string> StrList;
    StrList.push_back("Spreadsheet");
    return StrList;
}

QIcon ViewProviderSheet::getIcon() const
{
    static const char * const Points_Feature_xpm[] = {
        "16 16 3 1",
        "       c None",
        ".      c #000000",
        "+      c #FFFFFF",
        "                ",
        "                ",
        "................",
        ".++++.++++.++++.",
        ".++++.++++.++++.",
        "................",
        ".++++.++++.++++.",
        ".++++.++++.++++.",
        "................",
        ".++++.++++.++++.",
        ".++++.++++.++++.",
        "................",
        ".++++.++++.++++.",
        ".++++.++++.++++.",
        "................",
        "                "};
    QPixmap px(Points_Feature_xpm);
    return px;
}

bool ViewProviderSheet::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        if (!this->view) {
            showSpreadsheetView();
            view->viewAll();
        }
        Gui::getMainWindow()->setActiveWindow(this->view);
        return false;
    }
    return false;
}

bool ViewProviderSheet::doubleClicked()
{
    if (!this->view) {
        showSpreadsheetView();
        view->viewAll();
    }
    Gui::getMainWindow()->setActiveWindow(this->view);
    return true;
}

void ViewProviderSheet::setupContextMenu(QMenu * menu, QObject *receiver, const char *member)
{
    QAction* act;
    act = menu->addAction(QObject::tr("Show spreadsheet"), receiver, member);
    act->setData(QVariant((int)ViewProvider::Default));
}

Sheet *ViewProviderSheet::getSpreadsheetObject() const
{
    return freecad_dynamic_cast<Sheet>(pcObject);
}

bool ViewProviderSheet::onDelete(const std::vector<std::string> &)
{
    // If view is closed, delete the object
    if (view.isNull())
        return true;

    // View is not closed, delete cell contents instead if it is active
    if (Gui::Application::Instance->activeDocument()) {
        Gui::MDIView* activeWindow = Gui::getMainWindow()->activeWindow();
        SpreadsheetGui::SheetView * sheetView = freecad_dynamic_cast<SpreadsheetGui::SheetView>(activeWindow);

        if (sheetView) {
            Sheet * sheet = sheetView->getSheet();
            QModelIndexList selection = sheetView->selectedIndexes();

            if (selection.size() > 0) {
                Gui::Command::openCommand("Clear cell(s)");
                for (QModelIndexList::const_iterator it = selection.begin(); it != selection.end(); ++it) {
                    std::string address = CellAddress((*it).row(), (*it).column()).toString();
                    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.clear('%s')", sheet->getNameInDocument(),
                                            address.c_str());
                }
                Gui::Command::commitCommand();
                Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
            }
        }
    }

    return false;
}

SheetView *ViewProviderSheet::showSpreadsheetView()
{
    if (!view){
        Gui::Document* doc = Gui::Application::Instance->getDocument
            (this->pcObject->getDocument());
        view = new SheetView(doc, this->pcObject, Gui::getMainWindow());
        view->setWindowIcon(Gui::BitmapFactory().pixmap(":icons/Spreadsheet.svg"));
        view->setWindowTitle(QString::fromUtf8(pcObject->Label.getValue()) + QString::fromAscii("[*]"));
        Gui::getMainWindow()->addWindow(view);
        startEditing();
    }

    return view;
}

void ViewProviderSheet::updateData(const App::Property* prop)
{
    if (view)
        view->updateCell(prop);
}
