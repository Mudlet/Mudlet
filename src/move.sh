#!/bin/bash

set -e

# List of functions to be moved
functions=(
addAreaName
addCustomLine
addMapEvent
addMapMenu
addRoom
addSpecialExit
auditAreas
centerview
clearAreaUserData
clearAreaUserDataItem
clearMapSelection
clearMapUserData
clearMapUserDataItem
clearRoomUserData
clearRoomUserDataItem
clearSpecialExits
closeMapWidget
connectExitStub
createMapLabel
createMapImageLabel
createMapper
createRoomID
deleteArea
deleteMap
deleteMapLabel
deleteRoom
disableMapInfo
enableMapInfo
getAllAreaUserData
getAllMapUserData
getAllRoomEntrances
getAllRoomUserData
getAreaExits
getAreaRooms
getAreaTable
getAreaTableSwap
getAreaUserData
getCustomEnvColorTable
getCustomLines
getCustomLines1
getDoors
getExitStubs
getExitStubs1
getExitWeights
getGridMode
getMapEvents
getMapLabel
getMapLabels
getMapMenus
getMapSelection
getMapUserData
getMapZoom
getPath
getPlayerRoom
getRoomArea
getRoomAreaName
getRoomChar
getRoomCharColor
getRoomCoordinates
getRoomEnv
getRoomExits
getRoomHashByID
getRoomIDbyHash
getRoomName
getRooms
getRoomsByPosition
getRoomUserData
getRoomUserDataKeys
getRoomWeight
getSpecialExits
getSpecialExitsSwap
gotoRoom
hasExitLock
hasSpecialExitLock
highlightRoom
killMapInfo
loadJsonMap
loadMap
lockExit
lockRoom
lockSpecialExit
moveMapWidget
openMapWidget
pauseSpeedwalk
registerMapInfo
resumeSpeedwalk
removeCustomLine
removeMapEvent
removeMapMenu
removeSpecialExit
resetRoomArea
resizeMapWidget
roomExists
roomLocked
saveJsonMap
saveMap
searchAreaUserData
searchRoom
searchRoomUserData
setAreaName
setAreaUserData
setCustomEnvColor
setDoor
setExit
setExitStub
setExitWeight
setGridMode
setMapUserData
setMapZoom
setRoomArea
setRoomChar
setRoomCharColor
setRoomCoordinates
setRoomEnv
setRoomIDbyHash
setRoomName
setRoomUserData
setRoomWeight
speedwalk
stopSpeedwalk
unHighlightRoom
unsetRoomCharColor
)

# Source and destination files
src_file="TLuaInterpreter.cpp"
dest_file="TLuaInterpreterMapper.cpp"

# Loop over the functions
for func in "${functions[@]}"; do
    echo "Processing function: $func"
    # Use ctags to find the function definition line number
    ctags -x --c++-kinds=f ${src_file} | grep "^${func} " | while read -r line ; do
        echo "Found function definition at line: $line"
        start_line=$(echo ${line} | awk '{print $3}')
        start_line=$((start_line - 1))
        echo "Actual start line is: $start_line"
                # Use awk to find the end line
        end_line=$(awk "/^${func} /{flag=1;next}/^}/{if(flag){print NR;exit}}" ${src_file})

        # If end_line is empty, use ctags and awk to find the end line
        if [ -z "$end_line" ]; then
            end_line=$(awk -v start=$start_line 'NR > start && /^}/ {print NR; exit}' ${src_file})
        fi

        # add 1 to the end line and save it
        end_line=$((end_line + 1))

        # Extract the function using sed and append it to the destination file
        echo "Extracting function from lines $start_line to $end_line"
        sed -n "${start_line},${end_line}p" ${src_file} >> ${dest_file}
        echo "Appended extracted function to destination file"

        # Remove the function from the source file
        echo "Removing function from source file"
        sed -i.bak -e "${start_line},${end_line}d" ${src_file}
        echo "Function removed from source file"

        # exit script after 1 iteration
        break
    done
done
