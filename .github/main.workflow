workflow "Update translation texts" {
  on = "push"
  resolves = ["label PR"]
}

action "Filters for GitHub Actions" {
  uses = "actions/bin/filter@master"
  args = "branch development"
}

action "cpp or ui files updated" {
  uses = "docker://cdssnc/touched-github-action"
  needs = ["Filters for GitHub Actions"]
  args = "{**cpp, **ui}"
}

action "run lupdate" {
  uses = "Mudlet/lupdate-action@master"
  needs = ["cpp or ui files updated"]
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

workflow "Approve translation texts PRs" {
  on = "pull_request"
  resolves = ["hmarr/auto-approve-action@master"]
}

action "Run only on translation PRs" {
  uses = "actions/bin/filter@master"
  args = "label translations"
}

action "hmarr/auto-approve-action@master" {
  uses = "hmarr/auto-approve-action@master"
  needs = ["Run only on translation PRs"]
  secrets = ["GITHUB_TOKEN"]
}

workflow "automerge pull requests on updates" {
  on = "pull_request"
  resolves = ["automerge"]
}

workflow "automerge pull requests on reviews" {
  on = "pull_request_review"
  resolves = ["automerge"]
}

workflow "automerge pull requests on status updates" {
  on = "status"
  resolves = ["automerge"]
}

action "automerge" {
  uses = "pascalgn/automerge-action@master"
  secrets = ["GITHUB_TOKEN"]
  env = {
    AUTOMERGE = "translations"
    MERGE_METHOD = "rebase"
  }
}

workflow "delete translation branch" {
  on = "pull_request"
  resolves = ["branch cleanup"]
}

action "branch cleanup" {
  needs = "Run only on translation PRs"
  uses = "jessfraz/branch-cleanup-action@master"
  secrets = ["GITHUB_TOKEN"]
}

workflow "Create translation update PR" {
  on = "push"
  resolves = ["Create PR"]
}

action "Create PR" {
  uses = "vsoch/pull-request-action@master"
  secrets = ["GITHUB_TOKEN"]
  env = {
    BRANCH_PREFIX = "translations/"
    PULL_REQUEST_BRANCH = "development"
    PULL_REQUEST_TITLE = "Update texts for translators"
  }
}

workflow "Label translation update PR" {
  on = "pull_request"
  resolves = ["label PR"]
}

action "label PR" {
  uses = "TimonVS/pr-labeler@master"
  secrets = ["GITHUB_TOKEN"]
}
