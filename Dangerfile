# Automates some of the checks for PRs
# Declarations and collections which are used in future checks and may be reused in future.
PROJ = "Mudlet"
REPO = "Mudlet"
issue_regexp = Regexp.new(/https?:\/\/(?:www\.)?github\.com\/#{PROJ}\/#{REPO}\/issues\/(\d+)/i)
issue_url = "https://github.com/#{PROJ}/#{REPO}/issues/"
sourcefile_regexp = Regexp.new(/.*\.(cpp|c|h|lua)$/)
sourcefiles = (git.modified_files + git.added_files).uniq.select { |file| file.match(sourcefile_regexp) }


## Checks title formatting. This formatting is used for changelog and release note generation.
title_regex = Regexp.new(/^(fix|improve|add)/i)
if github.pr_title.match(/^\[?WIP\]?\:/i)
  failure("PR is still a WIP, do not merge") 
elsif not github.pr_title.match(title_regex)
  failure("PR title must start with `fix`, `improve`, or `add` in order to be merged. Not case sensitive.")
else
  pr_type = github.pr_title[title_regex,1]
  type_to_readable = { add: "Addition", fix: "Fix", improve: "Improvement" }
  display_type = type_to_readable[pr_type.downcase.to_sym]
  message("PR title is proper (type: `#{display_type}`)")
end

## Gathers and displays TODOs, failing them if a source file contains a TODO with no github issue
bad_todos = []
added_todos = {}
sourcefiles.each do |filename|
  additions = git.diff_for_file(filename).patch.lines.grep(/^\+/)
  additions.each do |line|
    if line.include?("TODO")
      has_issue = line.match(issue_regexp)
      if has_issue
        added_todos[filename] ||= []
        added_todos[filename] << has_issue[1]
      else
        bad_todos << filename
      end
    end
  end
end
bad_todos.uniq!
total_todos = bad_todos.count + added_todos.count
if total_todos > 0
  todo_msg = "## #{total_todos} file(s) with new TODO(s).\n\n"
  todo_msg += "### #{added_todos.count} files with links\n\n" if added_todos.count > 0
  added_todos.each do |filename, issues|
    issue_links = issues.map {|issue| "[#{issue}](https://github.com/#{PROJ}/#{REPO}/issues/#{issue})" }
    todo_msg += "#{filename} adds issue(s): #{issue_links.join(', ')}\n"
  end
  if bad_todos.size > 0
    todo_msg += "### #{bad_todos.size} files with bad TODOs added\n\n"
    todo_msg += "* " + bad_todos.join("\n* ")
  end
  markdown(todo_msg)
end
failure("TODO added without an issue link, see below for list of files.") if bad_todos.count > 0
