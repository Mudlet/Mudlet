function main() {
    startApplication("mudlet");
    clickButton(waitForObject(":MainWindow.Connect_QToolButton"));
    waitForObjectItem(":profile_dialog.profiles_tree_widget_QListWidget", "Avalon\\.de");
    doubleClickItem(":profile_dialog.profiles_tree_widget_QListWidget", "Avalon\\.de", 26, 9, 0, Qt.LeftButton);


    type(waitForObject(":MainWindow_TCommandLine"), "lua disconnect()");
    type(waitForObject(":MainWindow_TCommandLine"), "<Return>");

    clickButton(waitForObject(":MainWindow.Triggers_QToolButton"));
    clickButton(waitForObject(":trigger_editor.Add Item_QToolButton"));
    mouseDrag(waitForObject(":lineEdit_trigger_name_QLineEdit"), 114, 13, -156, -17, 1, Qt.LeftButton);
    type(waitForObject(":lineEdit_trigger_name_QLineEdit"), "Simple highlight trigger");
    type(waitForObject(":lineEdit_trigger_name_QLineEdit"), "<Tab>");
    type(waitForObject(":trigger_command_QLineEdit"), "<Tab>");
    mouseClick(waitForObject(":scrollArea.lineEdit_QLineEdit"), 59, 11, 0, Qt.LeftButton);
    type(waitForObject(":scrollArea.lineEdit_QLineEdit"), "highlight this red");
    waitForObject(":frame_rightTop.colorizerTrigger_QGroupBox").setChecked(true);
    clickButton(waitForObject(":trigger_editor.Save Item_QToolButton"));
    test.compare(waitForObjectExists(":Triggers.Simple highlight trigger_QModelIndex").text, "Simple highlight trigger");
    test.compare(waitForObjectExists(":Triggers.Simple highlight trigger_QModelIndex").selected, true);
    test.compare(waitForObjectExists(":lineEdit_trigger_name_QLineEdit").displayText, "Simple highlight trigger");
    test.compare(waitForObjectExists(":frame_rightTop.colorizerTrigger_QGroupBox").checked, true);
    mouseClick(waitForObject(":MainWindow_TTextEdit"), 774, 627, 0, Qt.LeftButton);

    type(waitForObject(":MainWindow_TCommandLine"), "`echo highlight this red");
    type(waitForObject(":MainWindow_TCommandLine"), "<Return>");
    type(waitForObject(":MainWindow_TCommandLine"), "<Right>");
    type(waitForObject(":MainWindow_TCommandLine"), "<Ctrl+Shift+Left>");
    type(waitForObject(":MainWindow_TCommandLine"), "blue");
    type(waitForObject(":MainWindow_TCommandLine"), "<Return>");
    type(waitForObject(":MainWindow_TCommandLine"), "<Backspace>");
    test.vp("VP3");

    clickButton(waitForObject(":trigger_editor.Delete Item_QToolButton"));
    sendEvent("QCloseEvent", waitForObject(":MainWindow_mudlet"));
}
