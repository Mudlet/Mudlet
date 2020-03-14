binaryheap.lua
==============

[Binary heap](http://en.wikipedia.org/wiki/Binary_heap) implementation

Both the [source code](https://github.com/Tieske/binaryheap.lua) as well as the
[documentation](http://tieske.github.io/binaryheap.lua) are on github

Based on [original code](http://lua-users.org/lists/lua-l/2015-04/msg00137.html)
by Oliver Kroth, with
[extras](http://lua-users.org/lists/lua-l/2015-04/msg00133.html)
as proposed by Sean Conner.

Contributions
=============
This library was create by contributions from Oliver Kroth,
Thijs Schreijer, Boris Nagaev

History
=======

Version 0.4, 7-Nov-2018

 - [breaking] added additional tests, mostly on returning errors, minor behaviour changes
 - added `size` method
 - fixed a lot of linter issues
 
Version 0.3, 15-Jul-2018

 - bugfix `unique:pop` returning wrong order results (by Daurnimator)
 - change `unique:peek` returning same order as `pop`
 - added `unique:peekValue` returning just the value

Version 0.2, 21-Apr-2015

 - bugfix `remove` function (by Boris Nagaev)
 - configurable comparison function for the tree

Version 0.1, 20-Apr-2015

 - Initial release


Copyright
=========
Copyright 2015-2018 Thijs Schreijer

License
=======
MIT/X11
