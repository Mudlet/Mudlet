# This is a sample .feature file
# Squish feature files use the Gherkin language for describing features, a short example
# is given below. You can find a more extensive introduction to the Gherkin format at
# https://github.com/cucumber/cucumber/wiki/Gherkin
Feature: Profile basics (creating, editing, renaming, deleting them)

    This feature allows people to work with Mudlets concept of being profile-based
    and will test the basics of working with profiles.

    Scenario: Selecting an existing profile loads profile information

        Given the connection dialog is open
         When the 'squish' profile is selected
         Then the profile name is 'squish'
          And the server address is 'fake-test-mud.com'
          And the port is '23'
          
    Scenario: Creating and loading a new profile saves the data for it
        
        Given the connection dialog is open
         When the New button is pressed
         Then the profile name is 'new profile name'