const {danger, fail, message, warn} = require('danger');
const ISSUE_REGEX = /https?:\/\/(?:www\.)?github\.com\/Mudlet\/Mudlet\/issues\/(\d+)/i
const ISSUE_URL = "https://github.com/Mudlet/Mudlet/issues"
const SOURCE_REGEX = /.*\.(cpp|c|h|lua)$/i
const TITLE_REGEX = /^(fix|improve|add|infra)/i
const touched_files = [...danger.git.created_files, ...danger.git.modified_files]
const sourcefiles = touched_files.filter(item => item.match(SOURCE_REGEX))
const pr_title = danger.github.pr.title

// Checks the title to make sure it matches expectations
if (pr_title.match(TITLE_REGEX)) {
  title_type = pr_title.match(TITLE_REGEX)
  const type_to_readable = {
    add: "Addition",
    fix: "Fix",
    improve: "Improvement"
  }
  message(`PR type: \`${type_to_readable[title_type[0].toLowerCase()]}\``)
} else if(pr_title.match(/^\[?WIP\]?/i)) {
  fail("PR is still a WIP, do not merge")
} else {
  fail("PR title must start with `fix, `improve`, `add` or `infra` for release notes purposes.")
}

// checks sourcefile changes to ensure any new TODO items also have a Mudlet issue
sourcefiles.forEach(function(filename) {
  let additions = danger.git.diffForFile(filename)
  additions.then(diff => {
    var issues = []
    diff.added.split("\n").forEach(function(item) {
      if (item.includes("TODO:")) {
        let has_issue = item.match(ISSUE_REGEX)
        if (!has_issue) {
          fail(`Source file ${filename} includes a TODO with no Mudlet issue link.\n  New TODO items in source files must have an accompanying github issue`)
        } else {
          issues.push(has_issue[1])
        }
      }
    })
    if (issues.length > 0) {
      message(`\`${filename}\` adds TODO issues: ${issues.map(iss => `[${iss}](${ISSUE_URL}/${iss})`).join(", ")}`)
    }
  })
})

// Warns if a PR touched more than 10 source files.
if (sourcefiles.length > 10) {
  warn(`PR makes changes to ${sourcefiles.length} source files. Double check the scope hasn't gotten out of hand`)
}

// Warns if the title is perhaps a bit verbose
title_wordcount = pr_title.split(" ").length
if (title_wordcount > 25) {
  warn(`PR title is ${title_wordcount} words long, double check it will make a good changelog line`)
}
