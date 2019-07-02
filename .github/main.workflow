workflow "Update translations" {
  resolves = ["Commit back changes"]
  on = "pull_request"
}

action "Check out correct branch" {
  uses = "gr2m/git-checkout-pull-request-action@master"
  secrets = ["GITHUB_TOKEN"]
}

action "Run lupdate" {
  uses = "Mudlet/lupdate-action@master"
  needs = ["Check out correct branch"]
  args = "./src/ -ts ./translations/mudlet.ts"
}


action "Commit back changes" {
  uses = "docker://cdssnc/auto-commit-github-action"
  needs = ["Run lupdate"]
  args = "Updated texts for translators"
  secrets = ["GITHUB_TOKEN"]
}
