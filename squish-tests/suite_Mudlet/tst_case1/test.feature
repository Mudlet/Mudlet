# This is a sample .feature file
# Squish feature files use the Gherkin language for describing features, a short example
# is given below. You can find a more extensive introduction to the Gherkin format at
# https://github.com/cucumber/cucumber/wiki/Gherkin
Feature: Profile basics (creating, editing, renaming, deleting them)

    This feature allows people to work with Mudlets concept of being profile-based
    and will test the basics of working with profiles.

    Scenario: Selecting an existing profile

        Given the connection dialog is open
         When the Avalon.de profile is selected
         Then the profile name is "Avalon.de"
          And the server address is "avalon.mud.de"
          And the port is "23"
          