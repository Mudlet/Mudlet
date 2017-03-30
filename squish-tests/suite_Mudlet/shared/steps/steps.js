// A quick introduction to implementing scripts for BDD tests:
//
// This file contains snippets of script code to be executed as the .feature
// file is processed. See the section 'Behaviour Driven Testing' in the 'API
// Reference Manual' chapter of the Squish manual for a comprehensive reference.
//
// The functions Given/When/Then/Step can be used to associate a script snippet
// with a pattern which is matched against the steps being executed. Optional
// table/multi-line string arguments of the step are passed via a mandatory
// 'context' parameter:
//
//   When("I enter the text", function(context) {
//     <code here>
//   });
//
// The pattern is a plain string without the leading keyword, but a couple of
// placeholders including |any|, |word| and |integer| are supported which can
// be used to extract arbitrary, alphanumeric and integer values resp. from the
// pattern; the extracted values are passed as additional arguments:
//
//   Then("I get |integer| different names", function(context, numNames) {
//     <code here>
//   });
//
// Instead of using a string with placeholders, a regular expression object can
// be passed to Given/When/Then/Step to use regular expressions.
//

OnFeatureStart(function(context) {
    startApplication("mudlet");
});

Given("the connection dialog is open", function(context) {
    clickButton(waitForObject(":MainWindow.Connect_QToolButton"));
});

When("the Avalon.de profile is selected", function(context) {
    waitForObjectItem(":profile_dialog.profiles_tree_widget_QListWidget", "Avalon\\.de");
    clickItem(":profile_dialog.profiles_tree_widget_QListWidget", "Avalon\\.de", 81, 22, 0, Qt.LeftButton);
});
