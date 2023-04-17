#!/bin/bash

/home/mind/neuralClassifiersComparison/stopClassifier.sh   # stop eventual executions

PS3="Chose Neural Inference Engine to use with the plugin or Quit: " 
options=(\
         "Quit")
select opt in "${options[@]}"
do
    case $opt in
        "")
            echo "You chose "$opt
            CONFIG_FILE_PATH="/home/mind/neuralClassifiersComparison/configs/config_.json"
            break
            ;;
        "Quit")
            break
            ;;
        *) echo "invalid option $REPLY";;
    esac
done
# Launch sushi with classifier plugin
sushi -r  --timing-statistics -c $CONFIG_FILE_PATH &

sleep 2



ORIGLOGNAME=$(ls /tmp/guit* | tail -1 | grep -o -P "guit.+")
STATSLOGNAME="/tmp/$ORIGLOGNAME.cpumem.log" 

/home/mind/neuralClassifiersComparison/logutils/logcpumem.py $(pgrep sushi) $STATSLOGNAME &

echo "Classifier log at $ORIGLOGNAME"
echo ""
echo "CPU/MEM usage log at $STATSLOGNAME"
echo ""
