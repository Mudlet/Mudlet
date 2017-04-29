// This file contains hook functions to run as the .feature file is executed.
//
// A common use-case is to use the OnScenarioStart/OnScenarioEnd hooks to
// start and stop an AUT, e.g.
//
// OnScenarioStart(function(context) {
//     startApplication("addressbook");
// });
//
// OnScenarioEnd(function(context) {
//     currentApplicationContext().detach();
// });
//
// See the section 'Performing Actions During Test Execution Via Hooks' in the Squish
// manual for a complete reference of the available API.
//

// couldn't find an equivalent function in the Squish API... and
// https://doc.froglogic.com/squish/6.2/rgs-js.html#js-OS.removeRecursively-function
// seems to suggest this is the way to go.
// will fatally fail if it's unable to ensure that a directory exists
function mkdir(path) {
    var command;
    if (OS.name == "Linux" || OS.name == "Darwin") {
        command = "mkdir -p " + path;
    } else if (OS.name == "Windows") {
        // md is better, http://stackoverflow.com/questions/905226/mkdir-p-linux-windows#comment713410_905237
        command = "md " + path;
    }

    var result = OS.system(command);
    if (result != 0) {
        test.fatal("'" + command + "' failed: " + result);
    }
}

function getMudletHomeDir() {
    var homeDir;
    if (OS.name == "Linux" || OS.name == "Darwin") {
        homeDir = OS.getenv("HOME");
    } else if (OS.name == "Windows") {
        homeDir = OS.getenv("HOMEPATH");
    } else {
        test.fatal("Sorry, but " + OS.name + " is not supported by Squish tests")
    }

    var sp = File.separator;
    var mudletProfileDir = homeDir + sp + ".config" + sp + "mudlet" + sp + "profiles";

    if (!File.exists(mudletProfileDir)) {
        mkdir(mudletProfileDir);
    }
    
    return mudletProfileDir;
}

// ensure squish profile is created and is blank
OnFeatureStart(function(context) {
    var mudletHomeDir = getMudletHomeDir();
    var squishProfileDir = mudletHomeDir + File.separator + "squish";
    
    // check for existence of "squish-test-profile" file that indicates this is not a user-created
    // "squish" profile that we're about to stuff up
    if (File.exists(squishProfileDir) && !File.exists(squishProfileDir + File.separator + "squish-test-profile")) {
        test.fatal("The existing 'squish' profile lacks the 'squish-test-profile' file and does not "
            + "seem to be a testing profile.");
    }

    // creating Qt objects like this doesn't work
    // var existingDir = QDir(squishProfileDir);
    // existingDir.removeRecursively();

    mkdir(squishProfileDir);
    var file = File.open(squishProfileDir + File.separator + "squish-test-profile", "w");
    file.write("This file indicates that this is a Squish testing profile and can used for automated testing.");
    file.close();
    
    var portFile = findFile("testdata", "mudlet-profile-setup" + File.separator + "port");
    var urlFile  = findFile("testdata", "mudlet-profile-setup" + File.separator + "url");
    
    // once deleting the entire directory works above, this should be a straight copy without
    // checking for existence
    var targetPortFile   = squishProfileDir + File.separator + "port";
    var targetSquishFile = squishProfileDir + File.separator + "url";
    if (!File.exists(targetPortFile)) {
        File.copy(portFile, targetPortFile);   
    }
    if (!File.exists(targetSquishFile)) {
        File.copy(urlFile,  targetSquishFile);    
    }        
});

// detach (i.e. potentially terminate) all AUTs at the end of a scenario
OnScenarioEnd(function(context) {
    applicationContextList().forEach(function(ctx) {
        ctx.detach();
    });
});

