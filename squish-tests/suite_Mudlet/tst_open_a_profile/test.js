function main() {
    startApplication("mudlet");
    clickButton(waitForObject(":MainWindow.Connect_QToolButton"));
    waitForObjectItem(":profile_dialog.profiles_tree_widget_QListWidget", "Avalon\\.de");
    clickItem(":profile_dialog.profiles_tree_widget_QListWidget", "Avalon\\.de", 81, 22, 0, Qt.LeftButton);
    test.vp("Check that Avalon connection info is loaded");

    test.compare(waitForObjectExists(":requiredArea.host_name_entry_QLineEdit").text, "avalon.mud.de");
    test.compare(waitForObjectExists(":requiredArea.port_entry_QLineEdit").text, "23");
    test.compare(waitForObjectExists(":informationalArea.website_entry_QLabel").text, "<center><a href='http://avalon.mud.de'>http://avalon.mud.de</a></center>");

    clickButton(waitForObject(":profile_dialog.Connect_QPushButton"));
    test.vp("Check that Lua modules & LuaGlobal are loaded");
}
