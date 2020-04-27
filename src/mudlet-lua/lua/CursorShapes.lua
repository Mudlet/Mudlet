mudlet = mudlet or {}
mudlet.cursor = {
  --see https://doc.qt.io/qt-5/qt.html#CursorShape-enum
  ["Reset"]              =  -1,      -- Resets your cursor.
  ["Arrow"]              =   0,      -- The standard arrow cursor.
  ["UpArrow"]            =   1,      -- An arrow pointing upwards toward the top of the screen.
  ["Cross"]              =   2,      -- A crosshair cursor, typically used to help the user accurately select a point on the screen.
  ["Wait"]               =   3,      -- An hourglass or watch cursor, usually shown during operations that prevent the user from interacting with the application.
  ["IBeam"]              =   4,      -- A caret or ibeam cursor, indicating that a widget can accept and display text input.
  ["ResizeVertical"]     =   5,      -- A cursor used for elements that are used to vertically resize top-level windows.
  ["ResizeHorizontal"]   =   6,      -- A cursor used for elements that are used to horizontally resize top-level windows.
  ["ResizeTopRight"]     =   7,      -- A cursor used for elements that are used to diagonally resize top-level windows at their top-right and bottom-left corners.
  ["ResizeTopLeft"]      =   8,      -- A cursor used for elements that are used to diagonally resize top-level windows at their top-left and bottom-right corners.
  ["ResizeAll"]          =   9,      -- A cursor used for elements that are used to resize top-level windows in any direction.
  ["Blank"]              =   10,     -- A blank/invisible cursor, typically used when the cursor shape needs to be hidden.
  ["VerticalSplit"]      =   11,     -- A cursor used for vertical splitters, indicating that a handle can be dragged horizontally to adjust the use of available space.
  ["HorizontalSplit"]    =   12,     -- A cursor used for horizontal splitters, indicating that a handle can be dragged vertically to adjust the use of available space.
  ["PointingHand"]       =   13,     -- A pointing hand cursor that is typically used for clickable elements such as hyperlinks.
  ["Forbidden"]          =   14,     -- A slashed circle cursor, typically used during drag and drop operations to indicate that dragged content cannot be dropped on particular widgets or inside certain regions.
  ["OpenHand"]           =   17,     -- A cursor representing an open hand, typically used to indicate that the area under the cursor is the visible part of a canvas that the user can click and drag in order to scroll around.
  ["ClosedHand"]         =   18,     -- A cursor representing a closed hand, typically used to indicate that a dragging operation is in progress that involves scrolling.
  ["WhatsThis"]          =   15,     -- An arrow with a question mark, typically used to indicate the presence of What's This? help for a widget.
  ["Busy"]               =   16,     -- An hourglass or watch cursor, usually shown during operations that allow the user to interact with the application while they are performed in the background.
  ["DragMove"]           =   20,     -- A cursor that is usually used when dragging an item.
  ["DragCopy"]           =   19,     -- A cursor that is usually used when dragging an item to copy it.
  ["DragLink"]           =   21,     -- A cursor that is usually used when dragging an item to make a link to it.
}
