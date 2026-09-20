const {danger, fail, message, warn} = require('danger');
const SOURCE_REGEX = /.*\.(cpp|c|h|lua)$/i
const TITLE_REGEX = /^(fix|improve|add|infra)/i
const touched_files = [...danger.git.created_files, ...danger.git.modified_files]
const sourcefiles = touched_files.filter(item => item.match(SOURCE_REGEX))
const pr_title = danger.github.pr.title

// Checks the title to make sure it matches expectations
const title_type = pr_title.match(TITLE_REGEX)
if (title_type) {
  // no-op
} else if(pr_title.match(/^\[?WIP\]?/i)) {
  fail("PR is still a WIP, do not merge")
} else {
  fail("PR title must start with `fix`, `improve`, `add` or `infra` for release notes purposes.")
}

// checks sourcefile changes to ensure any new TODO items also have a Mudlet issue
sourcefiles.forEach(function(filename) {
  const additions = danger.git.diffForFile(filename)
  additions.then(diff => {
    diff.added.split("\n").forEach(function(item) {
      if (item.includes("TODO:")) {
          fail(`Source file ${filename} includes a TODO! Can you make the change right away? If no, better not to create a TODO - they just tend to hang around and never get fixed.`)
      }
    })
  })
})

// Warns if a PR touched more than 10 source files.
if (sourcefiles.length > 10) {
  warn(`PR makes changes to ${sourcefiles.length} source files. Double check the scope hasn't gotten out of hand`)
}

// Warns if the title is perhaps a bit verbose
const title_wordcount = pr_title.split(" ").length
if (title_wordcount > 25) {
  warn(`PR title is ${title_wordcount} words long, double check it will make a good changelog line`)
}
