workflow "Update translation texts" {
  on = "push"
  resolves = ["label PR"]
}

action "Filters for GitHub Actions" {
  uses = "actions/bin/filter@master"
  args = "branch development"
}

action "cpp fileses were touched" {
  uses = "docker://cdssnc/touched-github-action"
  needs = ["Filters for GitHub Actions"]
  args = "**cpp"
}

action "run lupdate" {
  uses = "Mudlet/lupdate-action@master"
  needs = ["cpp fileses were touched"]
  args = "./src/ -ts ./translations/mudlet.ts"
}

action "Commit to a branch and push" {
  uses = "Mudlet/commit-action-branch@master"
  needs = ["run lupdate"]
  secrets = ["GITHUB_TOKEN"]
  env = {
    USERNAME = "mudlet-bot"
    USEREMAIL = "info@mudlet.org"
    BRANCH_NAME = "translations/translation-updates"
  }
}

action "Create PR" {
  uses = "vsoch/pull-request-action@master"
  needs = ["Commit to a branch and push"]
  secrets = ["GITHUB_TOKEN"]
  env = {
    BRANCH_PREFIX = "translations/"
    PULL_REQUEST_BRANCH = "development"
    PULL_REQUEST_TITLE = "Update texts for translators"
  }
}

workflow "Approve translation texts PRs" {
  on = "pull_request"
  resolves = ["hmarr/auto-approve-action@master"]
}

action "Filters for GitHub Actions-1" {
  uses = "actions/bin/filter@master"
  args = "label translations"
}

action "label PR" {
  uses = "TimonVS/pr-labeler@master"
  needs = ["Create PR"]
  secrets = ["GITHUB_TOKEN"]
}

action "hmarr/auto-approve-action@master" {
  uses = "hmarr/auto-approve-action@master"
  needs = ["Filters for GitHub Actions-1"]
  secrets = ["GITHUB_TOKEN"]
}
