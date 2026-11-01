# Sets <out_var> to the suffix that follows APP_VERSION, from the environment
# variable MUDLET_VERSION_BUILD. An explicitly empty value must be distinguished
# from an unset one so release builds (CI exports MUDLET_VERSION_BUILD="") get a
# clean version with no "-dev-<sha>" suffix:
#   unset (local dev build)         -> "-dev-<sha>"
#   defined but empty (release)     -> "" (no suffix)
#   defined with a value (ptb/test) -> "<value>-<sha>", or "<value>" alone when
#                                      <append_sha> is false, for a distributor's
#                                      own label
function(mudlet_build_suffix out_var git_sha1 append_sha)
  if(DEFINED ENV{MUDLET_VERSION_BUILD})
    if("$ENV{MUDLET_VERSION_BUILD}" STREQUAL "")
      set(suffix "")
    elseif(append_sha)
      set(suffix "$ENV{MUDLET_VERSION_BUILD}-${git_sha1}")
    else()
      set(suffix "$ENV{MUDLET_VERSION_BUILD}")
    endif()
  else()
    set(suffix "-dev-${git_sha1}")
  endif()
  set(${out_var} "${suffix}" PARENT_SCOPE)
endfunction()
