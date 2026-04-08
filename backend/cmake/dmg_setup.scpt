on run argv
    set volumeName to item 1 of argv
    tell application "Finder"
        tell disk volumeName
            open
            set current view of container window to icon view
            set toolbar visible of container window to false
            set statusbar visible of container window to false
            set the bounds of container window to {400, 100, 885, 330}
            set theViewOptions to the icon view options of container window
            set arrangement of theViewOptions to not arranged
            set icon size of theViewOptions to 72
            set background picture of theViewOptions to file ".background:background.png"
            make new alias file at container window to POSIX file "/Applications" with properties {name:"Applications"}
            set position of item "EnvMonitor" of container window to {140, 120}
            set position of item "Applications" of container window to {385, 120}
            close
            open
            update without registering applications
            delay 5
        end tell
    end tell
end run
