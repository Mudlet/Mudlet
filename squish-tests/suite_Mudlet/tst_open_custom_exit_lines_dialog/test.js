function main() {
    startApplication("mudlet");
    clickButton(waitForObject(":MainWindow.Connect_QToolButton"));
    waitForObjectItem(":profile_dialog.profiles_tree_widget_QListWidget", "Achaea");
    doubleClickItem(":profile_dialog.profiles_tree_widget_QListWidget", "Achaea", 70, 5, 0, Qt.LeftButton);
    type(waitForObject(":MainWindow_TCommandLine"), "lua createMapper(0,0,500,500) setMapZoom(10) setBorderLeft(500) centerview(3000)");
    type(waitForObject(":MainWindow_TCommandLine"), "<Return>");
    mouseClick(waitForObject(":MainWindow.mp2dMap_T2DMap"), 153, 145, 0, Qt.LeftButton);
    openContextMenu(waitForObject(":MainWindow.mp2dMap_T2DMap"), 153, 145, 0);
    activateItem(waitForObjectItem(":MainWindow_QMenu", "custom exit lines"));
    test.compare(waitForObjectExists(":roomExits_QDialog").minimized, false);
    test.compare(waitForObjectExists(":roomExits_QDialog").visible, true);
    test.compare(waitForObjectExists(":roomExits_QDialog").isActiveWindow, true);
    test.compare(waitForObjectExists(":roomExits_QDialog").windowTitle, "Custom Line selection");
}
