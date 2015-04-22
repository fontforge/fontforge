echo "Type the name of the dmg"
read input_variable
appdmg `dirname "$0"`/appdmg.json ~/Desktop/$input_variable.dmg
