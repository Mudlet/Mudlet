workflow "Update translations" {
  resolves = ["Commit back changes"]
  on = "pull_request"
}

action "Check out branch" {
  uses = "gr2m/git-checkout-pull-request-action@master"
  secrets = ["GITHUB_TOKEN"]
}

action "Run lupdate" {
  uses = "Mudlet/lupdate-action@master"
  needs = ["Check out branch"]
  args = "./src/ -ts ./translations/mudlet.ts"
}


action "Commit changes" {
  uses = "docker://cdssnc/auto-commit-github-action"
  needs = ["Run lupdate"]
  args = "Updated texts for translators"
  secrets = ["GITHUB_TOKEN"]
}
