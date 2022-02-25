#!/usr/bin/env bash

set -e 

ELK_DESTINATION_FOLDER='/home/mind/neuralClassifiersComparison'
ELK_DESTINATION_IP='dom-elk-pi.local'

BASE=$(realpath $(pwd))

COPY_TO_REMOTE="true"
# COPY_TO_REMOTE="false"

get_bname () {
    echo $(echo "$1" | grep -o -P -e '-[^/]+' | grep -o -P -e '[a-zA-Z]+' | tail -1)
}

print_compile_script () {
    echo '#!/bin/bash' > $1
    echo '' >> $1
    echo '# Cross-compilation script for ElkOS on RPI4' >> $1
    echo '# Instructions at:' >> $1
    echo '# https://elk-audio.github.io/elk-docs/html/documents/building_plugins_for_elk.html#cross-compiling-juce-plugin' >> $1
    echo '' >> $1
    echo 'unset LD_LIBRARY_PATH' >> $1
    echo 'source /opt/elk/1.0/environment-setup-aarch64-elk-linux' >> $1
    echo 'export CXXFLAGS="-O3 -pipe -ffast-math -feliminate-unused-debug-types -funroll-loops"' >> $1
    echo 'AR=aarch64-elk-linux-ar make -j`nproc` CONFIG=Release CFLAGS="-DJUCE_HEADLESS_PLUGIN_CLIENT=1 -Wno-psabi" TARGET_ARCH="-mcpu=cortex-a72 -mtune=cortex-a72"' >> $1
}


print_elk_config () {
    echo '{' > $1
    echo '    "host_config" : {' >> $1
    echo '        "samplerate" : 48000' >> $1
    echo '    },' >> $1
    echo '    "tracks" : [' >> $1
    echo '        {' >> $1
    echo '            "name" : "main",' >> $1
    echo '            "mode" : "stereo",' >> $1
    echo '            "inputs" : [' >> $1
    echo '                {' >> $1
    echo '                    "engine_bus" : 0,' >> $1
    echo '                    "track_bus" : 0' >> $1
    echo '                }' >> $1
    echo '            ],' >> $1
    echo '            "outputs" : [' >> $1
    echo '                {' >> $1
    echo '                    "engine_bus" : 0,' >> $1
    echo '                    "track_bus" : 0' >> $1
    echo '                }' >> $1
    echo '            ],' >> $1
    echo '            "plugins" : [' >> $1
    echo '                {' >> $1
    echo '                    "path" : "'$2'",' >> $1
    echo '                    "name" : "guitarclassifiervst_'$3'",' >> $1
    echo '                    "type" : "vst2x"' >> $1
    echo '                }' >> $1
    echo '            ]' >> $1
    echo '        }' >> $1
    echo '    ],' >> $1
    echo '    "midi" : {' >> $1
    echo '        "cc_mappings": [' >> $1
    echo '        ]' >> $1
    echo '    }' >> $1
    echo '}' >> $1
}

mkdir -p bin
cd bin
rm -rf *
cd ..

for build_folder in ./Builds/LinuxMakefile-*; do
    print_compile_script $build_folder'/compileRPI4.sh'
    chmod +x $build_folder'/compileRPI4.sh'

    bname=$(get_bname $build_folder)
    echo "Building "$bname" version"
    
    cd $(realpath  $build_folder)
    mkdir -p config
    print_elk_config config/config_$bname.json $ELK_DESTINATION_FOLDER/plugins/guitarTimbreClassifier$bname.so $bname
    ./compileRPI4.sh
    cd $BASE/bin 
    ln -s $(realpath  .$build_folder)/build/Demo-guitarTimbreClassifier.so ./guitarTimbreClassifier$bname.so

    mkdir -p configs
    cd configs
    # echo $(realpath  ../.$build_folder)/config/config_$bname.json
    ln -s $(realpath  ../.$build_folder)/config/config_$bname.json .
    cd ..

    if [[ "${COPY_TO_REMOTE}" == "true" ]]; then
        scp ./guitarTimbreClassifier$bname.so mind@$ELK_DESTINATION_IP:$ELK_DESTINATION_FOLDER/plugins/
        scp ./configs/config_$bname.json mind@$ELK_DESTINATION_IP:$ELK_DESTINATION_FOLDER/configs/
    fi
    cd $BASE
done


print_start_script () {
    echo '#!/bin/bash' > $1
    echo '' >> $1
    echo $ELK_DESTINATION_FOLDER'/stopClassifier.sh   # stop eventual executions' >> $1
    echo '' >> $1
    echo 'PS3="Chose Neural Inference Engine to use with the plugin or Quit: " ' >> $1
    echo 'options=(\'>> $1
    for build_folder in ../Builds/LinuxMakefile-*; do
        bname=$(get_bname $build_folder)
        echo '         "'$bname'" \'>> $1
    done
    echo '         "Quit")'>> $1
    echo 'select opt in "${options[@]}"'>> $1
    echo 'do'>> $1
    echo '    case $opt in'>> $1

    for build_folder in ../Builds/LinuxMakefile-*; do
        bname=$(get_bname $build_folder)
        echo '        "'$bname'")'>> $1
        echo '            echo "You chose "$opt'>> $1
        echo '            CONFIG_FILE_PATH="'$ELK_DESTINATION_FOLDER'/configs/config_'$bname'.json"'>> $1
        echo '            break'>> $1
        echo '            ;;'>> $1
    done
    echo '        "Quit")'>> $1
    echo '            break'>> $1
    echo '            ;;'>> $1
    echo '        *) echo "invalid option $REPLY";;'>> $1
    echo '    esac'>> $1
    echo 'done'>> $1


    echo '# Launch sushi with classifier plugin' >> $1
    echo 'sushi -r  --timing-statistics -c $CONFIG_FILE_PATH &' >> $1
    echo '' >> $1
    echo 'sleep 2' >> $1
    echo '' >> $1
    echo '' >> $1
    echo '' >> $1
    echo '# ORIGLOGNAME=$(ls /tmp/guit* | tail -1 | grep -o -P "guit.+")' >> $1
    echo '# STATSLOGNAME="/tmp/$ORIGLOGNAME.cpumem.log" ' >> $1
    echo '' >> $1
    echo '# '$ELK_DESTINATION_FOLDER'/logutils/logcpumem.py $(pgrep sushi) $STATSLOGNAME &' >> $1
    echo '' >> $1
    echo '# echo "Classifier log at $ORIGLOGNAME"' >> $1
    echo '# echo ""' >> $1
    echo '# echo "CPU/MEM usage log at $STATSLOGNAME"' >> $1
    echo '# echo ""' >> $1

}
cd bin
print_start_script startTechniqueClassifier.sh
if [[ "${COPY_TO_REMOTE}" == "true" ]]; then
    scp startTechniqueClassifier.sh mind@$ELK_DESTINATION_IP:$ELK_DESTINATION_FOLDER/
fi
cd ..