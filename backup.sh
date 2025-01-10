projects="/data/share/projects"

home="/home/pi/gnius-sendevent"
project="$projects/gnius-sendevent"

sudo rm -rf $project.old >/dev/null 2>/dev/null
sudo mv $project $project.old
sudo cp -rp $home $project
