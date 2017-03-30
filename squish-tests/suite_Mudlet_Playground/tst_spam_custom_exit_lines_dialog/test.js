function main() {
    var x = 145;
    var y = 150;
    
    startApplication("mudlet");
    clickButton(waitForObject(":MainWindow.Connect_QToolButton"));
    waitForObjectItem(":profile_dialog.profiles_tree_widget_QListWidget", "Achaea");
    doubleClickItem(":profile_dialog.profiles_tree_widget_QListWidget", "Achaea", 70, 5, 0, Qt.LeftButton);
    type(waitForObject(":MainWindow_TCommandLine"), "lua createMapper(0,0,500,500) setMapZoom(10) setBorderLeft(500) centerview(3000)");
    type(waitForObject(":MainWindow_TCommandLine"), "<Return>");
 
    
    var step_x, step_y;
    // try every combination of x and y values from the current to current + however many pixels
    // start bottom left and work our way up by decreasing y and increasing x
    for (step_x = x; step_x < (x+20); step_x++) {
        for (step_y = y; step_y > (y-20); step_y--) {
            compare(step_x, step_y);
        }   
    }    
}

function compare(x,y) {
    mouseClick(waitForObject(":MainWindow.mp2dMap_T2DMap"), 0, 0, 0, Qt.LeftButton);    
    mouseClick(waitForObject(":MainWindow.mp2dMap_T2DMap"), x, y, 0, Qt.LeftButton);
    openContextMenu(waitForObject(":MainWindow.mp2dMap_T2DMap"), x, y, 0);
//    snooze(.5);
    activateItem(waitForObjectItem(":MainWindow_QMenu", "custom exit lines"));
//    test.compare(waitForObjectExists(":roomExits_QDialog").minimized, false);
//    test.compare(waitForObjectExists(":roomExits_QDialog").visible, true);
//    test.compare(waitForObjectExists(":roomExits_QDialog").isActiveWindow, true);
//    test.compare(waitForObjectExists(":roomExits_QDialog").windowTitle, "Custom Line selection");
//    sendEvent("QCloseEvent", waitForObject(":roomExits_QDialog"));
    if (object.exists(":roomExits_QDialog")) { 
        sendEvent("QCloseEvent", ":roomExits_QDialog");
    }
    
//    snooze(.5);
//    mouseDrag(waitForObject(":MainWindow_TTextEdit"), 105, 163, 49, 53, 1, Qt.LeftButton);
}
