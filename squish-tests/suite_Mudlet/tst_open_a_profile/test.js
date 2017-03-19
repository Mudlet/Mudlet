function main() {
    startApplication("mudlet");
    clickButton(waitForObject(":MainWindow.Connect_QToolButton"));
    waitForObjectItem(":profile_dialog.profiles_tree_widget_QListWidget", "Avalon\\.de");
    clickItem(":profile_dialog.profiles_tree_widget_QListWidget", "Avalon\\.de", 81, 22, 0, Qt.LeftButton);
    clickButton(waitForObject(":profile_dialog.Connect_QPushButton"));
}
