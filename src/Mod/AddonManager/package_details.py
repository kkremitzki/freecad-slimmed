# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2021 Chris Hennes <chennes@pioneerlibrarysystem.org>    *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
from PySide2.QtCore import *
from PySide2.QtGui import *
from PySide2.QtWidgets import *
from PySide2.QtWebEngineWidgets import *

import os

import FreeCAD

import addonmanager_utilities as utils
from addonmanager_workers import GetMacroDetailsWorker, CheckSingleUpdateWorker
from AddonManagerRepo import AddonManagerRepo

translate = FreeCAD.Qt.translate

show_javascript_console_output = False


class PackageDetails(QWidget):

    back = Signal()
    install = Signal(AddonManagerRepo)
    uninstall = Signal(AddonManagerRepo)
    update = Signal(AddonManagerRepo)
    execute = Signal(AddonManagerRepo)
    update_status = Signal(AddonManagerRepo)
    check_for_update = Signal(AddonManagerRepo)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.ui = Ui_PackageDetails()
        self.ui.setupUi(self)

        self.worker = None
        self.repo = None
        self.status_update_thread = None

        self.ui.buttonBack.clicked.connect(self.back.emit)
        self.ui.buttonBack.clicked.connect(self.clear_web_view)
        self.ui.buttonExecute.clicked.connect(lambda: self.execute.emit(self.repo))
        self.ui.buttonInstall.clicked.connect(lambda: self.install.emit(self.repo))
        self.ui.buttonUninstall.clicked.connect(lambda: self.uninstall.emit(self.repo))
        self.ui.buttonUpdate.clicked.connect(lambda: self.update.emit(self.repo))
        self.ui.buttonCheckForUpdate.clicked.connect(
            lambda: self.check_for_update.emit(self.repo)
        )
        self.ui.webView.loadStarted.connect(self.load_started)
        self.ui.webView.loadProgress.connect(self.load_progress)
        self.ui.webView.loadFinished.connect(self.load_finished)

        loading_html_file = os.path.join(os.path.dirname(__file__), "loading.html")
        with open(loading_html_file, "r", errors="ignore") as f:
            html = f.read()
            self.ui.loadingLabel.setHtml(html)
            self.ui.loadingLabel.show()
            self.ui.webView.hide()

    def show_repo(self, repo: AddonManagerRepo, reload: bool = False) -> None:

        # If this is the same repo we were already showing, we do not have to do the
        # expensive refetch unless reload is true
        if self.repo != repo or reload:
            self.repo = repo
            self.ui.loadingLabel.show()
            self.ui.webView.hide()
            self.ui.progressBar.show()

            if self.worker is not None:
                if not self.worker.isFinished():
                    self.worker.requestInterruption()
                    self.worker.wait()

            if repo.repo_type == AddonManagerRepo.RepoType.MACRO:
                self.show_macro(repo)
                self.ui.buttonExecute.show()
            elif repo.repo_type == AddonManagerRepo.RepoType.WORKBENCH:
                self.show_workbench(repo)
                self.ui.buttonExecute.hide()
            elif repo.repo_type == AddonManagerRepo.RepoType.PACKAGE:
                self.show_package(repo)
                self.ui.buttonExecute.hide()

        if self.status_update_thread is not None:
            if not self.status_update_thread.isFinished():
                self.status_update_thread.requestInterruption()
                self.status_update_thread.wait()

        if repo.status() == AddonManagerRepo.UpdateStatus.UNCHECKED:
            self.status_update_thread = QThread()
            self.status_update_worker = CheckSingleUpdateWorker(repo, self)
            self.status_update_worker.moveToThread(self.status_update_thread)
            self.status_update_thread.finished.connect(
                self.status_update_worker.deleteLater
            )
            self.check_for_update.connect(self.status_update_worker.do_work)
            self.status_update_worker.update_status.connect(self.display_repo_status)
            self.status_update_thread.start()
            self.check_for_update.emit(self.repo)

        self.display_repo_status(self.repo.update_status)

    def display_repo_status(self, status):
        repo = self.repo
        if status != AddonManagerRepo.UpdateStatus.NOT_INSTALLED:

            version = repo.installed_version
            date = ""
            installed_version_string = "<h3>"
            if repo.updated_timestamp:
                date = (
                    QDateTime.fromTime_t(repo.updated_timestamp)
                    .date()
                    .toString(Qt.SystemLocaleShortDate)
                )
            if version and date:
                installed_version_string += (
                    translate(
                        "AddonsInstaller", "Version {version} installed on {date}"
                    ).format(version=version, date=date)
                    + ". "
                )
            elif version:
                installed_version_string += (
                    translate("AddonsInstaller", "Version {version} installed") + ". "
                ).format(version=version)
            elif date:
                installed_version_string += (
                    translate("AddonsInstaller", "Installed on {date}") + ". "
                ).format(date=date)
            else:
                installed_version_string += (
                    translate("AddonsInstaller", "Installed") + ". "
                )

            if status == AddonManagerRepo.UpdateStatus.UPDATE_AVAILABLE:
                if repo.metadata:
                    installed_version_string += (
                        "<b>"
                        + translate("AddonsInstaller", "Update available to version")
                        + " "
                    )
                    installed_version_string += repo.metadata.Version
                    installed_version_string += ".</b>"
                elif repo.macro and repo.macro.version:
                    installed_version_string += (
                        "<b>"
                        + translate("AddonsInstaller", "Update available to version")
                        + " "
                    )
                    installed_version_string += repo.macro.version
                    installed_version_string += ".</b>"
                else:
                    installed_version_string += (
                        "<b>"
                        + translate(
                            "AddonsInstaller",
                            "An update is available",
                        )
                        + ".</b>"
                    )
            elif status == AddonManagerRepo.UpdateStatus.NO_UPDATE_AVAILABLE:
                installed_version_string += (
                    translate("AddonsInstaller", "This is the latest version available")
                    + "."
                )
            elif status == AddonManagerRepo.UpdateStatus.PENDING_RESTART:
                installed_version_string += (
                    translate(
                        "AddonsInstaller", "Updated, please restart FreeCAD to use"
                    )
                    + "."
                )
            elif status == AddonManagerRepo.UpdateStatus.UNCHECKED:

                pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
                autocheck = pref.GetBool("AutoCheck", False)
                if autocheck:
                    installed_version_string += (
                        translate("AddonsInstaller", "Update check in progress") + "."
                    )
                else:
                    installed_version_string += (
                        translate("AddonsInstaller", "Automatic update checks disabled")
                        + "."
                    )

            installed_version_string += "</h3>"
            self.ui.labelPackageDetails.setText(installed_version_string)
            if repo.status() == AddonManagerRepo.UpdateStatus.UPDATE_AVAILABLE:
                self.ui.labelPackageDetails.setStyleSheet(
                    "color:" + utils.attention_color_string()
                )
            else:
                self.ui.labelPackageDetails.setStyleSheet(
                    "color:" + utils.bright_color_string()
                )
            self.ui.labelPackageDetails.show()

            if repo.macro is not None:
                moddir = FreeCAD.getUserMacroDir(True)
            else:
                basedir = FreeCAD.getUserAppDataDir()
                moddir = os.path.join(basedir, "Mod", repo.name)
            installationLocationString = (
                translate("AddonsInstaller", "Installation location") + ": " + moddir
            )

            self.ui.labelInstallationLocation.setText(installationLocationString)
            self.ui.labelInstallationLocation.show()
        else:
            self.ui.labelPackageDetails.hide()
            self.ui.labelInstallationLocation.hide()

        if status == AddonManagerRepo.UpdateStatus.NOT_INSTALLED:
            self.ui.buttonInstall.show()
            self.ui.buttonUninstall.hide()
            self.ui.buttonUpdate.hide()
            self.ui.buttonCheckForUpdate.hide()
        elif status == AddonManagerRepo.UpdateStatus.NO_UPDATE_AVAILABLE:
            self.ui.buttonInstall.hide()
            self.ui.buttonUninstall.show()
            self.ui.buttonUpdate.hide()
            self.ui.buttonCheckForUpdate.hide()
        elif status == AddonManagerRepo.UpdateStatus.UPDATE_AVAILABLE:
            self.ui.buttonInstall.hide()
            self.ui.buttonUninstall.show()
            self.ui.buttonUpdate.show()
            self.ui.buttonCheckForUpdate.hide()
        elif status == AddonManagerRepo.UpdateStatus.UNCHECKED:
            self.ui.buttonInstall.hide()
            self.ui.buttonUninstall.show()
            self.ui.buttonUpdate.hide()
            self.ui.buttonCheckForUpdate.show()
        elif status == AddonManagerRepo.UpdateStatus.PENDING_RESTART:
            self.ui.buttonInstall.hide()
            self.ui.buttonUninstall.show()
            self.ui.buttonUpdate.hide()
            self.ui.buttonCheckForUpdate.hide()

        if repo.obsolete:
            self.ui.labelWarningInfo.show()
            self.ui.labelWarningInfo.setText(
                "<h1>"
                + translate("AddonsInstaller", "WARNING: This addon is obsolete")
                + "</h1>"
            )
            self.ui.labelWarningInfo.setStyleSheet(
                "color:" + utils.warning_color_string()
            )
        elif repo.python2:
            self.ui.labelWarningInfo.show()
            self.ui.labelWarningInfo.setText(
                "<h1>"
                + translate("AddonsInstaller", "WARNING: This addon is Python 2 Only")
                + "</h1>"
            )
            self.ui.labelWarningInfo.setStyleSheet(
                "color:" + utils.warning_color_string()
            )
        else:
            self.ui.labelWarningInfo.hide()

    def show_workbench(self, repo: AddonManagerRepo) -> None:
        """loads information of a given workbench"""
        url = utils.get_readme_html_url(repo)
        self.ui.webView.load(QUrl(url))
        self.ui.urlBar.setText(url)

    def show_package(self, repo: AddonManagerRepo) -> None:
        """Show the details for a package (a repo with a package.xml metadata file)"""

        readme_url = None
        if repo.metadata:
            urls = repo.metadata.Urls
            for url in urls:
                if url["type"] == "readme":
                    readme_url = url["location"]
                    break
        if not readme_url:
            readme_url = utils.get_readme_html_url(repo)
        self.ui.webView.load(QUrl(readme_url))
        self.ui.urlBar.setText(readme_url)

    def show_macro(self, repo: AddonManagerRepo) -> None:
        """loads information of a given macro"""

        self.ui.webView.load(QUrl(repo.macro.url))
        self.ui.urlBar.setText(repo.macro.url)

        # We need to populate the macro information... may as well do it while the user reads the wiki page
        self.worker = GetMacroDetailsWorker(repo)
        self.worker.start()

    def run_javascript(self):
        """Modify the page for a README to optimize for viewing in a smaller window"""

        s = """
( function() {
    const url = new URL (window.location);
    const body = document.getElementsByTagName("body")[0];
    if (url.hostname === "github.com") {
        const articles = document.getElementsByTagName("article");
        if (articles.length > 0) {
            const article = articles[0];
            body.appendChild (article);
            body.style.padding = "1em";
            let sibling = article.previousSibling;
            while (sibling) {
                sibling.remove();
                sibling = article.previousSibling;
            }
        }
    } else if (url.hostname === "gitlab.com" || 
               url.hostname === "framagit.org" || 
               url.hostname === "salsa.debian.org") {
        // These all use the GitLab page display...
        const articles = document.getElementsByTagName("article");
        if (articles.length > 0) {
            const article = articles[0];
            body.appendChild (article);
            body.style.padding = "1em";
            let sibling = article.previousSibling;
            while (sibling) {
                sibling.remove();
                sibling = article.previousSibling;
            }
        }
    } else if (url.hostname === "wiki.freecad.org" || 
               url.hostname === "wiki.freecadweb.org") {
        const first_heading = document.getElementById('firstHeading');
        const body_content = document.getElementById('bodyContent');
        const new_node = document.createElement("div");
        new_node.appendChild(first_heading);
        new_node.appendChild(body_content);
        body.appendChild(new_node);
        let sibling = new_node.previousSibling;
        while (sibling) {
            sibling.remove();
            sibling = new_node.previousSibling;
        }
    }
}
) ()
"""
        self.ui.webView.page().runJavaScript(s)

    def clear_web_view(self):
        self.ui.webView.setHtml("<html><body><h1>Loading...</h1></body></html>")

    def load_started(self):
        self.ui.progressBar.show()
        self.ui.progressBar.setValue(0)

    def load_progress(self, progress: int):
        self.ui.progressBar.setValue(progress)

    def load_finished(self, load_succeeded: bool):
        self.ui.loadingLabel.hide()
        self.ui.webView.show()
        self.ui.progressBar.hide()
        url = self.ui.webView.url()
        if load_succeeded:
            # It says it succeeded, but it might have only succeeded in loading a "Page not found" page!
            title = self.ui.webView.title()
            path_components = url.path().split("/")
            expected_content = path_components[-1]
            if url.host() == "github.com" and expected_content not in title:
                self.show_error_for(url)
            elif title == "":
                self.show_error_for(url)
            else:
                self.run_javascript()
        else:
            self.show_error_for(url)

    def show_error_for(self, url: QUrl) -> None:
        m = translate(
            "AddonsInstaller", "Could not load README data from URL {}"
        ).format(url.toString())
        html = f"<html><body><p>{m}</p></body></html>"
        self.ui.webView.setHtml(html)


class RestrictedWebPage(QWebEnginePage):
    """A class that follows links to FreeCAD wiki pages, but opens all other clicked links in the system web browser"""

    def __init__(self, parent):
        super().__init__(parent)
        self.settings().setAttribute(QWebEngineSettings.ErrorPageEnabled, False)

    def acceptNavigationRequest(self, url, _type, isMainFrame):
        if _type == QWebEnginePage.NavigationTypeLinkClicked:

            # See if the link is to a FreeCAD Wiki page -- if so, follow it, otherwise ask the OS to open it
            if url.host() == "wiki.freecad.org" or url.host() == "wiki.freecadweb.org":
                return super().acceptNavigationRequest(url, _type, isMainFrame)
            else:
                QDesktopServices.openUrl(url)
                return False
        return super().acceptNavigationRequest(url, _type, isMainFrame)

    def javaScriptConsoleMessage(self, level, message, lineNumber, sourceID):
        global show_javascript_console_output
        if show_javascript_console_output:
            tag = translate("AddonsInstaller", "Page JavaScript reported")
            if level == QWebEnginePage.InfoMessageLevel:
                FreeCAD.Console.PrintMessage(f"{tag} {lineNumber}: {message}\n")
            elif level == QWebEnginePage.WarningMessageLevel:
                FreeCAD.Console.PrintWarning(f"{tag} {lineNumber}: {message}\n")
            elif level == QWebEnginePage.ErrorMessageLevel:
                FreeCAD.Console.PrintError(f"{tag} {lineNumber}: {message}\n")


class Ui_PackageDetails(object):
    def setupUi(self, PackageDetails):
        if not PackageDetails.objectName():
            PackageDetails.setObjectName("PackageDetails")
        self.verticalLayout_2 = QVBoxLayout(PackageDetails)
        self.verticalLayout_2.setObjectName("verticalLayout_2")
        self.layoutDetailsBackButton = QHBoxLayout()
        self.layoutDetailsBackButton.setObjectName("layoutDetailsBackButton")
        self.buttonBack = QToolButton(PackageDetails)
        self.buttonBack.setObjectName("buttonBack")
        self.buttonBack.setIcon(
            QIcon.fromTheme("back", QIcon(":/icons/button_left.svg"))
        )

        self.layoutDetailsBackButton.addWidget(self.buttonBack)

        self.horizontalSpacer = QSpacerItem(
            40, 20, QSizePolicy.Expanding, QSizePolicy.Minimum
        )

        self.layoutDetailsBackButton.addItem(self.horizontalSpacer)

        self.buttonInstall = QPushButton(PackageDetails)
        self.buttonInstall.setObjectName("buttonInstall")

        self.layoutDetailsBackButton.addWidget(self.buttonInstall)

        self.buttonUninstall = QPushButton(PackageDetails)
        self.buttonUninstall.setObjectName("buttonUninstall")

        self.layoutDetailsBackButton.addWidget(self.buttonUninstall)

        self.buttonUpdate = QPushButton(PackageDetails)
        self.buttonUpdate.setObjectName("buttonUpdate")

        self.layoutDetailsBackButton.addWidget(self.buttonUpdate)

        self.buttonCheckForUpdate = QPushButton(PackageDetails)
        self.buttonCheckForUpdate.setObjectName("buttonCheckForUpdate")

        self.layoutDetailsBackButton.addWidget(self.buttonCheckForUpdate)

        self.buttonExecute = QPushButton(PackageDetails)
        self.buttonExecute.setObjectName("buttonExecute")

        self.layoutDetailsBackButton.addWidget(self.buttonExecute)

        self.verticalLayout_2.addLayout(self.layoutDetailsBackButton)

        self.labelPackageDetails = QLabel(PackageDetails)
        self.labelPackageDetails.hide()

        self.verticalLayout_2.addWidget(self.labelPackageDetails)

        self.labelInstallationLocation = QLabel(PackageDetails)
        self.labelInstallationLocation.setTextInteractionFlags(Qt.TextSelectableByMouse)
        self.labelInstallationLocation.hide()

        self.verticalLayout_2.addWidget(self.labelInstallationLocation)

        self.labelWarningInfo = QLabel(PackageDetails)
        self.labelWarningInfo.hide()

        self.verticalLayout_2.addWidget(self.labelWarningInfo)

        sizePolicy1 = QSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        sizePolicy1.setHorizontalStretch(0)
        sizePolicy1.setVerticalStretch(0)

        self.webView = QWebEngineView(PackageDetails)
        self.webView.setObjectName("webView")
        self.webView.setSizePolicy(sizePolicy1)
        self.webView.setPage(RestrictedWebPage(PackageDetails))

        self.verticalLayout_2.addWidget(self.webView)

        self.loadingLabel = QWebEngineView(PackageDetails)
        self.loadingLabel.setObjectName("loadingLabel")
        self.loadingLabel.setSizePolicy(sizePolicy1)

        self.verticalLayout_2.addWidget(self.loadingLabel)

        self.progressBar = QProgressBar(PackageDetails)
        self.progressBar.setObjectName("progressBar")
        self.progressBar.setTextVisible(False)

        self.verticalLayout_2.addWidget(self.progressBar)

        self.urlBar = QLineEdit(PackageDetails)
        self.urlBar.setObjectName("urlBar")
        self.urlBar.setReadOnly(True)

        self.verticalLayout_2.addWidget(self.urlBar)

        self.retranslateUi(PackageDetails)

        QMetaObject.connectSlotsByName(PackageDetails)

    # setupUi

    def retranslateUi(self, _):
        self.buttonBack.setText("")
        self.buttonInstall.setText(
            QCoreApplication.translate("AddonsInstaller", "Install", None)
        )
        self.buttonUninstall.setText(
            QCoreApplication.translate("AddonsInstaller", "Uninstall", None)
        )
        self.buttonUpdate.setText(
            QCoreApplication.translate("AddonsInstaller", "Update", None)
        )
        self.buttonCheckForUpdate.setText(
            QCoreApplication.translate("AddonsInstaller", "Check for Update", None)
        )
        self.buttonExecute.setText(
            QCoreApplication.translate("AddonsInstaller", "Run Macro", None)
        )
        self.buttonBack.setToolTip(
            QCoreApplication.translate(
                "AddonsInstaller", "Return to package list", None
            )
        )

    # retranslateUi
